/*
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of LeanCreator.
**
** $QT_BEGIN_LICENSE:LGPL21$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*/

#include "Engine.h"
#include <QFile>
#include <QtDebug>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <bslib.h>
#include <bsparser.h>
#include <bshost.h>
}

using namespace busy;

class Engine::Imp
{
public:
    lua_State *L;
    bool ok() const { return L != 0; }
};

static bool loadLib( lua_State *L, const QByteArray& source, const QByteArray& name)
{
    const int top = lua_gettop(L);

    const int status = luaL_loadbuffer( L, source, source.size(), name.constData() );
    switch( status )
    {
    case 0:
        // Stack: function
        break;
    case LUA_ERRSYNTAX:
    case LUA_ERRMEM:
        qCritical() << lua_tostring( L, -1 );
        lua_pop( L, 1 );  /* remove error message */
        // Stack: -
        return false;
    }

    const int err = lua_pcall( L, 0, 1, 0 );
    switch( err )
    {
    case LUA_ERRRUN:
        qCritical() << lua_tostring( L, -1 );
        lua_pop( L, 1 );  /* remove error message */
        return false;
    case LUA_ERRMEM:
        qCritical() << "Lua memory exception";
        return false;
    case LUA_ERRERR:
        // should not happen
        qCritical() << "Lua unknown error";
        return false;
    }

    // stack: lib
    lua_getfield(L, LUA_GLOBALSINDEX, "package"); // stack: lib, package
    lua_getfield(L, -1, "loaded"); // stack: lib, package, loaded
    lua_pushvalue(L, -3 ); // stack: lib, package, loaded, lib
    lua_setfield(L, -2, name.constData() ); // stack: lib, package, loaded,
    lua_pop(L,2); // stack: lib

    Q_ASSERT( top + 1 == lua_gettop(L) );
    return true;
}

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

    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveXref");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveNumRefs");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveLocInfo");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveFullAst");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveXref");
}

Engine::~Engine()
{
    if( d_imp->L )
        lua_close(d_imp->L);
    delete d_imp;
}

bool Engine::parse(const QByteArray& dir)
{
    // TODO: set #build_mode
    QFile script(":/busy/builtins.lua");
    if( !script.open(QIODevice::ReadOnly) )
    {
        qCritical() << "cannot open builtins.lua";
        return false;
    }
    const QByteArray source = script.readAll();
    if( loadLib(d_imp->L,source,"builtins") )
        lua_setglobal(d_imp->L,"#builtins");
    else
        return false;

    lua_pushcfunction(d_imp->L, bs_compile);
    lua_pushstring(d_imp->L,dir.constData());
    const int err = lua_pcall(d_imp->L,1,0,0);
    switch( err )
    {
    case LUA_ERRRUN:
        qCritical() << lua_tostring(d_imp->L, -1 );
        lua_pop(d_imp->L, 1 );  /* remove error message */
        return false;
    case LUA_ERRMEM:
        qCritical() << "Lua memory exception";
        return false;
    case LUA_ERRERR:
        // should not happen
        qCritical() << "Lua unknown error";
        return false;
    }
    return true;
}

int Engine::getRootModule() const
{
    if( d_imp->ok() )
    {
        lua_getglobal(d_imp->L,"#root");
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            const int ref = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,2);
            return ref;
        }else
            lua_pop(d_imp->L,1);
    }
    return 0;
}

