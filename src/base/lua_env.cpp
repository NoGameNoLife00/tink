#include <lauxlib.h>
#include <lua.h>
#include <cassert>

#include "lua_env.h"

namespace tink {

    void LuaEnv::Init() {
        std::scoped_lock<Mutex> lock(mutex_);
        L_ = luaL_newstate();
    }

    std::string LuaEnv::GetEnv(std::string_view key) {
        std::scoped_lock<Mutex> lock(mutex_);

        lua_State *L = L_;

        lua_getglobal(L, key.data());
        const char * result = lua_tostring(L, -1);
        lua_pop(L, 1);
        return result;
    }

    void LuaEnv::SetEnv(std::string_view key, std::string_view value) {
        std::scoped_lock<Mutex> lock(mutex_);

        lua_State *L = L_;
        lua_getglobal(L, key.data());
        assert(lua_isnil(L, -1));
        lua_pop(L, 1);
        lua_pushstring(L, value.data());
        lua_setglobal(L, key.data());
    }


}


