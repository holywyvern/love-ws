#ifndef LUA_WS_SERVER_CHANNEL
#define LUA_WS_SERVER_CHANNEL

#include <queue>
#include <mutex>

#include "lua.hpp"
#include "server_ws.hpp"
#include "lua_ws_server.hpp"

class WsServerMessage {
public:
	std::string connectionId;
	int type;
	std::string message;
	WsServerMessage(std::string id, int type, std::string message);
	std::string getTypeName();
};

class WsServerChannel {
private: 
	WsServer *_server;
	std::mutex _queueMutex;
	std::mutex _idMutex;
	std::queue<WsServerMessage> _messageQueue;
	std::string _endpoint;
	std::map<std::shared_ptr<WsServer::Connection>, std::string> _connectionIds;
	std::map<std::string, std::shared_ptr<WsServer::Connection>> _idMap;
public: 
	WsServerChannel(WsServer *server, const char *channel, size_t len);
	void popMessage(lua_State *L);
	void pushMessage(std::string connection, int type);
	void pushMessage(std::string connection, int type, std::string message);
	void removeConnection(std::string id);
	void removeConnection(std::shared_ptr<WsServer::Connection> connection);
	void sendMessage(std::string id, std::string message);
	void broadcast(std::string message);
	void disconnect(std::string id);
	std::string getId(std::shared_ptr<WsServer::Connection> connection);
};

class LuaWsServerChannel {
    public:
    static WsServerChannel *check(lua_State *L, int narg);
    static const char className[];
    static const luaL_reg methods[];
	// Lua VM methods
    static void setup(lua_State *L);
    static int create(lua_State *L);
    static int  gc(lua_State *L);
	// Lua methods
	static int disconnect(lua_State *L);
	static int send(lua_State *L);
	static int broadcast(lua_State *L);
	static int checkQueue(lua_State *L);
};

#endif