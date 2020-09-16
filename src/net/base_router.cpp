#include <base_router.h>
namespace tink {
    int BaseRouter::PreHandle(Request &request) {
        return 0;
    }

    int BaseRouter::Handle(Request &request) {
        return 0;
    }

    int BaseRouter::PostHandle(Request &request) {
        return 0;
    }
}

