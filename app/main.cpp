#include <iostream>
#include <server.h>
#include <cygwin/socket.h>
#include <base_router.h>

int main() {
    Server *s = new Server();
    BaseRouter *br = new BaseRouter();
    s->Init("tink", AF_INET, "0.0.0.0", 8823);
    s->AddRouter(br);
    s->Run();
    delete  s;
    delete br;
    return 0;
}
