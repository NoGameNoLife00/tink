#ifndef TINK_SERVICE_DUMMY_H
#define TINK_SERVICE_DUMMY_H

#include "base/base_module.h"
#include "harbor_message.h"

namespace tink::Service {
    class ServiceDummy : public BaseModule {
    public:
//        struct Msg {
//            std::shared_ptr<uint8_t[]> buffer;
//            size_t size;
//        };
//
//        typedef std::list<Msg> MsgQueue;
//        typedef std::shared_ptr<MsgQueue> MsgQueuePtr;
//        typedef std::pair<uint32_t, MsgQueuePtr> QueueBind;
//        typedef std::unordered_map<std::string, QueueBind> QueueMap;

        int Init(ContextPtr ctx, std::string_view param) override;

        void Release() override;
    private:
        static int MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);

        void UpdateName_(const std::string &name, uint32_t handle);
        void SendName_(uint32_t source, const std::string &name, int type, int session, DataPtr msg, size_t sz);
        void DispatchQueue_(HarborMap::iterator &node);
        void PushQueue_(HarborMsgQueue& queue, DataPtr buffer, size_t sz, RemoteMsgHeader& header);
        HarborMap map_;

        ContextPtr ctx_;
    };
}




#endif //TINK_SERVICE_DUMMY_H
