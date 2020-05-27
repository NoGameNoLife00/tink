//
// Created by admin on 2020/5/26.
//

#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H


#include <irequest.h>

class Request : public IRequest {
public:
    IConnection *conn;
    char* data;
    IConnection *GetConnection() override;

    char *GetData() override;



};


#endif //TINK_REQUEST_H
