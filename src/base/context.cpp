#include "context.h"
#include "handle_storage.h"
#include "string_util.h"
#include <module_manage.h>
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <common.h>
#include <timer.h>
#include <harbor.h>
#include <sstream>
#include <server.h>

namespace tink {
    std::atomic_int Context::total = 0;

    void Context::Destroy() {
        mod_->Release();
        queue_->MarkRelease();
        --total;
    }



    void Context::SetCallBack(const ContextCallBack &cb, void *ud) {
        callback_ = cb;
        cb_ud_ = ud;
    }

    int Context::NewSession() {
        int session = ++session_id_;
        if (session <= 0) {
            session_id_ = 1;
            return session_id_;
        }
        return session;
    }

    void Context::DispatchAll() {
        TinkMessage msg;
        while (queue_->Pop(msg, true)) {
            DispatchMessage(msg);
        }
    }

    void Context::DispatchMessage(TinkMessage &msg) {
        assert(init_);
        if (!callback_) {
            return;
        }
        std::lock_guard<Mutex> guard(mutex_);
        Global::SetHandle(handle_);
        int type = msg.size >> MESSAGE_TYPE_SHIFT;
        size_t sz = msg.size & MESSAGE_TYPE_MASK;
        message_count_++;
        // 消息回调
        if (profile_) {
            cpu_start_ = TimeUtil::GetThreadTime();
            callback_(cb_ud_, type, msg.session, msg.source, msg.data, sz);
            uint64_t cost_tm = TimeUtil::GetThreadTime() - cpu_start_;
            cpu_cost_ += cost_tm;
        } else {
            callback_(cb_ud_, type, msg.session, msg.source, msg.data, sz);
        }
    }

    void Context::Send(DataPtr data, size_t sz, uint32_t source, int type, int session) {
        TinkMessage msg;
        msg.source = source;
        msg.session = session;
        msg.data = std::move(data);
        msg.size = sz | (static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT);
        queue_->Push(msg);
    }

    int Context::Send(uint32_t source, uint32_t destination, int type, int session, DataPtr data,
                      size_t sz) {
        if ((sz & MESSAGE_TYPE_MASK) != sz) {
            spdlog::error("The message to {} is too large", destination);
            return E_PACKET_SIZE;
        }
        FilterArgs_(type, session, data, sz);
        if (source == 0) {
            source = handle_;
        }
        if (destination == 0) {
            if (data) {
                spdlog::error("Destination address can't be 0");
                data.reset();
                return E_FAILED;
            }
            return session;
        }
        if (HARBOR.MessageIsRemote(destination)) {
            // 发送目标不在同一节点,交给harbor处理
            RemoteMessagePtr r_msg = std::make_shared<RemoteMessage>();
            r_msg->destination.handle = destination;
            r_msg->message = data;
            r_msg->size = sz & MESSAGE_TYPE_MASK;
            r_msg->type = sz >> MESSAGE_TYPE_SHIFT;
            HARBOR.Send(r_msg, source, session);
        } else {
            TinkMessage s_msg;
            s_msg.source = source;
            s_msg.session = session;
            s_msg.data = data;
            s_msg.size = sz;
            // push到目标ctx的消息队列
            if (HANDLE_STORAGE.PushMessage(destination, s_msg)) {
                return E_FAILED;
            }
        }
        return session;
    }

    int Context::FilterArgs_(int type, int &session, DataPtr data, size_t &sz) {
//        int needcopy = !(type & PTYPE_TAG_DONTCOPY);
        // 如果在type里设上PTYPE_TAG_ALLOCSESSION，就忽略掉传入的session参数, 重新生成个唯一session
        int allocsession = type & PTYPE_TAG_ALLOCSESSION;
        type &= 0xff;

        if (allocsession) {
            assert(session == 0);
            session = NewSession();
        }

        sz |= static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT;
        return 0;
    }

