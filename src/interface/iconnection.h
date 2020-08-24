//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <sys/socket.h>
#include <memory>
#include <type.h>
#include <imessage_handler.h>
#include <mutex>
#include <buffer.h>

namespace tink {

    class IConnection {
    public:
        // 启动链接
        virtual int Start() = 0;
        // 停止链接
        virtual int Stop() = 0;

        // 获取链接id
        virtual int GetConnId() {
            return 0;
        };
        // 获取客户端的tcp状态 ip port
        virtual RemoteAddrPtr GetRemoteAddr() = 0;
        // 发送msg包到客户端
        virtual int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len) {
            return 0;
        };
        // 获取链接的socket
        virtual int GetTcpConn() {
            return 0;
        };

        virtual FixBufferPtr& GetBuffer() = 0;

//        virtual uint32_t GetBufferLen() {return 0;};
//
//        virtual uint32_t GetBuffOffset() {return 0;};

        virtual void SetBuffOffset(uint32_t offset) {};

        virtual const IMessageHandlerPtr &GetMsgHandler() = 0;

        virtual Mutex& GetMutex() = 0;
        virtual ~IConnection(){};
    };
    typedef std::shared_ptr<IConnection> IConnectionPtr;

}


#endif //TINK_INTERFACE_CONN_H
