#include <cstring>
#include <dlfcn.h>
#include <error_code.h>
#include "module_manage.h"
#include "string_util.h"

namespace tink {
    static void *TryOpen(const std::string& path_str, const std::string& name_str) {
        const char *l;
        size_t path_size = path_str.length();
        size_t name_size = name_str.length();

        int sz = path_size + name_size;
        //search path
        void * dl = NULL;
        char tmp[sz];
        const char *path = path_str.c_str();
        const char *name = name_str.c_str();
        do
        {
            memset(tmp,0,sz);
            while (*path == ';') path++;
            if (*path == '\0') break;
            l = strchr(path, ';');
            if (l == NULL) l = path + strlen(path);
            int len = l - path;
            int i;
            for (i=0;path[i]!='?' && i < len ;i++) {
                tmp[i] = path[i];
            }
            memcpy(tmp+i,name,name_size);
            if (path[i] == '?') {
                strncpy(tmp+i+name_size,path+i+1,len - i - 1);
            } else {
                fprintf(stderr,"Invalid C service path\n");
                exit(1);
            }
            dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL);
            path = l;
        }while(dl == NULL);

        if (dl == NULL) {
            fprintf(stderr, "try open %s failed : %s\n",name,dlerror());
        }

        return dl;
    }

    ModulePtr ModuleManage::Query(const std::string &name) {
        ModulePtr result = Query_(name);
        if (result) {
            return result;
        }
        std::lock_guard<Mutex> guard(mutex_);
        result = Query_(name);
        if (!result && m_.size() > MAX_MODULE_TYPE) {
            void * dl = TryOpen(path_, name);
            if (dl) {
                auto* create = reinterpret_cast<ModuleCreateInstance*>(dlsym(dl, "CreateModule"));
                const char * error = dlerror();
                if (error) {
                    fprintf(stderr, "load module instance %s failed : %s\n", name.c_str(), error);
                    return result;
                }
                result = m_.emplace_back(std::shared_ptr<BaseModule>(create()));
            }
        }
        return result;
    }

    ModulePtr ModuleManage::Query_(const string &name) {
        for (auto &it : m_) {
            if (strcmp(it->Name().c_str(), name.c_str())) {
                return it;
            }
        }
        return nullptr;
    }

    int ModuleManage::Init(const std::string& path) {
        path_ = path;
        return E_OK;
    }

}

