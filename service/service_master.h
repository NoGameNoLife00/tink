#ifndef TINK_SERVICE_MASTER_H
#define TINK_SERVICE_MASTER_H

#include <base_module.h>
#include <socket.h>

#define REMOTE_MAX 256


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

        static int MainLoop_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);

        void OnConnected_(int id);
        void Broadcast(const byte *name, size_t sz, uint32_t handle);
        void SendTo_(int id, const void *buf, int sz, uint32_t handle);
        void CloseHarbor_(int harbor_id);
        void RequestName_(const std::string& name);
        void UpdateName_(uint32_t handle, const std::string &name);
        void UpdateAddress_(int harbor_id, const std::string &addr);
        void ConnectTo_(int id);
        std::array<int, REMOTE_MAX> remote_fd;
        std::array<bool, REMOTE_MAX> connected;
        std::array<StringPtr, REMOTE_MAX> remote_addr;

        std::unordered_map<std::string, uint32_t> name_map;
        ContextPtr ctx_;

    };
}




#endif //TINK_SERVICE_MASTER_H
