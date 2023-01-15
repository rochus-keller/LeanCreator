#include "Engine.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <bslib.h>

using namespace busy;

class Engine::Imp
{
public:
    lua_State *L;
};

Engine::Engine()
{
    d_imp = new Imp();
    d_imp->L = lua_open();
    if( d_imp->L == 0 )
        return;
    luaL_openlibs(d_imp->L);
    lua_pushcfunction(d_imp->L, bs_open_busy);
    lua_pushstring(d_imp->L, BS_BSLIBNAME);
    lua_call(d_imp->L, 1, 0);
}

Engine::~Engine()
{
    if( d_imp->L )
        lua_close(d_imp->L);
    delete d_imp;
}

