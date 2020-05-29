//
// Created by admin on 2020/5/26.
//

#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H


#include <irequest.h>

namespace tink {
    class Request : public IRequest {
    private:
    public:
        Request(IConnection &, std::shared_ptr<byte>);
        IConnection& conn;
        std::shared_ptr<byte> data;
        IConnection & GetConnection();
        std::shared_ptr<byte> GetData();
    };

}


#endif //TINK_REQUEST_H
