#include "lua.hpp"
#include "lua_ws_client.hpp"
#include "lua_ws_server.hpp"
#include "lua_ws_server_channel.hpp"

#if defined(_WIN32) || defined(WIN32)
#define LOVE_WS_EXPORT __declspec(dllexport)
#else
#define LOVE_WS_EXPORT
#endif

extern "C" LUALIB_API LOVE_WS_EXPORT int
luaopen_ws(lua_State *L)
{
    lua_newtable(L);
    LuaWsClient::setup(L);
    LuaWsServer::setup(L);
	LuaWsServerChannel::setup(L);
    return 1;
}