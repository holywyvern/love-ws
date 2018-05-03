#ifndef LUA_WS_SERVER
#define LUA_WS_SERVER

#include <queue>
#include <mutex>

#include "lua.hpp"
#include "server_ws.hpp"

extern "C" {
typedef struct lua_State lua_State;
}

class WsServer : public SimpleWeb::SocketServer<SimpleWeb::WS>
{
  public:
	  WsServer(int port);
};

class LuaWsServer
{
public:
  static WsServer *check(lua_State *L, int narg);
  static const char className[];
  static const luaL_reg methods[];
  static void setup(lua_State *L);
  static int create(lua_State *L);
  static int  gc(lua_State *L);
};

#endif
