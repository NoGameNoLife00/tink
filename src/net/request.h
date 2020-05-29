//
// Created by admin on 2020/5/26.
//

#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H


#include <irequest.h>

namespace tink {
    class Request : public IRequest {
    public:
        IConnection *conn;
        char* data;
        IConnection *GetConnection();

        char *GetData();
    };

}


#endif //TINK_REQUEST_H
