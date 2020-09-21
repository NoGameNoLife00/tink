#include "context.h"
#include <module_manage.h>
#include <error_code.h>

namespace tink {
    int Context::Init(const std::string &name, const std::string &param) {
        auto mod = ModuleManage::GetInstance().Query(name);
        if (!mod) {
            return E_QUERY_MODULE;
        }
        this->mod = mod;
        cb = nullptr;
        cb_ud = nullptr;
        session_id = 0;
        init = false;
        endless = false;
        cpu_cost = 0;
        cpu_start = 0;
        profile = false;
        handle = 0;

        return 0;
    }

    void Context::Destory() {

    }
}

