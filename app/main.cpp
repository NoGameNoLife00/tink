#include <iostream>
#include <server.h>
#include <cygwin/socket.h>

int main() {
    server *s = new server();
    s->init("tink", AF_INET, "0.0.0.0", 8823);
    s->run();
    return 0;
}
