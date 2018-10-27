#ifndef LUA_WSS_CLIENT
#define LUA_WSS_CLIENT

#include <queue>

#include <boost/thread.hpp>

#include "lua.hpp"
#include "client_wss.hpp"

class WssClientMessage {
public:
	int type;
	std::string message;
	WssClientMessage(int type, std::string message);
	std::string getTypeName();
};

class WssClient : public SimpleWeb::SocketClient<SimpleWeb::WSS>
{
  public:
	  WssClient(const std::string &server_port_path, bool certificate) noexcept;
    bool _serverOpen;
    boost::mutex _queueMutex;
    std::queue<WssClientMessage> _messageQueue;
	  std::shared_ptr<boost::thread> thread; 
	  void popMessage(lua_State *L);
    void pushMessage(int type, std::string message);
    void sendMessage(std::string message);
};

class LuaWssClient
{
public:
  static const char className[];
  static const luaL_reg methods[];
  // Lua VM functions
  static WssClient *check(lua_State *L, int narg);
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