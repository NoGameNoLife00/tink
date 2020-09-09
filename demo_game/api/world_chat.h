#pragma once
#include <base_router.h>
namespace api {
class WorldChat : public tink::BaseRouter {
    int Handle(tink::IRequest &request);
};
}

