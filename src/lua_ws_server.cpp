#include "lua_ws_server.hpp"
#include "lua_ws_server_channel.hpp"

#include "lua.hpp"

#ifndef lua_boxpointer
#define lua_boxpointer(L,u) \
        (*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))
#endif

#ifndef lua_unboxpointer
#define lua_unboxpointer(L,i)   (*(void **)(lua_touserdata(L, i)))
#endif 

#define method(class, name) \
    { #name, class::name }

static const std::string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

const luaL_reg LuaWsServer::methods[] = {
	{"getChannel", LuaWsServerChannel::create},
	method(LuaWsServer, start),
	method(LuaWsServer, stop),
    {0, 0}};

const char LuaWsServer::className[] = "LuaWsServer";

static std::string
generateUUID() {
	std::string uuid = std::string(36, ' ');
	int rnd = 0;
	int r = 0;

	uuid[8] = '-';
	uuid[13] = '-';
	uuid[18] = '-';
	uuid[23] = '-';

	uuid[14] = '4';

	for (int i = 0; i < 36; i++) {
		if (i != 8 && i != 13 && i != 18 && i != 14 && i != 23) {
			if (rnd <= 0x02) {
				rnd = 0x2000000 + (std::rand() * 0x1000000) | 0;
			}
			rnd >>= 4;
			uuid[i] = CHARS[(i == 19) ? ((rnd & 0xf) & 0x3) | 0x8 : rnd & 0xf];
		}
	}
	return uuid;
}

WsServer::WsServer(int port) : SimpleWeb::SocketServer<SimpleWeb::WS>() {
	this->config.port = port;
	this->thread = nullptr;
}

std::string
WsServer::getId(std::shared_ptr<WsServer::Connection> connection)
{
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_connectionIds.count(connection)  < 1) {
		std::string uuid = generateUUID();
		while (_idMap.count(uuid) > 0) {
			uuid = generateUUID();
		}
		_connectionIds[connection] = uuid;
		_idMap[uuid] = connection;
	}
	return _connectionIds[connection];
}

WsServer *
LuaWsServer::check(lua_State *L, int narg) 
{
    luaL_checktype(L, narg, LUA_TUSERDATA);
    void *ud = luaL_checkudata(L, narg, className);
    if(!ud) luaL_typerror(L, narg, className);
    return *(WsServer**)ud;  // unbox pointer
}

void LuaWsServer::setup(lua_State *L)
{
    lua_newtable(L);
    int methodtable = lua_gettop(L);
    luaL_newmetatable(L, className);
    int metatable = lua_gettop(L);

    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, methodtable);
    lua_settable(L, metatable); // hide metatable from Lua getmetatable()

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, methodtable);
    lua_settable(L, metatable);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, LuaWsServer::gc);
    lua_settable(L, metatable);

    lua_pop(L, 1); // drop metatable

    luaL_openlib(L, 0, methods, 0); // fill methodtable
    lua_pop(L, 1);                  // drop methodtable

	lua_pushcfunction(L, LuaWsServer::create);
	lua_setfield(L, -2, "newServer");    
}

int LuaWsServer::create(lua_State *L)
{
    lua_settop(L, 1);
    int port = luaL_checkinteger(L, 1);
    WsServer *server = new WsServer(port);
    lua_boxpointer(L, server);
    luaL_getmetatable(L, className);
    lua_setmetatable(L, -2);    
    return 1;  
}

int  LuaWsServer::gc(lua_State *L)
{
    WsServer *server = (WsServer*)lua_unboxpointer(L, 1);
	if (server->thread) {
		server->stop();
		server->thread->join();
		server->thread = nullptr;
	}
    delete server;
    return 0;       
}

int LuaWsServer::start(lua_State *L)
{
	WsServer * server = LuaWsServer::check(L, 1);
	server->thread = std::make_shared<std::thread>([=]() {
		server->start();
	});
	return 0;
}

int LuaWsServer::stop(lua_State *L)
{
	WsServer * server = LuaWsServer::check(L, 1);
	server->stop();
	if (server->thread) {
		server->thread->join();
		server->thread = nullptr;
	}
	return 0;
}