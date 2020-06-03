//
// Created by admin on 2020/5/28.
//

#include "global_mng.h"
#include <stdio.h>
#include <cJSON.h>
#include <sys/unistd.h>

namespace tink {


    GlobalMng::GlobalMng() {
//        name_ = std::shared_ptr<std::string>(new std::string("tink_server"));
//        host_ = std::shared_ptr<std::string>(new std::string("0.0.0.0"));
//        version_ = std::shared_ptr<std::string>(new std::string("v0.5"));
        name_ = std::make_shared<std::string>("tink_server");
        host_ = std::make_shared<std::string>("0.0.0.0");
        version_ = std::make_shared<std::string>("v0.5");
        port_ = 8896;
        max_conn_ = 10000;
        max_package_size_ = 2048;
    }

    int GlobalMng::Init() {
        FILE *fp = nullptr;
        cJSON *json;
        char *out;
        char line[READ_BUF_SIZE] = {0};
        fp = fopen("etc/config.json", "r");
        if (fp != nullptr) {
            fseek(fp, 0, SEEK_END);
            uint f_size = ftell(fp);
            char* buff = new char[f_size];
            rewind(fp);
            fread(buff, sizeof(char), f_size, fp);
            buff[f_size] = '\0';
            json = cJSON_Parse(buff);
            cJSON *item = cJSON_GetObjectItem(json, "name");
            if (item != nullptr) {
                name_->clear();
                name_->append(item->valuestring);
//                name_ = std::shared_ptr<std::string>(new std::string(item->valuestring));
            }

            item = cJSON_GetObjectItem(json, "host");
            if (item != nullptr) {
                host_->clear();
                host_->append(item->valuestring);
//                host_ = std::shared_ptr<std::string>(new std::string(item->valuestring));
            }
            item = cJSON_GetObjectItem(json, "port");
            if (item != nullptr) {
                port_ = item->valueint;
            }
            item = cJSON_GetObjectItem(json, "max_connection");
            if (item != nullptr) {
                max_conn_ = item->valueint;
            }
            delete [] buff;
        } else {
            exit(0);
        }
        cJSON_Delete(json);
        fclose(fp);
        return 0;
    }

    const std::shared_ptr<IServer> &GlobalMng::GetServer() const {
        return server_;
    }

    const std::shared_ptr<std::string> &GlobalMng::GetHost() const {
        return host_;
    }

    const std::shared_ptr<std::string> &GlobalMng::GetName() const {
        return name_;
    }

    int GlobalMng::getPort() const {
        return port_;
    }

    const std::shared_ptr<std::string> &GlobalMng::GetVersion() const {
        return version_;
    }

    int GlobalMng::GetMaxConn() const {
        return max_conn_;
    }

    uint GlobalMng::GetMaxPackageSize() const {
        return max_package_size_;
    }
}