QList<int> Engine::getSubModules(int id) const
{
    const int top = lua_gettop(d_imp->L);
    QList<int> res;
    if( !d_imp->ok() )
        return res;
    if( d_imp->ok() && pushInst(id) )
    {
        const int n = lua_objlen(d_imp->L,-1);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,-1,i);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_getfield(d_imp->L,-1,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_ModuleDef )
                {
                    lua_getfield(d_imp->L,-1,"#ref");
                    const int ref = lua_tointeger(d_imp->L,-1);
                    lua_pop(d_imp->L,1);
                    res += ref;
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<int> Engine::getAllProducts(int id, bool withSourceOnly, bool runnableOnly) const
{
    if( runnableOnly )
        withSourceOnly = false;
    const int top = lua_gettop(d_imp->L);
    QList<int> res;
    if( d_imp->ok() && pushInst(id) )
    {
        const int inst = lua_gettop(d_imp->L);
        lua_getglobal(d_imp->L,"#builtins");
        if( runnableOnly )
            lua_getfield(d_imp->L,-1,"Executable");
        else
            lua_getfield(d_imp->L,-1,"Product");
        lua_replace(d_imp->L,-2);
        const int productClass = lua_gettop(d_imp->L);

        const int n = lua_objlen(d_imp->L,inst);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,inst,i);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_getfield(d_imp->L,-1,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                lua_getfield(d_imp->L,-1,"#ctr");
                const int hasBody = !lua_isnil(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_VarDecl && hasBody )
                {
                    lua_getfield(d_imp->L,-1,"#type");
                    const bool isProduct = bs_isa(d_imp->L,productClass,-1);
                    bool hasSource = false;
                    if( withSourceOnly )
                    {
                        lua_getfield(d_imp->L,-1,"sources");
                        lua_getfield(d_imp->L,-2,"use_deps"); // exclude Copy
                        hasSource = !lua_isnil(d_imp->L,-2) && lua_isnil(d_imp->L,-1);
                        lua_pop(d_imp->L,2);
                    }
                    lua_pop(d_imp->L,1);
                    if( isProduct )
                    {
                        if( !withSourceOnly || hasSource )
                        {
                            lua_getfield(d_imp->L,-1,"#ref");
                            const int ref = lua_tointeger(d_imp->L,-1);
                            lua_pop(d_imp->L,1);
                            res += ref;
                        }
                    }
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,2);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QStringList Engine::getAllSources(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,decl,"#owner");
        lua_getfield(d_imp->L,-1,"#dir");
        lua_replace(d_imp->L,-2);
        const int absDir = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,decl,"#inst");
        lua_getfield(d_imp->L,-1,"sources");
        lua_replace(d_imp->L,-2);
        const int sources = lua_gettop(d_imp->L);


        if( !lua_isnil(d_imp->L,sources) )
        {
            const int n = lua_objlen(d_imp->L,sources);
            for( int i = 1; i <= n; i++ )
            {
                lua_rawgeti(d_imp->L,sources,i);
                const int file = lua_gettop(d_imp->L);
                if( *lua_tostring(d_imp->L,file) != '/' )
                {
                    if( bs_add_path(d_imp->L,absDir,file) == 0 )
                        lua_replace(d_imp->L,file);
                }
                res << QString::fromUtf8( bs_denormalize_path(lua_tostring(d_imp->L,file)) );

                lua_pop(d_imp->L,1);
            }
        }
        lua_pop(d_imp->L,3);
    }
    return res;
}

bool Engine::isExecutable(int id) const
{
    if( !d_imp->ok() )
        return false;
    if( d_imp->ok() && pushInst(id) )
    {
        const int inst = lua_gettop(d_imp->L);
        lua_getglobal(d_imp->L,"#builtins");
        lua_getfield(d_imp->L,-1,"Executable");
        lua_replace(d_imp->L,-2);
        const int cls = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,inst,"#kind");
        const int k = lua_tointeger(d_imp->L,-1);
        lua_pop(d_imp->L,1);
        lua_getfield(d_imp->L,inst,"#ctr");
        const int hasBody = !lua_isnil(d_imp->L,-1);
        lua_pop(d_imp->L,1);
        bool isExe = false;
        if( k == BS_VarDecl )
        {
            lua_getfield(d_imp->L,-1,"#type");
            isExe = bs_isa(d_imp->L,cls,-1);
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
        return hasBody;
    }else
        return false;
}

QByteArray Engine::getString(int def, const char* field, bool inst) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        if( inst )
        {
            lua_getfield(d_imp->L,-1,"#inst");
            if( !lua_isnil(d_imp->L,-1) )
                lua_replace(d_imp->L,-2);
            else
                lua_pop(d_imp->L,1);
        }
        lua_getfield(d_imp->L,-1,field);
        const QByteArray res = lua_tostring(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return res;
    }
    return QByteArray();
}

int Engine::getInteger(int def, const char* field) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        const int res = lua_tointeger(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return res;
    }
    return 0;
}

int Engine::getOwner(int def) const
{
    int res = 0;
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,"#owner");
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            res = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,2);
    }
    return res;
}

bool Engine::pushInst(int ref) const
{
    int n = 1;
    lua_getglobal(d_imp->L,"#refs");
    if( lua_istable(d_imp->L,-1) )
    {
        lua_rawgeti(d_imp->L,-1,ref);
        n++;
        if( lua_istable(d_imp->L,-1) )
        {
            lua_replace(d_imp->L,-2);
            return true;
        }
    }
    lua_pop(d_imp->L,n);
    return false;
}

