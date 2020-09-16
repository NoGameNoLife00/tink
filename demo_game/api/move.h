#pragma once
#include <base_router.h>

namespace api {
    class Move : public tink::BaseRouter {
    public:
        int Handle(tink::Request &request) override;
    };
}
