#include <server.h>
#include <config.h>

int main(int argc, char** argv) {
    setbuf(stdout, nullptr); // debug
    const char * config_file = nullptr;
    if (argc > 1) {
        config_file = argv[1];
    } else {
        fprintf(stderr, "Need a config file.");
        return 1;
    }
    std::shared_ptr<tink::Config> conf = std::make_shared<tink::Config>();
    conf->Init(config_file);
    printf("tink start");
    tink::GetGlobalServer().reset(new tink::Server());
    tink::GetGlobalServer()->Init(conf);
    tink::GetGlobalServer()->Start();
    return 0;
}
