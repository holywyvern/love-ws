#ifndef LUA_WS_SERVER
#define LUA_WS_SERVER

extern "C" {
typedef struct lua_State lua_State;
}

class LuaWsServer
{
public:
  static void setup(lua_State *L);
  static void create(lua_State *L);
  static int  gc(lua_State *L);
};

#endif
