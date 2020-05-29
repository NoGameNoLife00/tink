
#include <request.h>
namespace tink {
    IConnection *Request::GetConnection() {
        return conn;
    }

    char *Request::GetData() {
        return data;
    }
}

