#ifndef TINK_HANDLE_MANAGER_H
#define TINK_HANDLE_MANAGER_H
#include <string>
#include <map>
#include <shared_mutex>
#include <memory>
#include "base/noncopyable.h"
#include "base/context.h"
#include "base/singleton.h"
#include "net/server.h"

//#define HANDLE_STORAGE tink::Singleton<tink::HandleMgr>::GetInstance()

namespace tink {
    class Server;
    class Context;
    class HandleMgr : public noncopyable {
    public:
        using ContextPtr = std::shared_ptr<Context>;

        explicit HandleMgr(std::shared_ptr<Server> server) : server_(server) {}
        int Init(int harbor);
        // 创建一个新服务的context
        ContextPtr CreateContext(std::string_view name, std::string_view param);
        uint32_t Register(ContextPtr ctx);
        std::string BindName(uint32_t handle, std::string_view name);
        int Unregister(int handle);
        void UnregisterAll();
        uint32_t FindName(std::string_view name);
        uint32_t QueryName(std::string_view name);
        ContextPtr HandleGrab(uint32_t handle);
        int PushMessage(uint32_t handle, TinkMessage &msg);
        void ContextEndless(uint32_t handle);
    private:
        std::shared_ptr<Server> server_;
        uint32_t harbor_; // 组网的唯一harbor id
        uint32_t handle_index_; // 用于生成handle id
        std::map<std::string, uint32_t> name_map_; // <ctx名字,handle>
        std::unordered_map<uint32_t, ContextPtr> handle_map_; // <handle, context>
        mutable std::shared_mutex mutex_;
    };
}



#endif //TINK_HANDLE_MANAGER_H
