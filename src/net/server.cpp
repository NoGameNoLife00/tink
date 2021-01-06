#include <server.h>
#include <cstring>
#include <unistd.h>
#include <config.h>
#include <scope_guard.h>
#include <common.h>
#include <message.h>
#include <context.h>
#include <handle_manager.h>
#include <csignal>
#include <daemon.h>
#include <module_manager.h>
#include <thread.h>
#include <harbor.h>
#include <timer.h>
#include <monitor.h>

namespace tink {
    static volatile int SIG = 0;

    std::shared_ptr<Server> &GetGlobalServer() {
        static std::shared_ptr<Server> g_server;
        return g_server;
    }

    namespace Global {
        thread_local uint32_t t_handle = 0;
        uint32_t monitor_exit = 0;
        bool profile = false;
    }

    static void HandleHup(int signal) {
        if (signal == SIGHUP) {
            SIG = 1;
        }
    }

    int Sigign() {
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGPIPE, &sa, 0);
        return 0;
    }


    int Server::Init(ConfigPtr config) {
        config_ = config;
        Global::InitThread(THREAD_MAIN);
        Sigign();
        srand(static_cast<unsigned>(time(nullptr)));
        // register SIGHUP for log file reopen
        struct sigaction sa;
        sa.sa_handler = &HandleHup;
        sa.sa_flags = SA_RESTART;
        sigfillset(&sa.sa_mask);
        sigaction(SIGHUP, &sa, nullptr);
        if (!config_->GetDaemon().empty()) {
            if (Daemon::Init(config_->GetDaemon())) {
                exit(1);
            }
        }
        HARBOR.Init(config_->GetHarbor());
        HANDLE_STORAGE.Init(config_->GetHarbor());
        MODULE_MNG.Init(config_->GetModulePath());
        TIMER.Init();
        SOCKET_SERVER.Init(TIMER.Now());
        Bootstrap(config->GetBootstrap());
        return 0;
    }


    static void Wakeup(Monitor &m, int busy) {
        if (m.sleep >= m.count - busy) {
            // signal sleep worker, "spurious wakeup" is harmless
            m.cond.notify_one();
        }
    }


