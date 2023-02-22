#ifndef BUSYENGINE_H
#define BUSYENGINE_H

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

#include <QSharedData>
#include <QString>
#include <bslogger.h>

extern "C" {
typedef int (*BSRunCmd)(const char* cmd, void* data);
}
namespace busy
{
class Engine : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<Engine> Ptr;
    struct Loc
    {
        int d_row;
        short d_col, d_len;
        Loc():d_row(0),d_col(0),d_len(0){}
    };
    struct AllLocsInFile
    {
        QList<Loc> d_locs;
        QString d_file;
    };

    Engine();
    ~Engine();

    void registerLogger(BSLogger,void* data);

    struct ParseParams
    {
        QByteArray root_source_dir;
        QByteArray root_build_dir;
        QByteArray build_mode; // optimized, nonoptimized, debug
        QByteArray toolchain_path;
        QByteArray toolchain_prefix;
        QByteArray os;
        QByteArray toolchain; // gcc, msvc, clang
        QByteArray cpu;
        QByteArray wordsize;
        QList<QPair<QByteArray,QByteArray> > params;
        QByteArrayList targets;
    };

    bool parse( const ParseParams& params, bool checkTargets = true );
    bool build( const QByteArrayList& targets, BSRunCmd, void* data );
    QByteArrayList generateBuildCommands(const QByteArrayList& targets = QByteArrayList());
    int getRootModule() const;
    int findModule(const QString& path) const; // TODO: path can point to more than one module
    QList<int> findDeclByPos(const QString& path, int row, int col ) const;
    QString findPathByPos(const QString& path, int row, int col) const;
    QList<Loc> findDeclInstsInFile(const QString& path, int decl) const;
    QList<AllLocsInFile> findAllLocsOf(int decl) const;
    QList<int> getSubModules(int) const;
    QList<int> getAllProducts(int module, bool withSourceOnly = false, bool runnableOnly = false, bool onlyActives = false) const;
    QList<int> getAllDecls(int module) const;
    QStringList getAllSources(int product) const;
    QStringList getIncludePaths(int product) const;
    QStringList getDefines(int product) const;
    QStringList getCppFlags(int product) const;
    QStringList getCFlags(int product) const;
    bool isExecutable(int) const;
    bool isActive(int) const;
    QByteArray getString(int def, const char* field, bool inst = false) const;
    QByteArray getDeclPath(int decl) const;
    int getInteger(int def, const char* field) const;
    QString getPath(int def, const char* field) const;
    int getObject(int def, const char* field) const;
    int getGlobals() const;
    int getOwningModule(int def) const;
    int getOwner(int def) const;
    void dump(int def, const char* title = "") const;
protected:
    bool pushInst(int ref) const;
    int assureRef(int table) const;
private:
    class Imp;
    Imp* d_imp;
};
}

#endif // BUSYENGINE_H
