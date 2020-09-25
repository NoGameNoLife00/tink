//
// Created by Ёблн on 2020/9/21.
//

#ifndef TINK_HANDLE_MANAGE_H
#define TINK_HANDLE_MANAGE_H


#include <shared_mutex>
#include <context.h>
#include <map>
#include <singleton.h>

#define ContextMngInstance tink::Singleton<tink::ContextManage>::GetInstance()

namespace tink {
    namespace CurrentHandle {
        extern thread_local uint32_t t_handle;
        inline uint32_t Handle() {
            return t_handle;
        }
        inline int SetHandle(uint32_t h) {
            t_handle = h;
        }
    }

    class ContextManage {
    public:
        int Init(int harbor);
        uint32_t Register(ContextPtr ctx);
        int BindName(uint32_t handle, std::string& name);
        int Unregister(int handle);
        void UnregisterAll();
        uint32_t FindName(const std::string& name);
        ContextPtr HandleGrab(uint32_t handle);

        int PushMessage(uint32_t handle, MsgPtr &msg);


    private:
        uint32_t harbor_;
        uint32_t handle_index_;
        std::map<std::string, uint32_t> name_map_;
        std::unordered_map<uint32_t, ContextPtr> handle_map_;
        mutable std::shared_mutex mutex_;
    };
}



#endif //TINK_HANDLE_MANAGE_H
