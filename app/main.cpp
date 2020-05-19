#include <iostream>
#include <server.h>
int main() {
    server* s  = new_server();
    s->run();
    return 0;
}
