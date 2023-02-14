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
#include <bsrunner.h>
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

void Engine::registerLogger(BSLogger l, void* data)
{
    if( d_imp->ok() )
        bs_preset_logger(d_imp->L,l,data);
}

bool Engine::parse(const ParseParams& params)
{

    lua_pushstring(d_imp->L,params.build_mode.constData());
    lua_setglobal(d_imp->L,"#build_mode");

    QFile script(":/busy/builtins.lua");
    if( !script.open(QIODevice::ReadOnly) )
    {
        qCritical() << "cannot open builtins.lua";
        return false;
    }
    const QByteArray source = script.readAll();

    if( !loadLib(d_imp->L,source,"builtins") )
        return false;
    const int builtins = lua_gettop(d_imp->L);
    lua_pushvalue(d_imp->L,builtins);
    lua_setglobal(d_imp->L,"#builtins");
    lua_getfield(d_imp->L,builtins,"#inst");
    lua_replace(d_imp->L,builtins);

    lua_pushstring(d_imp->L,params.cpu);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_cpu");
    lua_setfield(d_imp->L,builtins,"target_cpu");

    lua_pushstring(d_imp->L,params.os);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_os");
    lua_setfield(d_imp->L,builtins,"target_os");

    lua_pushstring(d_imp->L,params.wordsize);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_wordsize");
    lua_setfield(d_imp->L,builtins,"target_wordsize");

    lua_pushstring(d_imp->L,params.toolchain);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_toolchain");
    lua_setfield(d_imp->L,builtins,"target_toolchain");

    lua_pushstring(d_imp->L,params.toolchain_path); // TODO normalize
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"#toolchain_path");
    lua_setfield(d_imp->L,builtins,"target_toolchain_path");

    lua_pushstring(d_imp->L,params.toolchain_prefix);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"#toolchain_prefix");
    lua_setfield(d_imp->L,builtins,"target_toolchain_prefix");

    lua_pushinteger(d_imp->L,0);
    lua_pushvalue(d_imp->L,-1);
    lua_pushvalue(d_imp->L,-1);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_cpu_ver");
    lua_setfield(d_imp->L,builtins,"host_toolchain_ver");
    lua_setfield(d_imp->L,builtins,"target_cpu_ver");
    lua_setfield(d_imp->L,builtins,"target_toolchain_ver");

    lua_pop(d_imp->L,1); // builtins

    lua_pushcfunction(d_imp->L, bs_compile);
    lua_pushstring(d_imp->L,params.root_source_dir.constData());
    lua_pushstring(d_imp->L,params.root_build_dir.constData());
    if( params.params.isEmpty() )
        lua_pushnil(d_imp->L);
    else
    {
        lua_createtable(d_imp->L,0,params.params.size());
        const int table = lua_gettop(d_imp->L);
        for( int i = 0; i < params.params.size(); i++ )
        {
            lua_pushstring(d_imp->L,params.params[i].first.constData());
            if( params.params[i].second.isEmpty() )
                lua_pushstring(d_imp->L,"true");
            else
                lua_pushstring(d_imp->L,params.params[i].second.constData());
            lua_rawset(d_imp->L,table);
        }
    }
    const int err = lua_pcall(d_imp->L,3,0,0);
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

