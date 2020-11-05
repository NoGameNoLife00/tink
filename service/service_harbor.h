#ifndef TINK_SERVICE_HARBOR_H
#define TINK_SERVICE_HARBOR_H

#include <base_module.h>
#include <string_view>
#include <harbor.h>
#include <string>


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

        typedef struct RemoteMsgHeader_ {
            uint32_t source;
            uint32_t destination;
            uint32_t session;
        } RemoteMsgHeader;

        typedef std::shared_ptr<RemoteMsgHeader> RemoteMsgHeaderPtr;
        typedef struct HarborMsg_ {
            RemoteMsgHeader header;
            DataPtr buffer;
            size_t size;
        } HarborMsg;
        typedef std::shared_ptr<HarborMsg> HarborMsgPtr;
        typedef std::list<HarborMsg> HarborMsgQueue;
        typedef std::shared_ptr<HarborMsgQueue> HarborMsgQueuePtr;
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
        typedef std::pair<int32_t, HarborMsgQueuePtr> HarborValue;
        typedef std::unordered_map<std::string, HarborValue> HarborMap;
        static int MainLoop_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        void CloseSlave_(int id);
        void DispatchQueue_(int id);
        void SendRemote_(int fd, const BytePtr& buffer, size_t sz, RemoteMsgHeader& cookie);
        void UpdateName_(const std::string& name, uint32_t handle);
        void DispatchNameQueue_(HarborMap::iterator &node);
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
