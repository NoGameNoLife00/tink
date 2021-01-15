#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "base/base_module.h"




namespace tink::Service {
    class ServiceCLua : public BaseModule {
    public:
        int Init(std::shared_ptr<Context> ctx, std::string_view param) override;

        void Release() override;

        void Signal(int signal) override;

        const string &Name() override;
        int InitCB(DataPtr msg, size_t sz);

        lua_State* L;
        size_t mem;
        size_t mem_report;
        size_t mem_limit;
        lua_State* active_L;
        volatile int trap;
    private:
        static int LaunchCB(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        ContextPtr ctx_;
    };


    int ServiceCLua::LaunchCB(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        assert(type == 0 && session == 0);
        auto *l = reinterpret_cast<ServiceCLua*>(ud);
        l->ctx_->SetCallBack(nullptr, nullptr);
        if (l->InitCB(msg, sz)) {
            l->ctx_->Command("EXIT", "");
        }
        return 0;
    }

    int ServiceCLua::Init(std::shared_ptr<Context> ctx, std::string_view param) {
        ctx_ = ctx;
        int sz = param.length();
        BytePtr tmp = std::make_shared<byte[]>(sz);
        memcpy(tmp.get(), param.data(), sz);
        ctx_->SetCallBack(LaunchCB, this);
        std::string self = ctx_->Command("REG", "");
        uint32_t handle_id = strtoul(self.data()+1, nullptr, 16);
        ctx_->Send(0, handle_id, PTYPE_TEXT, 0, tmp, sz);
    }

    static void SignalHook(lua_State* L, lua_Debug* ar) {
        void *ud = nullptr;
        lua_getallocf(L, &ud);
        auto* l = reinterpret_cast<ServiceCLua*>(ud);

        lua_sethook(L, nullptr, 0, 0);
        if (l->trap) {
            l->trap = 0;
            luaL_error(L, "signal 0");
        }

    }

    static int cleardummy(lua_State* L) {
        return 0;
    }

    static int codecache(lua_State* L) {
        luaL_Reg l[] = {
                { "clear", cleardummy },
                { "mode", cleardummy },
                { NULL, NULL },
        };
        luaL_newlib(L,l);
        lua_getglobal(L, "loadfile");
        lua_setfield(L, -2, "loadfile");
        return 1;
    }

    static void SwitchL(lua_State* L, ServiceCLua* l) {
        l->active_L = L;
        if (l->trap) {
            lua_sethook(L, SignalHook, LUA_MASKCALL, 1);
        }
    }

    static int lua_resumeX(lua_State* L, lua_State* from, int nargs, int& n_results) {
        void *ud = nullptr;
        lua_getallocf(L, &ud);
        auto* l = reinterpret_cast<ServiceCLua*>(ud);
        SwitchL(L, l);
        int err = lua_resume(L, from, nargs, &n_results);
        if (l->trap) {
            // 等lua_sethook设置trap == -1
            while (l->trap >= 0) ;
        }
        SwitchL(from, l);
        return err;
    }

    // 协程库, profile

    // 协程resume,没错误时返回参数数量，出现错误返回-1
    static int AuxResume(lua_State* L, lua_State* co, int n_arg) {
        int status, n_res;
        if (!lua_checkstack(co, n_arg)) {
            lua_pushliteral(L, "to many argument to resume");
            return -1;
        }

        lua_xmove(L, co, n_arg);
        status = lua_resumeX(co, L, n_arg, n_res);
        if (status == LUA_OK || status == LUA_YIELD) {
            if (!lua_checkstack(L, n_res + 1)) {
                lua_pop(co, n_res);
                lua_pushliteral(L, "too many result to resume");
                return -1;
            }
            lua_xmove(co, L, n_res);
            return n_res;
        } else {
            lua_xmove(co, L, 1);
            return -1;
        }
    }

    static int TimingEnable(lua_State* L, int co_index, lua_Number &start_time) {
        lua_pushvalue(L, co_index);
        lua_rawget(L, lua_upvalueindex(1));
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return 0;
        }
        start_time = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return 1;
    }

    static double TimingTotal(lua_State* L, int co_index) {
        lua_pushvalue(L, co_index);
        lua_rawget(L, lua_upvalueindex(2));
        double total_time = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return total_time;
    }

