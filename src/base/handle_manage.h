//
// Created by Ёблн on 2020/9/21.
//

#ifndef TINK_HANDLE_MANAGE_H
#define TINK_HANDLE_MANAGE_H


#include <shared_mutex>
#include <context.h>
#include <map>
#include <singleton.h>

#define HandleMngInstance tink::Singleton<tink::HandleManage>::GetInstance()

namespace tink {
    class HandleManage : public  {
    public:
        constexpr static int HANDLE_REMOTE_SHIFT = 24;
        constexpr static int HANDLE_MASK = 0xffffff;
        int Init(int harbor);
        uint32_t Register(ContextPtr ctx);
        int BindName(uint32_t handle, std::string& name);
        int Unregister(int handle);
        void UnregisterAll();
        uint32_t FindName(const std::string& name);
        ContextPtr HandleGrab(uint32_t handle);
    private:
        uint32_t harbor_;
        uint32_t handle_index_;
        std::map<std::string, uint32_t> name_map_;
        std::unordered_map<uint32_t, ContextPtr> handle_map_;
        mutable std::shared_mutex mutex_;
    };
}



#endif //TINK_HANDLE_MANAGE_H
