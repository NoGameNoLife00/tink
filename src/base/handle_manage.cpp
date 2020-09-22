#include <error_code.h>
#include <cassert>
#include "handle_manage.h"
namespace tink {
    int HandleManage::Init(int harbor) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handle_index_ = 1;
        harbor_ = (harbor & 0xff) << HANDLE_REMOTE_SHIFT;
    }

    uint32_t HandleManage::Register(ContextPtr ctx) {
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

    int HandleManage::Unregister(int handle) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        uint32_t hash = handle & HANDLE_MASK;
        auto it = handle_map_.find(hash);
        if (it == handle_map_.end()) {
            return E_FAILED;
        }
        auto ctx = it->second;
        if (ctx && ctx->handle == handle) {
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
            ctx->Destory();
        }
        return E_OK;
    }

    void HandleManage::UnregisterAll() {
        for (auto& it : handle_map_) {
            it.second->Destory();
        }
        handle_map_.clear();
        name_map_.clear();
    }

    int HandleManage::BindName(uint32_t handle, std::string &name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        // todo 需要优化效率
        for (auto it : name_map_) {
            if (it.first == name) {
                return E_FAILED;
            }
        }
        name_map_.emplace(std::make_pair(name, handle));
        return E_OK;
    }

    uint32_t HandleManage::FindName(const std::string & name) {
        if (auto it = name_map_.find(name); it != name_map_.end()) {
            return it->second;
        }
        return 0;
    }

    ContextPtr HandleManage::HandleGrab(uint32_t handle) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        uint32_t hash = handle & (HANDLE_MASK);
        if (auto it = handle_map_.find(hash); it != handle_map_.end()) {
            return it->second;
        }
        return nullptr;
    }
}