    static double GetTime() {
        struct timespec ti;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ti);
        int sec = ti.tv_sec & 0xffff;
        int nsec = ti.tv_nsec;
        return sec + nsec / 1000000000;
    }

    static inline double DiffTime(double start) {
        double now = GetTime();
        if (now < start) {
            return now + 0x10000 - start;
        } else {
            return now - start;
        }
    }

    static int TimingResume(lua_State* L, int co_index) {
        lua_State *co = lua_tothread(L, co_index);
        lua_Number start_time = 0;
        if (TimingEnable(L, co_index, start_time)) {
            start_time = GetTime();
#ifdef DEBUG
            spdlog::debug("PROFILE [{}] resume {}", co, start_time);
#endif
            lua_pushvalue(L, co_index);
            lua_pushnumber(L, start_time);
            lua_rawset(L, lua_upvalueindex(1));
        }
        int r = AuxResume(L, co, lua_gettop(L) - 1);
        if (TimingEnable(L, co_index, start_time)) {
            double total_time = TimingTotal(L, co_index);
            double diff = DiffTime(start_time);
            total_time += diff;
#ifdef DEBUG
            spdlog::debug("PROFILE [{}] yield {}/{}\n", co, diff, total_time);
#endif
            lua_pushvalue(L, co_index);
            lua_pushnumber(L, total_time);
            lua_rawset(L, lua_upvalueindex(2));
        }
        return r;
    }











    static int lstart(lua_State *L) {
        if (lua_gettop(L) != 0) {
            lua_settop(L, 1);
            luaL_checktype(L, 1, LUA_TTHREAD);
        } else {
            lua_pushthread(L);
        }
        lua_Number start_time = 0;
        if (TimingEnable(L, 1, start_time)) {
            return luaL_error(L, "Thread %p start profile more than once", lua_topointer(L, 1));
        }
        // 重置总时间
        lua_pushvalue(L, 1);
        lua_pushnumber(L, 0);
        lua_rawset(L, lua_upvalueindex(2));

        // 设置开始时间
        lua_pushvalue(L, 1);
        start_time = GetTime();
#ifdef DEBUG
        spdlog::debug("PROFILE [{}] start", start_time);
#endif
        lua_pushnumber(L, start_time);
        lua_rawset(L, lua_upvalueindex(1));
        return 0;
    }

    static int lstop(lua_State* L) {
        if (lua_gettop(L) != 0) {
            lua_settop(L, 1);
            luaL_checktype(L, 1, LUA_TTHREAD);
        } else {
            lua_pushthread(L);
        }
        lua_Number start_time = 0;
        if (!TimingEnable(L, 1, start_time)) {
            return luaL_error(L, "Call profile.start() before profile.stop()");
        }
        double ti = DiffTime(start_time);
        double total_time = TimingTotal(L, 1);

        lua_pushvalue(L, 1); // push coroutine
        lua_pushnil(L);
        lua_rawset(L, lua_upvalueindex(1));

        lua_pushvalue(L, 1); // push coroutine
        lua_pushnil(L);
        lua_rawset(L, lua_upvalueindex(2));

        total_time += ti;
        lua_pushnumber(L, total_time);
#ifdef DEBUG
        spdlog::debug("PROFILE [{}] stop ({}/{})", lua_tothread(L,1), ti, total_time);
#endif
        return 1;
    }

    static int luaB_coresume(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTHREAD);
        int r = TimingResume(L, 1);
        if (r < 0) {
            lua_pushboolean(L, 0);
            lua_insert(L, -2);
            return 2; // return false and error message
        } else {
            lua_pushboolean(L, 1);
            lua_insert(L, -(r + 1));
            return r + 1; // return true and resume return
        }
    }

    static int luaB_auxwrap(lua_State* L) {
        lua_State *co = lua_tothread(L, lua_upvalueindex(3));
        int r = TimingResume(L, lua_upvalueindex(3));
        if (r < 0) {
            int stat = lua_status(co);
            if (stat != LUA_OK && stat != LUA_YIELD) {
                lua_resetthread(co); // 发生错误 关闭co
            }
            // 错误信息是字符串
            if (lua_type(L, -1) == LUA_TSTRING) {
                luaL_where(L, 1); // 添加额外信息
                lua_insert(L, -2);
                lua_concat(L, 2);
            }
            return lua_error(L);
        }
        return r;
    }

    static int luaB_cocreate(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_State *NL = lua_newthread(L);
        lua_pushvalue(L, 1); // 移动function到栈顶
        lua_xmove(L, NL, 1); // 将function从L移动到NL
        return 1;
    }

    static int luaB_cowrap(lua_State* L) {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushvalue(L, lua_upvalueindex(2));
        luaB_cocreate(L);
        lua_pushcclosure(L, luaB_auxwrap, 3);
        return 1;
    }

    static int InitProfile(lua_State *L) {
        luaL_Reg l[] = {
                { "start", lstart },
                { "stop", lstop },
                { "resume", luaB_coresume },
                { "wrap", luaB_cowrap },
                { NULL, NULL },
        };
        luaL_newlibtable(L,l);
        lua_newtable(L);	// table thread->start time
        lua_newtable(L);	// table thread->total time

        lua_newtable(L);	// weak table
        lua_pushliteral(L, "kv");
        lua_setfield(L, -2, "__mode");

        lua_pushvalue(L, -1);
        lua_setmetatable(L, -3);
        lua_setmetatable(L, -3);

        luaL_setfuncs(L,l,2);

        return 1;
    }

    static std::string OptString(Context& ctx, std::string_view key, std::string_view str) {
        std::string ret = ctx.Command("GETENV", key);
        if (ret.empty()) {
            return string (str);
        }
        return ret;
    }

    static int Traceback(lua_State *L) {
        const char* msg = lua_tostring(L, 1);
        if (msg) {
            luaL_traceback(L, L, msg, 1);
        } else {
            lua_pushliteral(L, "(no error message)");
        }
        return 1;
    }

    static void ReportLauncherError(Context& ctx) {
        DataPtr msg(new byte[6] {"ERROR"}, std::default_delete<byte[]>());
        ctx.SendName(0, ".launcher", PTYPE_TEXT, 0, msg, 5);
    }

    int ServiceCLua::InitCB(DataPtr msg, size_t sz) {
        lua_State* L = this->L;
        lua_gc(L, LUA_GCSTOP, 0);
        lua_pushboolean(L, 1); /* 让lua忽略env. vars. */
        lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
        luaL_openlibs(L);
        luaL_requiref(L, "tink.profile", InitProfile, 0);

        int profile_lib = lua_gettop(L);
        // 替换lua原有的coroutine.resume / corotine.warp
        lua_getglobal(L, "coroutine");
        lua_getfield(L, profile_lib, "resume");
        lua_setfield(L, -2, "resume");
        lua_getfield(L, profile_lib, "warp");
        lua_setfield(L, -2, "warp");

        lua_settop(L, profile_lib - 1);

        lua_pushlightuserdata(L, ctx_.get());
        lua_setfield(L, LUA_REGISTRYINDEX, "tink_context");
        luaL_requiref(L, "tink.codecache", codecache, 0);
        lua_pop(L, 1);

        lua_gc(L, LUA_GCGEN, 0, 0);

        auto path = OptString(*ctx_, "lua_path", "./lualib/?.lua;./lualib/?/init.lua");
        lua_pushstring(L, path.c_str());
        lua_setglobal(L, "LUA_PATH");
        auto cpath = OptString(*ctx_, "lua_cpath", "./luaclib/?.so");
        lua_pushstring(L, cpath.c_str());
        lua_setglobal(L, "LUA_CPATH");
        auto service = OptString(*ctx_, "luaservice", "./service/?.lua");
        lua_pushstring(L, service.c_str());
        lua_setglobal(L, "LUA_SERVICE");
        auto preload = ctx_->Command("GETENV", "preload");
        lua_pushstring(L, preload.c_str());
        lua_setglobal(L, "LUA_PERLOAD");

        lua_pushcfunction(L, Traceback);
        assert(lua_gettop(L) == 1);

        auto loader = OptString(*ctx_, "lualoader", "./lualib/loader.lua");

        if (luaL_loadfile(L, loader.c_str()) != LUA_OK) {
            logger->error("can't load {} : {}", loader, lua_tostring(L, -1));
            ReportLauncherError(ctx_);
            return 1;
        }
        lua_pushlstring(L, static_cast<char*>(msg.get()), sz);
        if (lua_pcall(L, 1, 0, 1)) {
            logger->error("lua loader error: {}", lua_tostring(L, -1));
            ReportLauncherError(ctx_);
            return 1;
        }
        lua_settop(L, 0);
        if (lua_getfield(L, LUA_REGISTRYINDEX, "memlimit") == LUA_TNUMBER) {
            size_t limit = lua_tointeger(L, -1);
            mem_limit = limit;
            logger->error("set memory limit to {} M", static_cast<float>(limit) / (1024*1024));
            lua_pushnil(L);
            lua_setfield(L, LUA_REGISTRYINDEX, "memlimit");
        }
        lua_pop(L, 1);
        lua_gc(L, LUA_GCRESTART, 0);
        return 0;
    }
}