
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <context_manage.h>

#include "service_gate.h"


namespace tink::Service {
    ServiceGate::ServiceGate() : listen_id(-1) {

    }

    int ServiceGate::Init(ContextPtr ctx, const std::string &param) {
        if (param.empty()) {
            return E_FAILED;
        }
        char header;
        char watchdog[param.size()];
        char binding[param.size()];
        int client_tag = 0, max = 0;
        int n = sscanf(param.c_str(), "%c %s %s %d %d", &header, watchdog, binding, &client_tag, &max);
        if (n < 4){
            spdlog::error("invalid gate param {}", n);
            return E_FAILED;
        }
        if (max <= 0) {
            spdlog::error("need max connection");
            return E_FAILED;
        }
        if (client_tag == 0) {
            client_tag = PTYPE_CLIENT;
        }
        if (watchdog[0] = '!') {
            this->watchdog = 0;
        } else {
            this->watchdog = ContextMngInstance.FindName(watchdog);
            if (this->watchdog == 0) {
                spdlog::error("Invalid watchdog {}", static_cast<char*>(watchdog));
                return E_FAILED;
            }
        }
        this->ctx = ctx;
        for (int i = 0 ; i < max; i++) {
            ConnectionPtr cn = std::make_shared<Connection>();
            cn->id = -1;
            conn.emplace_back(cn);
        }
        max_connection = max;

        this->client_tag = client_tag;
        this->header_size = header == 'S' ? 2 : 4;
        ctx->SetCallBack(CallBack, this);
        return 0;
    }

    void ServiceGate::Release() {

    }
}
