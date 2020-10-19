#include <server.h>
#include <config_mng.h>

int main(int argc, char** argv) {
    setbuf(stdout, NULL); // debug
    srand(static_cast<unsigned>(time(NULL)));
    std::shared_ptr<tink::Config> conf = std::make_shared<tink::Config>();
    conf->Init();
    TINK_SERVER.Init(conf);
    TINK_SERVER.Start();
    return 0;
}
