#ifndef LUA_WS_CLIENT
#define LUA_WS_CLIENT

#include <queue>
#include <mutex>

#include "lua.hpp"
#include "client_ws.hpp"

class WsClient : public SimpleWeb::SocketClient<SimpleWeb::WS>
{
  public:
	  WsClient(const std::string &server_port_path) noexcept : SimpleWeb::SocketClient<SimpleWeb::WS>(server_port_path)  {}
    bool _serverOpen;
    std::mutex _queueMutex;
    std::queue<std::string> _messageQueue;
	void popMessage(lua_State *L);
    void pushMessage(std::string message);
    void sendMessage(std::string message);
};

class LuaWsClient
{
public:
  static const char className[];
  static const luaL_reg methods[];
  // Lua VM functions
  static WsClient *check(lua_State *L, int narg);
  static void setup(lua_State *L);
  static int create(lua_State *L);
  static int  gc(lua_State *L);
  // Lua Methods
  static int connect(lua_State *L);
  static int close(lua_State *L);
  static int checkQueue(lua_State *L);
  static int sendMessage(lua_State *L);
  static int isOpen(lua_State *L);
};

#endif