#include <error_code.h>
#include <cassert>
#include <spdlog/spdlog.h>
#include <string>
#include <handle_manager.h>
#include <module_manager.h>

namespace tink {

    int HandleMgr::Init(int harbor) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handle_index_ = 1;
        // harbor取最高8位, 组网最多2^8=255个节点
        harbor_ = (harbor & 0xff) << HANDLE_REMOTE_SHIFT;
        return E_OK;
    }

    uint32_t HandleMgr::Register(ContextPtr ctx) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        uint32_t handle = handle_index_;
        assert(handle_map_.size() <= HANDLE_MASK);
        if (handle > HANDLE_MASK) {
            handle = 1;
        }
        for (int hash = handle; hash < HANDLE_MASK; hash++ ) {
            if (handle_map_.find(hash) == handle_map_.end()) {
                handle_map_.emplace(std::make_pair(handle, ctx));
                handle_index_ = hash + 1;
                handle |= harbor_;
                return handle;
            }
        }
        return 0;
    }

    int HandleMgr::Unregister(int handle) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        uint32_t hash = handle & HANDLE_MASK;
        auto it = handle_map_.find(hash);
        if (it == handle_map_.end()) {
            return E_FAILED;
        }
        auto ctx = it->second;
        if (ctx && (ctx->Handle() == handle)) {
            handle_map_.erase(hash);
            for (auto it = name_map_.begin(); it != name_map_.end(); it++) {
                if (it->second == hash) {
                    it = name_map_.erase(it);
                    break;
                }
            }
        }

        lock.unlock();
        if (ctx) {
            ctx->Destroy();
        }
        return E_OK;
    }

    void HandleMgr::UnregisterAll() {
        for (auto& it : handle_map_) {
            it.second->Destroy();
        }
        handle_map_.clear();
        name_map_.clear();
    }

    std::string HandleMgr::BindName(uint32_t handle, std::string_view name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (auto it = name_map_.find(name.data()); it != name_map_.end()) {
            return "";
        }
        auto ret = name_map_.emplace(std::make_pair(name, handle));
        return ret.first->first;
    }

    uint32_t HandleMgr::FindName(std::string_view name) {
        if (auto it = name_map_.find(name.data()); it != name_map_.end()) {
            return it->second;
        }
        return 0;
    }

    ContextPtr HandleMgr::HandleGrab(uint32_t handle) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        uint32_t hash = handle & (HANDLE_MASK);
        if (auto it = handle_map_.find(hash); it != handle_map_.end()) {
            return it->second;
        }
        return nullptr;
    }

    int HandleMgr::PushMessage(uint32_t handle, TinkMessage &msg) {
        ContextPtr ctx = HandleGrab(handle);
        if (!ctx) {
            return E_FAILED;
        }
        ctx->Queue()->Push(msg);
        return E_OK;
    }

    void HandleMgr::ContextEndless(uint32_t handle) {
        ContextPtr ctx = HandleGrab(handle);
        if (!ctx) {
            return ;
        }
        ctx->SetEndless(true);
    }

    uint32_t HandleMgr::QueryName(std::string_view name) {
        switch (name[0]) {
            case ':':
                return strtoul(name.data() + 1, nullptr, 16);
            case '.':
                return FindName( name.substr(1));
        }
        spdlog::error("don't support query global name %s");
        return 0;
    }

    ContextPtr HandleMgr::CreateContext(std::string_view name, std::string_view param) {
        ContextPtr ctx = std::make_shared<Context>();
        auto mod = MODULE_MNG.Query(name); // 获取对应的模块
        if (!mod) {
            return nullptr;
        }
        ctx->mod_ = mod;
        ctx->callback_ = nullptr;
        ctx->cb_ud_ = nullptr;
        ctx->session_id_ = 0;
        ctx->init_ = false;
        ctx->endless_ = false;
        ctx->cpu_cost_ = 0;
        ctx->cpu_start_ = 0;
        ctx->profile_ = false;
        ctx->message_count_ = 0;
        ctx->handle_ = HANDLE_STORAGE.Register(ctx); // 获取唯一标识
        if (ctx->handle_ == 0) {
            return nullptr;
        }
        ctx->queue_ = std::make_shared<MessageQueue>(ctx->handle_);
        ctx->mutex_.lock();
        int ret = mod->Init(ctx, param); // 模块初始化
        ctx->mutex_.unlock();
        if (ret == E_OK) {
            ctx->init_ = true;
            GLOBAL_MQ.Push(ctx->queue_);
        } else {
            spdlog::error("Failed launch {}", name);
            HANDLE_STORAGE.Unregister(ctx->handle_);
            struct DropT d = {ctx->handle_};
            ctx->queue_->Release(Context::DropMessage, &d);
        }
        return ctx;
    }
}

