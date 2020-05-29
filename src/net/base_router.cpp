//
// Created by admin on 2020/5/27.
//

#include "base_router.h"
namespace tink {
    int BaseRouter::PreHandle(IRequest &request) {
        return 0;
    }

    int BaseRouter::Handle(IRequest &request) {
        return 0;
    }

    int BaseRouter::PostHandle(IRequest &request) {
        return 0;
    }
}

