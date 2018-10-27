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
	method(LuaWsServerChannel, disconnect),
	method(LuaWsServerChannel, send),
	method(LuaWsServerChannel, broadcast),
	method(LuaWsServerChannel, checkQueue),
	{ 0, 0 } };

WsServerMessage::WsServerMessage(std::string id, int type, std::string message)
{
	this->connectionId = id;
	this->type = type;
	this->message = message;
}

std::string WsServerMessage::getTypeName()
{
	switch (this->type)
	{
		case 0: return std::string("open");
		case 1: return std::string("close");
		case 2: return std::string("message");
		case 3: return std::string("connecting");
		case 4: return std::string("error");
		default: break;
	}
	return std::string("unknown");
}

WsServerChannel::WsServerChannel(WsServer *server, const char *channel, size_t len)
{
	_endpoint = std::string(channel, len);
	_server = server;
	auto& endpoint = _server->endpoint[_endpoint];
	auto channelPtr = this;
	endpoint.on_message = [=](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::Message> message) {
		channelPtr->pushMessage(this->getId(connection), 2, message->string());
	};
	endpoint.on_open = [=](std::shared_ptr<WsServer::Connection> connection) {
		auto id = channelPtr->getId(connection);
		channelPtr->pushMessage(this->getId(connection), 0);
	};

	// See RFC 6455 7.4.1. for status codes
	endpoint.on_close = [=](std::shared_ptr<WsServer::Connection> connection, int status, const std::string & reason) {
		channelPtr->pushMessage(this->getId(connection), 1);
		channelPtr->removeConnection(connection);
	};
	endpoint.on_error = [=](std::shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
		channelPtr->pushMessage(channelPtr->getId(connection), 4, ec.message());
	};
}

void
WsServerChannel::removeConnection(std::string id)
{
	boost::lock_guard<boost::mutex> lock{_idMutex};
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
	boost::lock_guard<boost::mutex> lock{_idMutex};
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
	boost::lock_guard<boost::mutex> lock{_idMutex};
	if (_connectionIds.count(connection)  < 1) {
		std::string uuid = this->_server->getId(connection);
		_connectionIds[connection] = uuid;
		_idMap[uuid] = connection;
	}
	return _connectionIds[connection];
}

void WsServerChannel::popMessage(lua_State *L)
{
	boost::lock_guard<boost::mutex> lock{_idMutex};
	if (_messageQueue.empty()) {
		lua_pushnil(L);
	}
	else {
		auto msg(_messageQueue.front());
		lua_createtable(L, 0, 3);
		lua_pushstring(L, "connection");
		lua_pushlstring(L, msg.connectionId.data(), msg.connectionId.size());
		lua_settable(L, -3);
		lua_pushstring(L, "type");
		auto type = msg.getTypeName();
		lua_pushlstring(L, type.data(), type.size());
		lua_settable(L, -3);
		lua_pushstring(L, "message");
		lua_pushlstring(L, msg.message.data(), msg.message.size());
		lua_settable(L, -3);
		_messageQueue.pop();
	}
}

void WsServerChannel::pushMessage(std::string connection, int type)
{
	std::string m("");
	this->pushMessage(connection, type, m);
}

void WsServerChannel::pushMessage(std::string connection, int type, std::string message)
{
	boost::lock_guard<boost::mutex> lock{_idMutex};
	WsServerMessage msg(connection, type, message);
	this->_messageQueue.push(msg);
}

void WsServerChannel::sendMessage(std::string id, std::string message)
{
	boost::lock_guard<boost::mutex> lock{_idMutex};
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
	auto send_stream = std::make_shared<WsServer::SendStream>();
	*send_stream << message;
	for (auto &pair : this->_connectionIds) {
		pair.first->send(send_stream);
	}
}

void WsServerChannel::disconnect(std::string id)
{
	this->removeConnection(id);
	boost::lock_guard<boost::mutex> lock{_idMutex};
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

int
LuaWsServerChannel::disconnect(lua_State *L)
{
	WsServerChannel * server = LuaWsServerChannel::check(L, 1);
	size_t len;
	const char *id = luaL_checklstring(L, 2, &len);
	server->disconnect(std::string(id, len));
	return 0;
}

int
LuaWsServerChannel::send(lua_State *L)
{
	WsServerChannel * server = LuaWsServerChannel::check(L, 1);
	size_t id_len, msg_len;
	const char *id = luaL_checklstring(L, 2, &id_len);
	const char *msg = luaL_checklstring(L, 3, &msg_len);
	server->sendMessage(std::string(id, id_len), std::string(msg, msg_len));
	return 0;
}

int
LuaWsServerChannel::broadcast(lua_State *L)
{
	WsServerChannel * server = LuaWsServerChannel::check(L, 1);
	size_t len;
	const char *msg = luaL_checklstring(L, 3, &len);
	server->broadcast(std::string(msg, len));
	return 0;
}


int
LuaWsServerChannel::checkQueue(lua_State *L)
{
	WsServerChannel * server = LuaWsServerChannel::check(L, 1);
	server->popMessage(L);
	return 1;
}