#define CHECK_ABORT if (Context::Total()==0) break;

    static void ThreadMonitor(MonitorPtr m) {
        int i;
        int n = m->count;
        Global::InitThread(THREAD_MONITOR);
        for (;;) {
            CHECK_ABORT
            for (i=0;i<n;i++) {
                m->m[i]->Check();
            }
            for (i=0;i<5;i++) {
                CHECK_ABORT
                sleep(1);
            }
        }
    }


    static void SignalHup() {
        // make log file reopen
        TinkMessage smsg;
        smsg.source = 0;
        smsg.session = 0;
        smsg.data = nullptr;
        smsg.size = static_cast<size_t>(PTYPE_SYSTEM) << MESSAGE_TYPE_SHIFT;

        uint32_t logger = HANDLE_STORAGE.FindName("logger");
        if (logger) {
            HANDLE_STORAGE.PushMessage(logger, smsg);
        }
    }

    static void ThreadTimer(MonitorPtr m) {
        Global::InitThread(THREAD_TIMER);
        for (;;) {
            TIMER.UpdateTime();
            // socket_updatetime();
            CHECK_ABORT
            Wakeup(*m, m->count - 1);
            usleep(2500); // 0.0025s
            if (SIG) {
                SignalHup();
                SIG = 0;
            }
        }
        // wakeup socket thread
        // socket_exit();
        // wakeup all worker thread
        m->mutex.lock();
        m->quit = 1;
        m->cond.notify_all();
        m->mutex.unlock();
   }


    static void ThreadSocket(MonitorPtr m) {
        Global::InitThread(THREAD_SOCKET);
        for (;;) {
            int ret = SOCKET_SERVER.Poll();
            if (ret == 0) {
                break;
            }
            if (ret < 0) {
                CHECK_ABORT
                continue;
            }
            Wakeup(*m, 0);
        }
    }

    MQPtr ContextMessageDispatch(MonitorNode &m_node, MQPtr q, int weight) {
        if (!q) {
            q = GetGlobalMQ().Pop();
            if (!q) {
                return nullptr;
            }
        }
        uint32_t handle = q->Handle();
        // ?????????§Ö?ctx
        ContextPtr ctx = HANDLE_STORAGE.HandleGrab(handle);
        if (!ctx) {
            // ??????????
            struct DropT d = { handle };
            q->Release(Context::DropMessage, &d);
            return GetGlobalMQ().Pop();
        }
        int n = 1;
        TinkMessage msg;
        for (int i = 0; i < n; i++) {
            if (!q->Pop(msg)) {
                // ?????????????????????????????§Ú???????????
                return GetGlobalMQ().Pop();
            } else if (i == 0 && weight >= 0) {
                // weight:-1??????????????
                // weight>0, i:0?: n=?????????*1/(2^weight) ??
                n = q->Size();
                n >>= weight;
            }
            m_node.Trigger(msg.source, handle);
            ctx->DispatchMessage(msg);
            m_node.Trigger(0, 0);
        }
        assert(q == ctx->Queue());
        MQPtr nq = GetGlobalMQ().Pop();
        if (nq) {
            // ?????????§Ó????????????????push???,????¦É?????????
            GetGlobalMQ().Push(q);
            q = nq;
        }
        return q;
    }


    static void ThreadWorker(MonitorPtr m, int id, int weight) {
        MonitorNodePtr m_node = m->m[id];
        Global::InitThread(THREAD_WORKER);
        MQPtr q;
        while (!m->quit) {
            q = ContextMessageDispatch(*m_node, q, weight);
            if (!q) {
                std::unique_lock lock(m->mutex);
                ++m->sleep;
                if (!m->quit) {
                    m->cond.wait(lock);
                }
                --m->sleep;
            }
        }
    }

    int Server::Start() {
        int thread = config_->GetWorkerPoolSize();
        MonitorPtr m = std::make_shared<Monitor>();
        m->count = thread;
        m->sleep = 0;
        for (int i = 0; i < thread; i++) {
            m->m.emplace_back(std::make_shared<MonitorNode>());
        }
        std::vector<std::shared_ptr<Thread>> thread_list;

        thread_list.emplace_back(std::make_shared<Thread>([m] { return ThreadMonitor(m); }, "monitor"));
        thread_list.emplace_back(std::make_shared<Thread>([m] { return ThreadTimer(m); }, "timer"));
        thread_list.emplace_back(std::make_shared<Thread>([m] { return ThreadSocket(m); }, "socket"));

        static int weight[] = {
        -1, -1, -1, -1, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, };

        for (int i = 0; i < thread; i++) {
            int w = 0;
            int id = i;
            if (i < sizeof(weight)/sizeof(weight[0])) {
                w= weight[i];
            }
            thread_list.emplace_back(std::make_shared<Thread>([this, m, id, w] { return ThreadWorker(m, id, w); }, "worker"));
        }

        for (auto &t : thread_list) {
            t->Start();
        }

        for (auto& t : thread_list) {
            t->Join();
        }
        return E_OK;
    }


    int Server::Stop() {
        spdlog::info("[Stop] tink server name %s", config_->GetName());
        return E_OK;
    }

    void Server::Bootstrap(const string &cmdline) {
        int pos = cmdline.find_first_of(' ');
        std::string name = cmdline.substr(0, pos);
        std::string args = cmdline.substr(pos+1);
        ContextPtr ctx = handler_mgr_->CreateContext(name, args);
        if (!ctx) {
            spdlog::error("bootstrap error: {}", cmdline);
            exit(1);
        }
    }

    ModuleMgr *Server::GetModuleMgr() const {
        return module_mgr_.get();
    }

    HandleMgr *Server::GetHandlerMgr() const {
        return handler_mgr_.get();
    }

    Harbor *Server::GetHarbor() const {
        return harbor_.get();
    }

    GlobalMQ *Server::GetGlobalMQ() const {
        return global_mq_.get();
    }


}
