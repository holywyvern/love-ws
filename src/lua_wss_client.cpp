#include "lua_wss_client.hpp"

#include "client_wss.hpp"
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

const luaL_reg LuaWssClient::methods[] = {
    method(LuaWssClient, connect),
    method(LuaWssClient, close),
    method(LuaWssClient, checkQueue),
    method(LuaWssClient, sendMessage),
    {0, 0}};

const char LuaWssClient::className[] = "LuaWssClient";

WssClientMessage::WssClientMessage(int type, std::string message)
{
	this->type = type;
	this->message = message;
}


std::string WssClientMessage::getTypeName()
{
	switch (this->type)
	{
	case 1: return std::string("message");
	case 2: return std::string("open");
	case 3: return std::string("close");
	case 4: return std::string("error");
	case 5: return std::string("connecting");
	default: break;
	}
	return std::string("unknown");
}

WssClient::WssClient(const std::string &server_port_path, bool validate_cert) noexcept :
SimpleWeb::SocketClient<SimpleWeb::WSS>(server_port_path, validate_cert) {
	this->thread = nullptr;
}

void
WssClient::pushMessage(int type, std::string message)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	WssClientMessage msg(type, message);
	_messageQueue.push(msg);
}

void
WssClient::popMessage(lua_State *L)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	if (_messageQueue.empty()) {
		lua_pushnil(L);
	}
	else {
		auto msg(_messageQueue.front());
		lua_createtable(L, 0, 2);
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

void
WssClient::sendMessage(std::string message)
{
	auto send_stream = std::make_shared<WssClient::SendStream>();
	*send_stream << message;
	this->connection->send(send_stream);
}


WssClient *
LuaWssClient::check(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud) luaL_typerror(L, narg, className);
	return *(WssClient**)ud;  // unbox pointer
}

void 
LuaWssClient::setup(lua_State *L)
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
	lua_pushcfunction(L, LuaWssClient::gc);
	lua_settable(L, metatable);

	lua_pop(L, 1); // drop metatable

	luaL_openlib(L, 0, methods, 0); // fill methodtable
	lua_pop(L, 1);                  // drop methodtable

	lua_pushcfunction(L, LuaWssClient::create);
	lua_setfield(L, -2, "newTlsClient");
}

int 
LuaWssClient::create(lua_State *L)
{
    lua_settop(L, 1);
	size_t len;
    const char *url = luaL_checklstring(L, 1, &len);
	std::string ref(url, len);
    int validate_cert = lua_toboolean(L, 2);
    WssClient *client = new WssClient(url, validate_cert ? true : false);
    client->_serverOpen = false;
	client->pushMessage(5, std::string(""));
    client->on_message = [=](std::shared_ptr<WssClient::Connection> connection, std::shared_ptr<WssClient::Message> message) {
        client->pushMessage(1, message->string());
    };
    client->on_open = [=](std::shared_ptr<WssClient::Connection> connection) {
        client->_serverOpen = true;
		client->pushMessage(2, std::string(""));
    };
    client->on_close = [=](std::shared_ptr<WssClient::Connection> connection, int status, const std::string & reason) {
        client->_serverOpen = false;
		client->pushMessage(3, std::string(""));
    };
    client->on_error = [=](std::shared_ptr<WssClient::Connection> connection, const SimpleWeb::error_code &ec) {
		client->pushMessage(4, ec.message());
    };
    lua_boxpointer(L, client);
    luaL_getmetatable(L, className);
    lua_setmetatable(L, -2);
    return 1;    
}

int  
LuaWssClient::gc(lua_State *L)
{
	WssClient *ws = (WssClient*)lua_unboxpointer(L, 1);
	if (ws->_serverOpen) {
		ws->stop();
		if (ws->thread) {
			ws->thread->join();
			ws->thread = nullptr;
		}
	}
	delete ws;
	return 0;
}

int 
LuaWssClient::connect(lua_State *L)
{
	WssClient *ws = LuaWssClient::check(L, 1);
	ws->thread = std::make_shared<std::thread>([=]() {
		ws->start();
	});
	return 0;
}

int 
LuaWssClient::close(lua_State *L)
{
	WssClient *ws = LuaWssClient::check(L, 1);
	if (!ws->_serverOpen) {
		return 0;
	}
	ws->stop();
	if (ws->thread) {
		ws->thread->join();
		ws->thread = nullptr;
	}
	return 0;
}

int 
LuaWssClient::checkQueue(lua_State *L)
{
	WssClient *ws = LuaWssClient::check(L, 1);
	ws->popMessage(L);
	return 1;
}

int 
LuaWssClient::sendMessage(lua_State *L)
{
	WssClient *ws = LuaWssClient::check(L, 1);
	size_t len;
	const char *str = luaL_checklstring(L, 2, &len);
	std::string message(str, len);
	ws->sendMessage(message);
	return 0;
}

int 
LuaWssClient::isOpen(lua_State *L)
{
	WssClient *ws = LuaWssClient::check(L, 1);
	lua_pushboolean(L, ws->_serverOpen ? 1 : 0);
	return 1;
}