#include "global_mng.h"
#include "scope_guard.h"
#include <stdio.h>
#include <cJSON.h>
#include <sys/unistd.h>
INITIALIZE_EASYLOGGINGPP
namespace tink {
    el::Logger* logger = el::Loggers::getLogger("default");

    GlobalMng::GlobalMng() {
        name_ = std::make_shared<std::string>("tink_server");
        host_ = std::make_shared<std::string>("0.0.0.0");
        version_ = std::make_shared<std::string>("v0.6");
        port_ = 8896;
        max_conn_ = 10000;
        max_package_size_ = 2048;
        worker_pool_size_ = 10;
        max_worker_task_len_ = 1024;
    }

    int GlobalMng::Init() {
        el::Configurations conf("../etc/log.conf");
        el::Loggers::reconfigureAllLoggers(conf);
        FILE *fp = nullptr;
        cJSON *json;
        char *out;
        char line[READ_BUF_SIZE] = {0};
        fp = fopen("../etc/config.json", "r");
        ON_SCOPE_EXIT([&] {
            cJSON_Delete(json);
            fclose(fp);
        });
        if (fp != nullptr) {
            fseek(fp, 0, SEEK_END);
            uint32_t f_size = ftell(fp);
            char* buff = new char[f_size];
            rewind(fp);
            fread(buff, sizeof(char), f_size, fp);
            buff[f_size] = '\0';
            json = cJSON_Parse(buff);
            cJSON *item = cJSON_GetObjectItem(json, "name");
            if (item != nullptr) {
                name_->clear();
                name_->append(item->valuestring);
            }

            item = cJSON_GetObjectItem(json, "host");
            if (item != nullptr) {
                host_->clear();
                host_->append(item->valuestring);
            }
            item = cJSON_GetObjectItem(json, "port");
            if (item != nullptr) {
                port_ = item->valueint;
            }
            item = cJSON_GetObjectItem(json, "max_connection");
            if (item != nullptr) {
                max_conn_ = item->valueint;
            }
            item = cJSON_GetObjectItem(json, "worker_pool_size");
            if (item != nullptr) {
                worker_pool_size_ = item->valueint;
            }
            delete [] buff;
        } else {
            logger->info("tink open file etc/config.json failed");
            exit(0);
        }
        return 0;
    }

    const std::shared_ptr<IServer>& GlobalMng::GetServer() const {
        return server_;
    }

    const StringPtr& GlobalMng::GetHost() const {
        return host_;
    }

    const StringPtr& GlobalMng::GetName() const {
        return name_;
    }

    int GlobalMng::getPort() const {
        return port_;
    }

    const StringPtr& GlobalMng::GetVersion() const {
        return version_;
    }

    int GlobalMng::GetMaxConn() const {
        return max_conn_;
    }

    uint32_t GlobalMng::GetMaxPackageSize() const {
        return max_package_size_;
    }

    uint32_t GlobalMng::GetWorkerPoolSize() const {
        return worker_pool_size_;
    }

}