int Engine::findModule(const QString& path) const
{
    int res = 0;
    lua_getglobal(d_imp->L,"#refs");
    if( lua_istable(d_imp->L,-1) )
    {
        lua_pushstring(d_imp->L,path.toUtf8().constData());
        lua_rawget(d_imp->L,-2);
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            res = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    return res;
}

QList<int> Engine::findDeclByPos(const QString& path, int row, int col) const
{
    QList<int> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    lua_getglobal(d_imp->L,"#xref");
    if( lua_istable(d_imp->L,-1) )
    {
        // filepath -> list_of_idents{ rowcol -> set_of_decls }
        lua_pushstring(d_imp->L,path.toUtf8().constData());
        lua_rawget(d_imp->L,-2);
        if( lua_istable(d_imp->L,-1) )
        {
            const int list_of_idents = lua_gettop(d_imp->L);
            lua_pushinteger(d_imp->L, bs_torowcol(row,col) );
            lua_rawget(d_imp->L,list_of_idents);
            if( lua_istable(d_imp->L,-1) )
            {
                const int set_of_decls = lua_gettop(d_imp->L);
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_decls) != 0)
                {
                    const int key = lua_gettop(d_imp->L)-1;
                    const int ref = assureRef(key);
                    if( ref )
                        res << ref;
                    lua_pop(d_imp->L,1);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<Engine::AllLocsInFile> Engine::findAllLocsOf(int id) const
{
    const int top = lua_gettop(d_imp->L);
    QList<Engine::AllLocsInFile> res;
    if( d_imp->ok() && pushInst(id) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#name");
        const QString name = QString::fromUtf8(lua_tostring(d_imp->L,-1));
        lua_pop(d_imp->L,1);
        lua_getfield(d_imp->L,decl,"#xref");
        const int refs = lua_gettop(d_imp->L);
        if( lua_istable(d_imp->L,refs) )
        {
            lua_pushnil(d_imp->L);  /* first key */
            while (lua_next(d_imp->L, refs) != 0)
            {
                AllLocsInFile a;
                a.d_file = QString::fromUtf8(lua_tostring(d_imp->L,-2));
                const int set_of_rowcol = lua_gettop(d_imp->L);
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_rowcol) != 0)
                {
                    const int rowCol = lua_tointeger(d_imp->L,-2);
                    Loc l;
                    l.d_row = bs_torow(rowCol);
                    l.d_col = bs_tocol(rowCol) + 1;
                    l.d_len = name.size();
                    a.d_locs.append(l);
                    lua_pop(d_imp->L, 1);
                }
                lua_pop(d_imp->L, 1);
                res.append(a);
            }
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<Engine::Loc> Engine::findDeclInstsInFile(const QString& path, int id) const
{
    const int top = lua_gettop(d_imp->L);
    QList<Engine::Loc> res;
    if( d_imp->ok() && pushInst(id) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#name");
        const QString name = QString::fromUtf8(lua_tostring(d_imp->L,-1));
        lua_pop(d_imp->L,1);
        lua_getfield(d_imp->L,decl,"#xref");
        const int refs = lua_gettop(d_imp->L);
        if( lua_istable(d_imp->L,refs) )
        {
            lua_pushstring(d_imp->L,path.toUtf8().constData());
            lua_rawget(d_imp->L,refs);
            const int set_of_rowcol = lua_gettop(d_imp->L);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_rowcol) != 0)
                {
                    const int key = lua_gettop(d_imp->L)-1;
                    const int rowCol = lua_tointeger(d_imp->L,key);
                    Loc loc;
                    loc.d_row = bs_torow(rowCol);
                    loc.d_col = bs_tocol(rowCol)+1;
                    loc.d_len = name.size();
                    res << loc;
                    lua_pop(d_imp->L, 1);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
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

QList<int> Engine::getAllDecls(int module) const
{
    QList<int> res;
    if( d_imp->ok() && pushInst(module) )
    {
        const int decl = lua_gettop(d_imp->L);
        const int n = lua_objlen(d_imp->L,decl);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,decl,i);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_getfield(d_imp->L,-1,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_ModuleDef || k == BS_ClassDecl || k == BS_EnumDecl ||
                        k == BS_VarDecl || k == BS_FieldDecl || k == BS_MacroDef )
                {
                    const int id = assureRef(-1);
                    if( id )
                        res.append(id);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
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

static void fetchConfig(lua_State* L,int inst, const char* field, QStringList& result, bool isPath)
{
    lua_getfield(L,inst,"configs");
    const int configs = lua_gettop(L);
    size_t i;
    for( i = 1; i <= lua_objlen(L,configs); i++ )
    {
        lua_rawgeti(L,configs,i);
        const int config = lua_gettop(L);
        // TODO: check for circular deps
        fetchConfig(L,config,field,result, isPath);
        lua_pop(L,1); // config
    }
    lua_pop(L,1); // configs

    bs_getModuleVar(L,inst,"#dir");
    const int absDir = lua_gettop(L);

    lua_getfield(L,inst,field);
    const int list = lua_gettop(L);

    for( i = 1; i <= lua_objlen(L,list); i++ )
    {
        lua_rawgeti(L,list,i);
        const int item = lua_gettop(L);
        if( isPath && *lua_tostring(L,-1) != '/' )
        {
            // relative path
            if( bs_add_path(L,absDir,item) == 0 )
                lua_replace(L,item);
        }
        if( isPath )
            result << QString::fromUtf8( bs_denormalize_path(lua_tostring(L,item)) );
        else
            result << QString::fromUtf8( lua_tostring(L,item) );
        lua_pop(L,1); // path
    }
    lua_pop(L,2); // absDir, list
}

QStringList Engine::getIncludePaths(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"include_dirs",res,true);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getDefines(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"defines",res,false);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getCppFlags(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"cflags",res,false);
        fetchConfig(d_imp->L,inst,"cflags_cc",res,false);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getCFlags(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"cflags",res,false);
        fetchConfig(d_imp->L,inst,"cflags_c",res,false);
        lua_pop(d_imp->L,2);
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

QString Engine::getPath(int def, const char* field) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        const QByteArray res = lua_tostring(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return QString::fromUtf8(bs_denormalize_path(res.constData()));
    }
    return QString();
}

int Engine::getObject(int def, const char* field) const
{
    const int top = lua_gettop(d_imp->L);
    int res = 0;
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        if( lua_istable(d_imp->L,-1) )
        {
            const int table = lua_gettop(d_imp->L);
            res = assureRef(table);
        }
        lua_pop(d_imp->L,2);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return res;
}

int Engine::getOwningModule(int def) const
{
    const int top = lua_gettop(d_imp->L);
    int res = 0;
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,"#owner");
        const int table = lua_gettop(d_imp->L);
        while( !lua_isnil(d_imp->L,table) )
        {
            lua_getfield(d_imp->L,table,"#kind");
            const int k = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
            if( k == BS_ModuleDef )
            {
                lua_getfield(d_imp->L,table,"#ref");
                res = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                lua_pushnil(d_imp->L);
                lua_replace(d_imp->L,table);
            }else
            {
                lua_getfield(d_imp->L,table,"#owner");
                lua_replace(d_imp->L,table);
            }
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return res;
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

int Engine::assureRef(int table) const
{
    int res = 0;
    const int top = lua_gettop(d_imp->L);
    if( table <= 0 )
        table += top + 1;
    lua_getfield(d_imp->L,table,"#ref");
    if( lua_isnil(d_imp->L,-1) )
    {
        // the object doesn't have a ref, so create and register one
        lua_getglobal(d_imp->L,"#refs");
        const int refs = lua_gettop(d_imp->L);
        res = lua_objlen(d_imp->L,refs) + 1;
        lua_pushinteger(d_imp->L,res);
        lua_setfield(d_imp->L,table,"#ref");
        lua_pushvalue(d_imp->L,table);
        lua_rawseti(d_imp->L,refs,res);
        lua_pop(d_imp->L,1);
    }else
        res = lua_tointeger(d_imp->L,-1);
    lua_pop(d_imp->L,1);
    return res;
}

