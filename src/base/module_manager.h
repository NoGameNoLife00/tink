#ifndef TINK_MODULE_MANAGER_H
#define TINK_MODULE_MANAGER_H

#include "base/base_module.h"
#include "base/singleton.h"

namespace tink {
    class BaseModule;
    class ModuleMgr {
    public:
        typedef BaseModule* (*ModuleCreateCallBack)(void);
        using ModulePtr = std::shared_ptr<BaseModule>;

        constexpr static int MAX_MODULE_TYPE = 64;
        int Init(std::string_view path);
        ModulePtr Query(std::string_view name);
    private:
        ModulePtr Query_(std::string_view name);
        std::vector<ModulePtr> m_;
        std::string path_; // .so¿âµÄÂ·¾¶
        mutable Mutex mutex_;
    };
}




#endif //TINK_MODULE_MANAGER_H
