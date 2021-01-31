#ifndef TINK_LUA_ENV_H
#define TINK_LUA_ENV_H

#include <lua.h>
#include "common.h"
namespace tink {
    class LuaEnv {
    public:
        void Init();
        std::string GetEnv(std::string_view key);
        void SetEnv(std::string_view key, std::string_view value);

    private:
        mutable Mutex mutex_;
        lua_State * L_;
    };
}

#endif //TINK_LUA_ENV_H
