#ifndef TINK_CONTEXT_MANAGE_H
#define TINK_CONTEXT_MANAGE_H


#include <shared_mutex>
#include <context.h>
#include <map>
#include <singleton.h>
#include <string>

#define CONTEXT_MNG tink::Singleton<tink::ContextManage>::GetInstance()

namespace tink {
    namespace CurrentHandle {
        extern thread_local uint32_t t_handle;

        inline uint32_t Handle() {
            return t_handle;
        }

        inline void InitThread(int m) {
            t_handle = static_cast<uint32_t>(-m);
        }

        inline void SetHandle(uint32_t h) {
            t_handle = h;
        }
    }

    class ContextManage {
    public:
        int Init(int harbor);
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
        uint32_t harbor_;
        uint32_t handle_index_;
        std::map<std::string, uint32_t> name_map_;
        std::unordered_map<uint32_t, ContextPtr> handle_map_;
        mutable std::shared_mutex mutex_;
    };
}



#endif //TINK_CONTEXT_MANAGE_H
