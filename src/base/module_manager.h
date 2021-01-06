#ifndef TINK_MODULE_MANAGER_H
#define TINK_MODULE_MANAGER_H

#include <base_module.h>
#include <singleton.h>
#define MODULE_MNG tink::Singleton<tink::ModuleMgr>::GetInstance()
namespace tink {
    class ModuleMgr {
    public:
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