    static void CopyName(char name[GLOBALNAME_LENGTH], const char * addr) {
        int i;
        for (i=0; i < GLOBALNAME_LENGTH && addr[i]; i++) {
            name[i] = addr[i];
        }
        for (; i < GLOBALNAME_LENGTH; i++) {
            name[i] = '\0';
        }
    }

    int Context::SendName(uint32_t source, std::string_view addr, int type, int session, DataPtr data, size_t sz) {
        if (source == 0) {
            source = handle_;
        }
        uint32_t  des = 0;
        if (addr[0] == ':') {
            des = strtoul(addr.data()+1, nullptr, 16);
        } else if ( addr[0] == '.') {
            des = HANDLE_STORAGE.FindName(addr.substr(1));
            if (des == 0) {
                return E_FAILED;
            }
        } else {
            if ((sz & MESSAGE_TYPE_MASK) != sz) {
                spdlog::error("The message to {} is too large", addr);
                return E_FAILED;
            }
            FilterArgs_(type, session, data, sz);

            RemoteMessagePtr r_msg = std::make_shared<RemoteMessage>();
            CopyName(r_msg->destination.name, addr.data());
            r_msg->destination.handle = 0;
            r_msg->message = data;
            r_msg->size = sz & MESSAGE_TYPE_MASK;
            r_msg->type = sz >> MESSAGE_TYPE_SHIFT;
            HARBOR.Send(r_msg, source, session);
            return session;
        }
        return Send(source, des, type, session, data, sz);
    }

    void Context::DropMessage(TinkMessage &msg, void *ud) {
        auto *d = static_cast<DropT *>(ud);
        msg.data.reset();
        uint32_t source = d->handle;
        assert(source);
    }


    std::string CMD_Timeout(Context& context, std::string_view& param) {
        char* end_ptr = nullptr;
        int ti = strtol(param.data(), &end_ptr, 10);
        int session = context.NewSession();
        TIMER.TimeOut(context.Handle(), ti, session);
        context.result = std::to_string(session);
        return context.result;
    }

    std::string CMD_Reg(Context& context, std::string_view& param) {
        if (param.empty()) {
            context.result = StringUtil::Format(":%x", context.Handle());
            return context.result;
        } else if (param[0] == '.') {
            auto&& name = param.substr(1);
            return HANDLE_STORAGE.BindName(context.Handle(), name);
        } else {
            spdlog::error("can't register global name %s", param);
            return "";
        }
    }

    std::string CMD_Query(Context& context, std::string_view& param) {
        if (param[0] == '.') {
            uint32_t handle = HANDLE_STORAGE.FindName(param.substr(1));
            if (handle) {
                context.result = StringUtil::Format(":%x", context.Handle());
                return context.result;
            }
        }
        return "";
    }

    std::string CMD_Name(Context& context, std::string_view& param) {
        string name;
        string handle;
        std::istringstream is(param.data());
        is >> name >> handle;
        if (handle[0] != ':') {
            return "";
        }
        uint32_t handle_id = strtoul(handle.data() + 1, nullptr, 16);
        if (handle_id == 0) {
            return "";
        }
        if (name[0] == '.') {
            return HANDLE_STORAGE.BindName(context.Handle(), name.data() + 1);
        } else {
            spdlog::error("Can't set global name %s", name);
        }
        return "";
    }

    std::string CMD_Exit(Context& context, std::string_view& param) {
        context.Exit(0);
        return "";
    }

    std::string CMD_Kill(Context& context, std::string_view& param) {
        uint32_t handle = context.ToHandle(param);
        if (handle) {
            context.Exit(handle);
        }
        return "";
    }

    std::string CMD_Launch(Context& context, std::string_view& param) {
        string tmp(param);
        char *args = tmp.data();
        char * mod = strsep(&args, " \t\r\n");
        args = strsep(&args, "\r\n");
        ContextPtr inst = HANDLE_STORAGE.CreateContext(mod, args);
        if (!inst) {
            return "";
        } else {
            StringUtil::IdToHex(context.result, inst->Handle());
            return context.result;
        }
    }

