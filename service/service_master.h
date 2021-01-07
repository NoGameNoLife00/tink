#ifndef TINK_SERVICE_MASTER_H
#define TINK_SERVICE_MASTER_H

#include "base/base_module.h"
#include "net/socket.h"

namespace tink::Service {

    class ServiceMaster : public BaseModule {
    public:

        ServiceMaster();

        int Init(ContextPtr ctx, std::string_view param) override;

        void Release() override;

        void Signal(int signal) override;

        void DispatchSocket(TinkSocketMsgPtr msg, int sz);

        int SocketId(int id);

    private:
        static constexpr int REMOTE_MAX = 256;
        static int MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        void OnConnected_(int id);
        void Broadcast(const byte *name, size_t sz, uint32_t handle);
        void SendTo_(int id, const void *buf, int sz, uint32_t handle);
        void CloseHarbor_(int harbor_id);
        void RequestName_(const std::string& name);
        void UpdateName_(uint32_t handle, const std::string &name);
        void UpdateAddress_(int harbor_id, const std::string &addr);
        void ConnectTo_(int id);
        std::array<int, REMOTE_MAX> remote_fd_;
        std::array<bool, REMOTE_MAX> connected_;
        std::array<StringPtr, REMOTE_MAX> remote_addr_;

        std::unordered_map<std::string, uint32_t> name_map_;
        ContextPtr ctx_;

    };
}




#endif //TINK_SERVICE_MASTER_H
