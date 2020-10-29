#ifndef TINK_BASE_MODULE_H
#define TINK_BASE_MODULE_H

#include <common.h>
#include <functional>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>
#include <context.h>
#include <spdlog/spdlog.h>

namespace tink {
    class Context;
    typedef std::shared_ptr<spdlog::logger> LoggerPtr;
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
        LoggerPtr logger;
    protected:
        std::string name_;

    };

    typedef std::shared_ptr<BaseModule> ModulePtr;
    typedef BaseModule* (*ModuleCreateCallBack)(void);
}

#endif //TINK_BASE_MODULE_H
