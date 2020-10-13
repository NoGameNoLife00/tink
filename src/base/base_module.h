#ifndef TINK_BASE_MODULE_H
#define TINK_BASE_MODULE_H

#include <common.h>

namespace tink {
    class Context;
    class BaseModule {
    public:
        virtual int Init(std::shared_ptr<Context> ctx, const string &param) = 0;
        virtual void Release() = 0;
        virtual void Signal(int signal) = 0;
        virtual const std::string& Name() {return name_;}
    private:
        std::string name_;
    };

    typedef std::shared_ptr<BaseModule> ModulePtr;
    typedef std::function<BaseModule*(void)>  ModuleCreateInstance;
}

#endif //TINK_BASE_MODULE_H
