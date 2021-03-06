#ifndef TINK_BASE_MODULE_H
#define TINK_BASE_MODULE_H


#include <functional>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/async_logger.h"
#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "base/context.h"
#include "common.h"

namespace tink {
    class Context;
    class BaseModule {
    public:
        BaseModule() : name_("default") {
            logger = spdlog::default_logger();
        }
        virtual int Init(std::shared_ptr<Context> ctx, std::string_view param) {
            return 0;
        };
        virtual void Release() { } ;
        virtual void Signal(int signal) {};
        virtual const std::string& Name() {return name_;}
        virtual void InitLog() {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(Name(), 0, 0);
            logger = std::make_shared<spdlog::async_logger>(Name(), spdlog::sinks_init_list{file_sink, console_sink}, spdlog::thread_pool());
            logger->set_pattern("[%Y-%m-%d.%e %T] [t-%t] [%l] %v");
            logger->flush_on(spdlog::level::debug);
        }
        std::shared_ptr<spdlog::logger> logger;
    protected:
        std::string name_;

    };

    using ModulePtr = std::shared_ptr<BaseModule>;
}

#endif //TINK_BASE_MODULE_H
