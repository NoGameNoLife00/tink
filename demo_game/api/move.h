//
// Created by Ёблн on 2020/9/10.
//

#ifndef TINK_MOVE_H
#define TINK_MOVE_H

#include <base_router.h>

namespace api {
    class Move : public tink::BaseRouter {
    public:
        int Handle(tink::IRequest &request) override;
    };
}




#endif //TINK_MOVE_H
