#ifndef TINK_MODULE_MANAGE_H
#define TINK_MODULE_MANAGE_H

#include <base_module.h>
#include <singleton.h>
#define MODULE_MNG tink::Singleton<tink::ModuleManage>::GetInstance()
namespace tink {
    class ModuleManage {
    public:
        constexpr static int MAX_MODULE_TYPE = 64;
        int Init(const std::string& path);
        ModulePtr Query(const std::string& name);
    private:
        ModulePtr Query_(const std::string& name);
        std::vector<ModulePtr> m_;
        std::string path_;
        mutable Mutex mutex_;
    };
}




#endif //TINK_MODULE_MANAGE_H
