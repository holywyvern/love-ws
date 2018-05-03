#include "lua_ws_client.hpp"

#include "client_ws.hpp"
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

const luaL_reg LuaWsClient::methods[] = {
    method(LuaWsClient, connect),
    method(LuaWsClient, close),
    method(LuaWsClient, checkQueue),
    method(LuaWsClient, sendMessage),
    {0, 0}};

const char LuaWsClient::className[] = "LuaWsClient";

void 
WsClient::pushMessage(std::string message)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    _messageQueue.push(message);
}

void
WsClient::popMessage(lua_State *L)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    if (_messageQueue.empty()) {
        lua_pushnil(L);
    } else {
        auto str(_messageQueue.front());
		size_t len = str.size();
		const char *data = str.data();
        lua_pushlstring(L, data, len);
    }
    _messageQueue.pop();
}

void
WsClient::sendMessage(std::string message)
{
    auto send_stream = std::make_shared<WsClient::SendStream>();
    *send_stream << message;
    this->connection->send(send_stream);  
}

WsClient *
LuaWsClient::check(lua_State *L, int narg) 
{
    luaL_checktype(L, narg, LUA_TUSERDATA);
    void *ud = luaL_checkudata(L, narg, className);
    if(!ud) luaL_typerror(L, narg, className);
    return *(WsClient**)ud;  // unbox pointer
}

void LuaWsClient::setup(lua_State *L)
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
    lua_pushcfunction(L, LuaWsClient::gc);
    lua_settable(L, metatable);

    lua_pop(L, 1); // drop metatable

    luaL_openlib(L, 0, methods, 0); // fill methodtable
    lua_pop(L, 1);                  // drop methodtable

    lua_register(L, "newClient", LuaWsClient::create);
}

int LuaWsClient::create(lua_State *L)
{
    lua_settop(L, 1);
	size_t len;
    const char *url = luaL_checklstring(L, 1, &len);
	std::string ref(url, len);
    WsClient *client = new WsClient(url);
    client->_serverOpen = false;
    client->on_message = [&](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
        client->pushMessage(message->string());
    };
    client->on_open = [&](std::shared_ptr<WsClient::Connection> connection) {
        client->_serverOpen = true;
    };
    client->on_close = [&](std::shared_ptr<WsClient::Connection> connection, int status, const std::string & reason) {
        client->_serverOpen = false;
    };
    client->on_error = [&](std::shared_ptr<WsClient::Connection> connection, const SimpleWeb::error_code &ec) {
    };
    lua_boxpointer(L, client);
    luaL_getmetatable(L, className);
    lua_setmetatable(L, -2);
    return 1;    
}

int  LuaWsClient::gc(lua_State *L)
{
    WsClient *client = (WsClient*)lua_unboxpointer(L, 1);
    delete client;
    return 0;    
}

int LuaWsClient::connect(lua_State *L)
{
    WsClient *ws = LuaWsClient::check(L, 1);
    ws->start();
    return 0;
}

int LuaWsClient::close(lua_State *L)
{
    WsClient *ws = LuaWsClient::check(L, 1);
    if (!ws->_serverOpen) {
        return 0;
    }
    return 0;
}

int LuaWsClient::checkQueue(lua_State *L)
{
    WsClient *ws = LuaWsClient::check(L, 1);
    ws->popMessage(L);
    return 1;
}

int LuaWsClient::sendMessage(lua_State *L)
{
    WsClient *ws = LuaWsClient::check(L, 1);
    size_t len;
    const char *str = luaL_checklstring(L, 2, &len);
    std::string message(str, len);  
    ws->sendMessage(message);
    return 0;
}

int LuaWsClient::isOpen(lua_State *L)
{
    WsClient *ws = LuaWsClient::check(L, 1);
    lua_pushboolean(L, ws->_serverOpen ? 1 : 0);
    return 1;
}