#include <config_mng.h>
#include <scope_guard.h>
#include <string_util.h>
#include <cJSON.h>
#include <version.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>
#include <common.h>

namespace tink {

    void Config::Default_() {
        name_ = "tink_server";
        host_ = "0.0.0.0";
        version_ = TINK_VERSION_STR;
        port_ = 8896;
        max_conn_ = 10000;
        max_package_size_ = 2048;
        worker_pool_size_ = 10;
        max_worker_task_len_ = 1024;
        log_name_ = "./logs/tink.log";
    }

    void GetJsonValue(const cJSON *json, int& val, std::string_view key, int default_val = 0) {
        cJSON *item = cJSON_GetObjectItem(json, key.data());
        if (item != nullptr) {
            val = item->valueint;
        } else {
            val = default_val;
        }
    }

    void GetJsonValue(cJSON *json, string& val, std::string_view key, const string& default_val = "") {
        cJSON *item = cJSON_GetObjectItem(json, key.data());
        if (item != nullptr) {
            val = item->valuestring;
        } else {
            val = default_val;
        }
    }

    void GetJsonValue(cJSON *json, bool& val, std::string_view key, bool default_val = false) {
        cJSON *item = cJSON_GetObjectItem(json, key.data());
        if (item != nullptr) {
            val = (item->valueint == 1);
        } else {
            val = default_val;
        }
    }

    // init log
    void Config::InitLog_() {
        spdlog::init_thread_pool(8192, 1);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_name_, 0, 0);
        auto logger = std::make_shared<spdlog::async_logger>("tink", spdlog::sinks_init_list{file_sink, console_sink}, spdlog::thread_pool());
        logger->set_pattern("[%Y-%m-%d.%e %T] [t-%t] [%l] %v");
        logger->flush_on(spdlog::level::debug);
        spdlog::set_default_logger(logger);
    }

    int Config::Init(std::string_view path) {
        Default_();
        FILE *fp = nullptr;
        cJSON *json;
        fp = fopen(path.data(), "r");
        ON_SCOPE_EXIT([&] {
            cJSON_Delete(json);
            fclose(fp);
        });
        if (fp != nullptr) {
            fseek(fp, 0, SEEK_END);
            uint32_t f_size = ftell(fp);
            UBytePtr buff = std::make_unique<byte[]>(f_size + 1);
            rewind(fp);
            fread(buff.get(), sizeof(byte), f_size, fp);
            buff[f_size] = '\0';
            json = cJSON_Parse(buff.get());
            GetJsonValue(json, name_, "name", "tink_server");
            GetJsonValue(json, host_, "host", "0.0.0.0");
            GetJsonValue(json, port_, "port", 8896);
            GetJsonValue(json, max_conn_, "max_connection", 1000);
            GetJsonValue(json, reinterpret_cast<int &>(worker_pool_size_), "worker_pool_size", 8);
            GetJsonValue(json, log_name_, "log_name", "./logs/tink.log");
            GetJsonValue(json, daemon_, "daemon", "");
            GetJsonValue(json, profile_, "profile", false);
            GetJsonValue(json, module_path_, "module_path", "./cservice/?.so");
            GetJsonValue(json, bootstrap_, "bootstrap", "lua bootstrap");
            InitLog_();
        } else {
            fprintf(stderr,"tink open file etc/config.json failed");
            exit(0);
        }
        return 0;
    }

    const string& Config::GetHost() const {
        return host_;
    }

    const string& Config::GetName() const {
        return name_;
    }

    int Config::GetPort() const {
        return port_;
    }

    const string& Config::GetVersion() const {
        return version_;
    }

    int Config::GetMaxConn() const {
        return max_conn_;
    }

    uint32_t Config::GetMaxPackageSize() const {
        return max_package_size_;
    }

    uint32_t Config::GetWorkerPoolSize() const {
        return worker_pool_size_;
    }




}

