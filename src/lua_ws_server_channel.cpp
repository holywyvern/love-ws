#include "lua_ws_server.hpp"
#include "lua_ws_server_channel.hpp"
#include <string>
#include <cstdlib>

#ifndef lua_boxpointer
#define lua_boxpointer(L,u) \
        (*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))
#endif

#ifndef lua_unboxpointer
#define lua_unboxpointer(L,i)   (*(void **)(lua_touserdata(L, i)))
#endif 

#define method(class, name) \
    { #name, class::name }

const char LuaWsServerChannel::className[] = "LuaWsServerChannel";

const luaL_reg LuaWsServerChannel::methods[] = {
	{ 0, 0 } };

static const std::string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

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

WsServerMessage::WsServerMessage(std::string id, int type, std::string message)
{
	this->connectionId = id;
	this->type = type;
	this->message = message;
}

WsServerChannel::WsServerChannel(WsServer *server, const char *channel, size_t len)
{
	_endpoint = std::string(channel, len);
	_server = server;
	auto& endpoint = _server->endpoint[_endpoint];
	endpoint.on_message = [&](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::Message> message) {
		this->pushMessage(this->getId(connection), 2, message->string());
	};
	endpoint.on_open = [&](std::shared_ptr<WsServer::Connection> connection) {
		auto id = this->getId(connection);
		this->pushMessage(this->getId(connection), 0);
	};

	// See RFC 6455 7.4.1. for status codes
	endpoint.on_close = [&](std::shared_ptr<WsServer::Connection> connection, int status, const std::string & reason) {
		this->pushMessage(this->getId(connection), 1);
		this->removeConnection(connection);
	};
}

void
WsServerChannel::removeConnection(std::string id)
{
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_idMap.count(id) > 0) {
		auto connection = _idMap[id];
		if (_connectionIds.count(connection) > 0) {
			_connectionIds.erase(connection);
		}
		_idMap.erase(id);
	}
}

void 
WsServerChannel::removeConnection(std::shared_ptr<WsServer::Connection> connection)
{
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_connectionIds.count(connection) > 0) {
		std::string id = _connectionIds[connection];
		if (_idMap.count(id) > 0) {
			_idMap.erase(id);
		}
		_connectionIds.erase(connection);
	}
}

std::string 
WsServerChannel::getId(std::shared_ptr<WsServer::Connection> connection)
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

void WsServerChannel::popMessage(lua_State *L)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
}

void WsServerChannel::pushMessage(std::string connection, int type)
{
	std::string m("");
	this->pushMessage(connection, type, m);
}

void WsServerChannel::pushMessage(std::string connection, int type, std::string message)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	WsServerMessage msg(connection, type, message);
	this->_messageQueue.push(msg);
}

void WsServerChannel::sendMessage(std::string id, std::string message)
{
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_idMap.count(id) > 0) {
		auto connection = _idMap[id];
		auto send_stream = std::make_shared<WsServer::SendStream>();
		*send_stream << message;
		connection->send(send_stream, [](const SimpleWeb::error_code &ec) {
			if (ec) {
				// Error
			}
		});
	}
	
}

void WsServerChannel::broadcast(std::string message)
{
	for (auto pair : _idMap) {
		this->sendMessage(pair.first, message);
	}
}

void WsServerChannel::disconnect(std::string id)
{
	this->removeConnection(id);
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_idMap.count(id) > 0) {
		auto connection = _idMap[id];
		connection->send_close(100);
	}
}

WsServerChannel *
LuaWsServerChannel::check(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud) luaL_typerror(L, narg, className);
	return *(WsServerChannel**)ud;  // unbox pointer
}

void 
LuaWsServerChannel::setup(lua_State *L)
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
	lua_pushcfunction(L, LuaWsServerChannel::gc);
	lua_settable(L, metatable);

	lua_pop(L, 1); // drop metatable

	luaL_openlib(L, 0, methods, 0); // fill methodtable
	lua_pop(L, 1);                  // drop methodtable
}

int 
LuaWsServerChannel::create(lua_State *L)
{
	WsServer * server = LuaWsServer::check(L, 1);
	size_t len;
	const char *endpoint = luaL_checklstring(L, 2, &len);
	WsServerChannel *channel = new WsServerChannel(server, endpoint, len);
	lua_boxpointer(L, channel);
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

int  
LuaWsServerChannel::gc(lua_State *L)
{
	WsServerChannel *channel = (WsServerChannel*)lua_unboxpointer(L, 1);
	delete channel;
	return 0;
}