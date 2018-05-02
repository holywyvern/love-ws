#include "lua.hpp"

#if defined(_WIN32) || defined(WIN32)
#define LOVE_WS_EXPORT __declspec(dllexport)
#else
#define LOVE_WS_EXPORT
#endif

extern "C" LUALIB_API LOVE_WS_EXPORT int
luaopen_ws(lua_State *L)
{
    lua_newtable(L);
    return 1;
}