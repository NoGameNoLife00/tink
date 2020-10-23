#ifndef TINK_BASE_MODULE_H
#define TINK_BASE_MODULE_H

#include <common.h>
#include <functional>
namespace tink {
    class Context;
    class BaseModule {
    public:
        virtual int Init(std::shared_ptr<Context> ctx, std::string_view param) { return 0; };
        virtual void Release() { } ;
        virtual void Signal(int signal) {};
        virtual const std::string& Name() {return name_;}
    private:
        std::string name_;
    };

    typedef std::shared_ptr<BaseModule> ModulePtr;
    typedef BaseModule* (*ModuleCreateCallBack)(void);
}

#endif //TINK_BASE_MODULE_H
