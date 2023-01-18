/*
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#include "busyapi.h"
#include "busy/Engine.h"
extern "C" {
#include <bshost.h>
#include <bsparser.h>
}
#include <QFileInfo>
using namespace busy;

class Internal::ModuleImp : public QSharedData
{
public:
    Engine::Ptr d_eng;
    int d_id;
};

Module::Module(Engine* eng, int id)
{
    d_imp = new Internal::ModuleImp();
    d_imp->d_eng = eng;
    d_imp->d_id = id;
}

Module::Module()
{

}

Module::Module(const Module& other) : d_imp(other.d_imp)
{

}

Module&Module::operator=(const Module& other)
{
    d_imp = other.d_imp;
    return *this;
}

Module::~Module()
{

}

bool Module::isValid() const
{
    return d_imp.data() != 0;
}

QString Module::name() const
{
    if( !isValid() )
        return QString();
    return QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"#name") );
}

CodeLocation Module::location() const
{
    CodeLocation res;
    if( !isValid() )
        return res;
    const int owner = d_imp->d_eng->getOwner(d_imp->d_id);
    const QByteArray tmp = d_imp->d_eng->getString(owner ? owner : d_imp->d_id,"#file");
    res.d_path = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    if( owner )
    {
        res.d_col = d_imp->d_eng->getInteger(d_imp->d_id,"#col") + 1;
        res.d_row = d_imp->d_eng->getInteger(d_imp->d_id,"#row");
    }
    return res;
}

QString Module::busyFile() const
{
    QString res;
    if( !isValid() )
        return res;
    const QByteArray tmp = d_imp->d_eng->getString(d_imp->d_id,"#file");
    res = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    return res;
}

QString Module::buildDirectory() const
{
    return QString();
}

QList<Product> Module::products() const
{
    QList<Product> res;
    if( !isValid() )
        return res;
    QList<int> ids = d_imp->d_eng->getAllProducts(d_imp->d_id,true);
    for( int i = 0; i < ids.size(); i++ )
        res << Product(d_imp->d_eng.data(),ids[i]);
    return res;
}

QList<Module> Module::subModules() const
{
    QList<Module> res;
    if( !isValid() )
        return res;
    QList<int> ids = d_imp->d_eng->getSubModules(d_imp->d_id);
    for( int i = 0; i < ids.size(); i++ )
        res << Module(d_imp->d_eng.data(),ids[i]);
    return res;
}

QSet<QString> Module::buildSystemFiles() const
{
    // TODO: not sure whether this is supposed to return all BUSY files of the project or just
    // the one of the module; in any case the result is apparently not needed
    QSet<QString> res;
    // res << busyFile();
    return res;
}

class Internal::ProjectImp : public QSharedData
{
public:
    QString d_path;
    Engine::Ptr d_eng;
};

Project::Project()
{

}

Project::Project(const QString& path)
{
    d_imp = new Internal::ProjectImp();
    if( path.endsWith("BUSY") )
        d_imp->d_path = QFileInfo(path).absolutePath();
    else
        d_imp->d_path = path;
    d_imp->d_eng = new Engine();
}

Project::Project(const Project& other) : d_imp(other.d_imp)
{

}

Project&Project::operator=(const Project& other)
{
    d_imp = other.d_imp;
    return *this;
}

Project::~Project()
{

}

bool Project::isValid() const
{
    return d_imp->d_eng;
}

bool Project::parse()
{
    if( !isValid() )
        return false;
    return d_imp->d_eng->parse(d_imp->d_path.toUtf8());
}

ErrorInfo Project::errors() const
{
    return ErrorInfo(); // TODO
}

Module Project::topModule() const
{
    if( !isValid() )
        return Module();
    const int ref = d_imp->d_eng->getRootModule();
    if( ref )
        return Module(d_imp->d_eng.data(), ref);
    else
        return Module();
}

class Internal::ProductImp : public QSharedData
{
public:
    Engine::Ptr d_eng;
    int d_id;
};

Product::Product(Engine* eng, int id)
{
    d_imp = new Internal::ProductImp();
    d_imp->d_eng = eng;
    d_imp->d_id = id;
}


Product::Product()
{

}

Product::Product(const Product &other): d_imp(other.d_imp)
{

}

Product &Product::operator=(const Product &other)
{
    d_imp = other.d_imp;
    return *this;
}

bool Product::operator==( const Product& rhs ) const
{
    return isValid() && d_imp->d_id == rhs.d_imp->d_id;
}

Product::~Product()
{
}

bool Product::isValid() const
{
    return d_imp.data() != 0;
}

QString Product::name(bool altName) const
{
    if( !isValid() )
        return QString();
    if( altName )
    {
        const QString name = QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"name",true) );
        if( !name.isEmpty() )
            return name;
    }
    return QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"#name") );
}

QString Product::profile() const
{
    return QString();
}

CodeLocation Product::location() const
{
    CodeLocation res;
    if( !isValid() )
        return res;
    const int owner = d_imp->d_eng->getOwner(d_imp->d_id);
    if( owner == 0 )
        return res;
    const QByteArray tmp = d_imp->d_eng->getString(owner,"#file");
    res.d_path = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    res.d_col = d_imp->d_eng->getInteger(d_imp->d_id,"#col") + 1;
    res.d_row = d_imp->d_eng->getInteger(d_imp->d_id,"#row");
    return res;
}

QList<TargetArtifact> Product::targetArtifacts() const
{
    return QList<TargetArtifact>();
}

QVariantMap Product::properties() const
{
    return QVariantMap();
}

bool Product::isEnabled() const
{
    return true;
}

bool Product::isRunnable() const
{
    if( !isValid() )
        return false;
    return d_imp->d_eng->isExecutable(d_imp->d_id);
}

QStringList Product::allFilePaths() const
{
    if( !isValid() )
        return QStringList();
    return d_imp->d_eng->getAllSources(d_imp->d_id);
}

PropertyMap Product::properties2() const
{
    return PropertyMap();
}

static void walkAllProducts(Engine* eng, int module, QList<int>& res, bool onlyRunnables )
{
    QList<int> subs = eng->getSubModules(module);
    for(int i = 0; i < subs.size(); i++ )
        walkAllProducts(eng,subs[i],res,onlyRunnables);
    res += eng->getAllProducts(module,true, onlyRunnables);
}

QList<Product> Project::allProducts(bool onlyRunnables) const
{
    if( !isValid() )
        return QList<Product>();
    QList<int> ids;
    walkAllProducts(d_imp->d_eng.data(),d_imp->d_eng->getRootModule(),ids, onlyRunnables);
    QList<Product> res;
    for( int i = 0; i < ids.size(); i++ )
        res << Product(d_imp->d_eng.data(),ids[i]);
    return res;
}