    std::string CMD_StartTime(Context& context, std::string_view& param) {
        uint32_t sec = TIMER.StartTime();
        context.result = std::to_string(sec);
        return context.result;
    }

    std::string CMD_Abort(Context& context, std::string_view& param) {
        HANDLE_STORAGE.UnregisterAll();
        return "";
    }

    std::string CMD_Monitor(Context& context, std::string_view& param) {
        uint32_t handle;
        if (param.empty()) {
            if (Global::MonitorExit()) {
                context.result = StringUtil::Format(":%x", Global::MonitorExit());
                return context.result;
            }
            return "";
        } else {
            handle = context.ToHandle(param);
        }
        Global::SetMonitorExit(handle);
        return "";
    }

    std::string CMD_Stat(Context& context, std::string_view param) {
        if (param == "mqlen") {
            context.result = std::to_string(context.Queue()->Size());
        } else if (param == "endless") {
            if (context.Endless()) {
                context.result = "1";
                context.SetEndless(false);
            } else {
                context.result = "0";
            }
        } else if (param == "cpu") {
            double t = static_cast<double>(context.GetCpuCost()) / 1000000.0;	// microsec
            context.result = std::to_string(t);
        } else if (param == "time") {
            if (context.GetProfile()) {
                uint64_t ti = TimeUtil::GetThreadTime() - context.GetCpuStart();
                context.result = std::to_string(ti);
            } else {
                context.result = "0";
            }
        } else if (param == "message") {
            context.result = std::to_string(context.GetMessageCount());
        } else {
            context.result.clear();
        }
        return context.result;
    }

    std::string CMD_Signal(Context& context, std::string_view param) {
        uint32_t handle = context.ToHandle(param);
        if (handle == 0) {
            return "";
        }
        ContextPtr ctx = HANDLE_STORAGE.HandleGrab(handle);
        if (!ctx) {
            return "";
        }
        const char* p = strchr(param.data(), ' ');
        int sig = 0;
        if (p) {
            sig = strtol(p, nullptr, 0);
        }
        ctx->GetModule()->Signal(sig);
        return "";
    }

    typedef std::function<std::string(Context&, std::string_view&)> CmdFunc;
    typedef std::pair<std::string, CmdFunc> CommandPair;
    static CommandPair cmd_funcs[] = {
            { "TIMEOUT",   CMD_Timeout },
            { "REG",       CMD_Reg },
            { "QUERY",     CMD_Query },
            { "NAME",      CMD_Name },
            { "EXIT",      CMD_Exit },
            { "KILL",      CMD_Kill },
            { "LAUNCH",    CMD_Launch },
//            { "GETENV",    cmd_getenv },
//            { "SETENV",    cmd_setenv },
            { "STARTTIME", CMD_StartTime },
            { "ABORT",     CMD_Abort },
            { "MONITOR",   CMD_Monitor },
            { "STAT",      CMD_Stat },
//            { "LOGON", cmd_logon },
//            { "LOGOFF", cmd_logoff },
            { "SIGNAL", CMD_Signal },
    };



    std::string Context::Command(std::string_view cmd, std::string_view param) {
        for (auto& it : cmd_funcs) {
            if (it.first == cmd) {
                return it.second(*this, param);
            }
        }
        return "";
    }

    void Context::Exit(uint32_t handle) {
        if (handle == 0) {
            handle = handle_;
            spdlog::warn("kill self");
        } else {
            spdlog::warn( "kill :{:x}", handle);
        }
        if (Global::MonitorExit()) {
            Send(handle, Global::MonitorExit(), PTYPE_CLIENT, 0, nullptr, 0);
        }
        HANDLE_STORAGE.Unregister(handle);
    }

    uint32_t Context::ToHandle(std::string_view param) {
        uint32_t handle = 0;
        if (param[0] == ':') {
            handle = strtoul(param.data()+1, nullptr, 16);
        } else if (param[0] == '.') {
            handle = HANDLE_STORAGE.FindName(param.substr(1));
        } else {
            spdlog::error("Can't convert %s to handle", param);
        }
        return handle;
    }
}

