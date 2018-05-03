#include "lua_ws_server.hpp"

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

const luaL_reg LuaWsServer::methods[] = {
    {0, 0}};

const char LuaWsServer::className[] = "LuaWsServer";

WsServer::WsServer(int port) : SimpleWeb::SocketServer<SimpleWeb::WS>() {
	this->config.port = port;
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
    delete server;
    return 0;       
}