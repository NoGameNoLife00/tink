#ifndef TINK_SERVICE_HARBOR_H
#define TINK_SERVICE_HARBOR_H

#include <base_module.h>
#include <string_view>
#include <harbor.h>

namespace tink::Service {

    class ServiceHarbor : public BaseModule {
    public:
        static constexpr int STATUS_WAIT = 0;
        static constexpr int STATUS_HANDSHAKE = 1;
        static constexpr int STATUS_HEADER = 2;
        static constexpr int STATUS_CONTENT = 3;
        static constexpr int STATUS_DOWN = 4;

        static constexpr int REMOTE_MAX = 256;

        typedef struct RemoteMsgHeader_ {
            uint32_t source;
            uint32_t destination;
            uint32_t session;
        } RemoteMsgHeader;

        typedef struct HarborMsg_ {
            RemoteMsgHeader header;
            DataPtr buffer;
            size_t size;
        } HarborMsg;
        typedef std::shared_ptr<HarborMsg> HarborMsgPtr;
        typedef std::list<HarborMsgPtr> HarborMsgQueue;

        typedef struct Slave_ {
            int fd;
            std::shared_ptr<HarborMsgQueue> queue;
            int status;
            int length;
            int read;
            uint8_t size[4];
            DataPtr RecvBuff;
        } Slave;

        ServiceHarbor();
        int Init(ContextPtr ctx, std::string_view param) override;

    private:
        static int MainLoop_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz);
        void PushSocketData_(TinkSocketMsgPtr message);
        void CloseSlave_(int id);
        void DispatchQueue_(int id);
        void SendRemote_(int fd, BytePtr buffer, size_t sz, RemoteMsgHeader& cookie);
        Slave & GetSlave(int id);
        ContextPtr ctx_;
        int id_;
        uint32_t slave_;
        std::unordered_map<uint32_t, uint32_t> map;
        std::array<Slave, REMOTE_MAX> s_;
    };
}




#endif //TINK_SERVICE_HARBOR_H
