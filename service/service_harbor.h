#ifndef TINK_SERVICE_HARBOR_H
#define TINK_SERVICE_HARBOR_H

#include <string_view>
#include <string>

#include "net/harbor.h"
#include "net/server.h"
#include "base/base_module.h"
#include "harbor_message.h"

namespace tink::Service {

    class ServiceHarbor : public BaseModule {
    public:
        static constexpr int STATUS_WAIT = 0;
        static constexpr int STATUS_HANDSHAKE = 1;
        static constexpr int STATUS_HEADER = 2;
        static constexpr int STATUS_CONTENT = 3;
        static constexpr int STATUS_DOWN = 4;

        static constexpr int REMOTE_MAX = 256;
        static constexpr int HEADER_COOKIE_LENGTH = 12;

        typedef struct Slave_ : public noncopyable {
            int fd;
            std::shared_ptr<HarborMsgQueue> queue;
            int status;
            int length;
            int read;
            uint8_t size[4];
            BytePtr recv_buffer;
        } Slave;

        ServiceHarbor();
        Slave & GetSlave(int id);
        int Init(ContextPtr ctx, std::string_view param) override;
        void PushSocketData(TinkSocketMsgPtr message);
        int GetHarborId(int fd);
        void ReportHarborDown(int id);
        void HarborCommand(const char *msg, size_t sz, int session, uint32_t source);
        int RemoteSendName(uint32_t source, const std::string& name, int type, int session, DataPtr msg, size_t sz);
        int RemoteSendHandle(uint32_t source, uint32_t destination, int type, int session, DataPtr msg, size_t sz);
    private:

        static int MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        void CloseSlave_(int id);
        void DispatchQueue_(int id);
        void DispatchNameQueue_(HarborMap::iterator &node);
        void SendRemote_(int fd, const BytePtr& buffer, size_t sz, RemoteMsgHeader& cookie);
        void UpdateName_(const std::string& name, uint32_t handle);
        void Handshake_(int id);
        void ForwardLocalMessage(DataPtr msg, int sz);
        void PushQueue_(HarborMsgQueue& queue, DataPtr buffer, size_t sz, RemoteMsgHeader& header);
        ContextPtr ctx_;
        int id_;
        uint32_t slave_;
        HarborMap map_;
        std::array<Slave, REMOTE_MAX> s_;
    };
}




#endif //TINK_SERVICE_HARBOR_H
