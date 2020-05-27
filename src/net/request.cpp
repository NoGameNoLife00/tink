
#include <request.h>

IConnection *Request::GetConnection() {
    return conn;
}

char *Request::GetData() {
    return data;
}
