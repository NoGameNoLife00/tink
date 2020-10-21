#include <server.h>
#include <cstring>
#include <unistd.h>
#include <config_mng.h>
#include <scope_guard.h>
#include <common.h>
#include <message.h>
#include <context.h>
#include <context_manage.h>
#include <signal.h>
#include <daemon.h>
#include <module_manage.h>
#include <thread.h>
#include <sstream>
#include "harbor.h"
#include "timer.h"
#include "monitor.h"


#define MAX_BUF_SIZE 2048
namespace tink {
    static volatile int SIG = 0;

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
        CurrentHandle::InitThread(THREAD_MAIN);
        Sigign();
        srand(static_cast<unsigned>(time(nullptr)));
        // register SIGHUP for log file reopen
        struct sigaction sa;
        sa.sa_handler = &HandleHup;
        sa.sa_flags = SA_RESTART;
        sigfillset(&sa.sa_mask);
        sigaction(SIGHUP, &sa, NULL);
        if (!config_->GetDaemon().empty()) {
            if (Daemon::Init(config_->GetDaemon())) {
                exit(1);
            }
        }
        HarborInstance.Init(config_->GetHarbor());
        CONTEXT_MNG.Init(config_->GetHarbor());
        MODULE_MNG.Init(config_->GetModulePath());
        TimerInstance.Init();
        SOCKET_SERVER.Init(TimerInstance.Now());
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
        CurrentHandle::InitThread(THREAD_MONITOR);
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

        uint32_t logger = CONTEXT_MNG.FindName("logger");
        if (logger) {
            CONTEXT_MNG.PushMessage(logger, smsg);
        }
    }

    static void ThreadTimer(MonitorPtr m) {
        CurrentHandle::InitThread(THREAD_TIMER);
        for (;;) {
            TimerInstance.UpdateTime();
            // socket_updatetime();
            CHECK_ABORT
            Wakeup(*m, m->count - 1);
            usleep(2500);
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
        return;
    }


    static void ThreadSocket(MonitorPtr m) {
        CurrentHandle::InitThread(THREAD_SOCKET);
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
            q = GLOBAL_MQ.Pop();
            if (!q) {
                return nullptr;
            }
        }
        uint32_t handle = q->Handle();
        ContextPtr ctx = CONTEXT_MNG.HandleGrab(handle);
        if (!ctx) {
            struct DropT d = {handle };
            q->Release(Context::DropMessage, &d);
            return GLOBAL_MQ.Pop();
        }
        int n = 1;
        TinkMessage msg;
        for (int i = 0; i < n; i++) {
            if (q->Pop(msg)) {
                return GLOBAL_MQ.Pop();
            } else if (i == 0 && weight >= 0) {
                n = q->Size();
                n >>= weight;
            }
            m_node.Trigger(msg.source, handle);
            ctx->DispatchMessage(msg);
            m_node.Trigger(0, 0);
        }
        assert(q.get() == ctx->Queue().get());
        MQPtr nq = GLOBAL_MQ.Pop();
        if (nq) {
            GLOBAL_MQ.Push(q);
            q = nq;
        }
        return q;
    }


    static void ThreadWorker(MonitorPtr m, int id, int weight) {
        MonitorNodePtr m_node = m->m[id];
        CurrentHandle::InitThread(THREAD_WORKER);
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
        thread_list.emplace_back(std::make_shared<Thread>( [m] { return ThreadSocket(m); }, "timer"));


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
        std::istringstream is(cmdline);
        string name, args;
        is >> name >> args;
        ContextPtr ctx = CONTEXT_MNG.CreateContext(name, args);
        if (!ctx) {
            spdlog::error("bootstrap error: {}", cmdline);
            exit(1);
        }
    }


}
