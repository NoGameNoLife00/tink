
#include <request.h>
#include "request.h"

namespace tink {
    Request::Request(IConnection &conn, std::shared_ptr<byte> data) : conn(conn) {
        this->data = data;
    }

    IConnection & Request::GetConnection() {
        return conn;
    }

    std::shared_ptr<byte> Request::GetData() {
        return data;
    }
}

