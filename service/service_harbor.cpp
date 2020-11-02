#include <string_view>
#include <error_code.h>
#include <sstream>
#include <harbor.h>
#include <socket_server.h>
#include "service_harbor.h"

namespace tink::Service {




    ServiceHarbor::ServiceHarbor() {

    }

    int ServiceHarbor::Init(ContextPtr ctx, std::string_view param) {
        if (param.empty()) {
            return E_FAILED;
        }
        ctx_ = ctx;

        InitLog();
        std::istringstream is(param.data());
        is >> id_ >> slave_;
        if (slave_ == 0) {
            return E_FAILED;
        }
        ctx_->SetCallBack(MainLoop_, this);
        HARBOR.Start(ctx_);
        return 0;
    }

    int
    ServiceHarbor::MainLoop_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr &msg, size_t sz) {
        auto* h = static_cast<ServiceHarbor*>(ud);
        switch (type) {
        case PTYPE_SOCKET: {
            auto message = std::dynamic_pointer_cast<TinkSocketMessage>(msg);
            switch (message->type) {
                case TINK_SOCKET_TYPE_DATA:

            }
        }
        }
        return 0;
    }

    void ServiceHarbor::PushSocketData_(TinkSocketMsgPtr message) {
        assert(message->type == TINK_SOCKET_TYPE_DATA);
        int fd = message->id;
        int id;
        Slave *s = nullptr;
        for (int i = 1; i < s_.max_size(); i++) {
            if (s_[i].fd == fd) {
                s = &s_[i];
                id = i;
            }
        }
        if (!s) {
            logger->error("invalid socket fd ({}) data", fd);
            return;
        }

//        BytePtr buffer = std::dynamic_pointer_cast<byte[]>(message->buffer) ;
        uint8_t *buffer = static_cast<uint8_t *>(message->buffer.get());
        int size = message->ud;
        for (;;) {
            switch (s->status) {
                case STATUS_HANDSHAKE: {
                    // check id
                    uint8_t remote_id = buffer[0];
                    if (remote_id != id) {
                        logger->error("invalid shakehand id ({}) from fd = {} , harbor = {}", id, fd, remote_id);
                        CloseSlave_(id);
                        return;
                    }
                    ++buffer;
                    --size;
                    s->status = STATUS_HEADER;

                }

            }
        }
    }

    void ServiceHarbor::CloseSlave_(int id) {
        Slave& s = GetSlave(id);
        s.status = STATUS_DOWN;
        if (s.fd) {
            SOCKET_SERVER.Close(ctx_->Handle(), id);
        }
        if (s.queue) {
            s.queue->clear();
            s.queue.reset();
        }
    }

    ServiceHarbor::Slave &ServiceHarbor::GetSlave(int id) {
        return s_[id];
    }

    void ServiceHarbor::DispatchQueue_(int id) {
        Slave& s = GetSlave(id);
        int fd = s.fd;
        assert(fd != 0);
        auto queue = s.queue;
        if (!queue) {
            return;
        }
        HarborMsgPtr m;
        while ((m = queue->front()) != nullptr) {

        }
    }

    void ServiceHarbor::SendRemote_(int fd, BytePtr buffer, size_t sz, ServiceHarbor::RemoteMsgHeader &cookie) {
        size_t sz_header = sz + sizeof(cookie);
        if (sz_header > UINT32_MAX) {
            logger->error("remote message from {0:08x} to :{0:08x} is too large.", cookie.source, cookie.destination);
            return;
        }
        
    }

}
