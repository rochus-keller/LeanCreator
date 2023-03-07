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

#include "busyBuilder.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QtDebug>
#include <cpptools/cppmodelmanager.h>
extern "C" {
#include <bsvisitor.h>
#include <lua.h>

static void  Builder_lua_reporter(const char* msg, void* data)
{
    QStringList* out = (QStringList*)data;
    out->append(QString::fromUtf8(msg));
}
}
using namespace busy;

class Builder::Runner : public QThread
{
public:
    QString d_program;
    QStringList d_arguments;
    QProcessEnvironment d_env;
    QString d_workdir;
    QStringList d_stdErr;
    bool d_success;

    static QStringList convert(const QByteArray& str)
    {
        QStringList res;
        QByteArrayList l = str.split('\n');
        foreach( const QByteArray& s, l )
#ifdef _WIN32
            res << QString::fromLocal8Bit( s.trimmed() );
#else
            res << QString::fromUtf8( s.trimmed() );
#endif
        return res;
    }

    static QString toDefine( const QByteArray& str )
    {
        QString res = QString::fromUtf8(str);
        res.replace("\\\\", "\\" );
        res.replace("\\\"", "\"");
        return res;
    }

    static bool isHeader( const QByteArray& file )
    {
        return file.endsWith(".h") || file.endsWith(".hh") || file.endsWith(".h") ||
                file.endsWith(".H") || file.endsWith(".HPP") || file.endsWith(".hpp") ||
                file.endsWith(".h++") || file.endsWith(".hp") || file.endsWith(".hxx");
    }

    void prepare( const Operation& op )
    {
        d_stdErr.clear();
        d_success = true;

        if( op.op == BS_Copy )
            d_program = "copy";
        else if( op.op == BS_RunLua )
            d_program = "lua";
        else
            d_program = QString::fromUtf8(op.cmd);

        QByteArrayList values;
        QByteArray value;
        QStringList params;
        // TODO eliminate duplicates
        switch(op.op)
        {
        case BS_Compile:
            values = op.getParams(BS_cflag);
            foreach(const QByteArray& v, values )
                params << QString::fromUtf8(v);
            values = op.getParams(BS_define);
            foreach(const QByteArray& v, values )
                params << QString("-D%1").arg(toDefine(v));
            values = op.getParams(BS_include_dir);
            foreach(const QByteArray& v, values )
                params << QString("-I%1").arg(QString::fromUtf8(v));
            switch(op.tc)
            {
            case BS_gcc:
            case BS_clang:
                params << "-c" << "-o";
                params << QString::fromUtf8(op.getOutfile());
                params << QString::fromUtf8(op.getInfile());
                break;
            case BS_msvc:
                params << "/nologo" << "/c";
                params << QString("/Fo%1").arg(QString::fromUtf8(op.getOutfile()));
                params << QString::fromUtf8(op.getInfile());
                break;
            }
            break;
        case BS_LinkExe:
        case BS_LinkDll:
            if( op.tc == BS_msvc )
                params << "/nologo";
            if( op.op == BS_LinkDll)
            {
                switch(op.tc)
                {
                case BS_gcc:
                case BS_clang:
                    if( op.os == BS_mac )
                        params << "-dynamiclib";
                    else
                        params << "-shared";
                    break;
                case BS_msvc:
                    params << "/dll";
                    break;
                }
            }
            values = op.getParams(BS_ldflag);
            foreach(const QByteArray& v, values )
                params << QString::fromUtf8(v);

            if( op.os == BS_windows )
            {
                // NOTE: on Windows QProcess::start internally calls CreateProcess which only supports command
                // lines up to a length of 32'767 unicode chars (including terminating zero)

                QFileInfo info(QString::fromUtf8( op.getOutfile() ));
                QFile rsp(info.absoluteDir().absoluteFilePath(info.completeBaseName() + ".rsp") );
                if( !rsp.open(QIODevice::WriteOnly) )
                    qCritical() << "cannot open rsp file for writing:" << rsp.fileName(); // TODO
                values = op.getInFiles();
                foreach(const QByteArray& v, values )
                {
                    if( op.tc == BS_msvc && v.endsWith(".dll") )
                    {
                        // add .lib because msvc requires an import library to use the dll
                        rsp.write(v);
                        rsp.write(".lib");
                    }else
                        rsp.write(v);
                    rsp.write(" ");
                }
                params << QString("@%1").arg(rsp.fileName());
            }else
            {
                values = op.getInFiles();
                foreach(const QByteArray& v, values )
                {
                    if( op.tc == BS_msvc && v.endsWith(".dll") )
                        // add .lib because msvc requires an import library to use the dll
                        params << QString("%1.lib").arg(QString::fromUtf8(v));
                    else
                        params << QString::fromUtf8(v);
                }
            }

            values = op.getParams(BS_lib_file);
            foreach(const QByteArray& v, values )
                params << QString::fromUtf8(v);
            values = op.getParams(BS_lib_dir);
            foreach(const QByteArray& v, values )
            {
                if( op.tc == BS_msvc )
                    params << QString("/libpath:%1").arg(QString::fromUtf8(v));
                else
                    params << QString("-L%1").arg(QString::fromUtf8(v));
            }
            values = op.getParams(BS_lib_name);
            foreach(const QByteArray& v, values )
            {
                if( op.tc == BS_msvc )
                    params << QString("%1.lib").arg(QString::fromUtf8(v));
                else
                    params << QString("-l%1").arg(QString::fromUtf8(v));
            }
            values = op.getParams(BS_framework);
            foreach(const QByteArray& v, values )
                params << "-framework" << QString::fromUtf8(v);
            values = op.getParams(BS_defFile);
            foreach(const QByteArray& v, values )
            {
                if( op.tc == BS_msvc )
                    params << QString("/def:%1").arg(QString::fromUtf8(v));
                else
                    params << QString::fromUtf8(v);
            }
            value = op.getOutfile();
            switch(op.tc)
            {
            case BS_gcc:
            case BS_clang:
                params << "-o" << QString::fromUtf8(value);
                break;
            case BS_msvc:
                params << QString("/out:%1").arg(QString::fromUtf8(value));
                break;
            }
            break;
        case BS_LinkLib:
            value = op.getOutfile();
            switch(op.tc)
            {
            case BS_gcc:
                params << "r" << QString::fromUtf8(value);
                break;
            case BS_clang:
                if( op.os == BS_windows )
                {
                    params << "/nologo";
                    params << QString("/out:%1").arg(QString::fromUtf8(value));
                }else
                    params << "r" << QString::fromUtf8(value);
                break;
            case BS_msvc:
                params << "/nologo" ;
                params << QString("/out:%1").arg(QString::fromUtf8(value));
                break;
            }
            if( op.os == BS_windows )
            {
                // NOTE: on Windows QProcess::start internally calls CreateProcess which only supports command
                // lines up to a length of 32'767 unicode chars (including terminating zero)

                QFileInfo info(QString::fromUtf8( op.getOutfile() ));
                QFile rsp(info.absoluteDir().absoluteFilePath(info.completeBaseName() + ".rsp") );
                if( !rsp.open(QIODevice::WriteOnly) )
                    qCritical() << "cannot open rsp file for writing:" << rsp.fileName(); // TODO
                values = op.getInFiles();
                foreach(const QByteArray& v, values )
                {
                    rsp.write(v);
                    rsp.write(" ");
                }
                params << QString("@%1").arg(rsp.fileName());
            }else
            {
                values = op.getInFiles();
                foreach(const QByteArray& v, values )
                    params << QString::fromUtf8(v);
            }
            break;
        case BS_RunMoc:
            {
                const QByteArray infile = op.getInfile();
                params << QString::fromUtf8(infile);
                params << "-o" << QString::fromUtf8(op.getOutfile());
                values = op.getParams(BS_define);
                foreach(const QByteArray& v, values )
                    params << "-D" << toDefine(v);
                if( isHeader(infile) )
                {
                    QFileInfo info(QString::fromUtf8(infile));
                    const QString privheader = info.absoluteDir().absoluteFilePath(
                                QString("%1_p.h").arg(info.completeBaseName()));
                    if( QFileInfo(privheader).exists() )
                    {
                        params << "-p" << info.absolutePath();
                        params << "-b" << QString("%1_p.h").arg(info.completeBaseName());
                    }
                }
            }
            break;
        case BS_RunRcc:
            params << QString::fromUtf8(op.getInfile());
            params << "-o" << QString::fromUtf8(op.getOutfile());
            params << "-name" << QString::fromUtf8(op.getParam(BS_name));
            break;
        case BS_RunUic:
            params << QString::fromUtf8(op.getInfile());
            params << "-o" << QString::fromUtf8(op.getOutfile());
            break;
        case BS_RunLua:
            params << QString::fromUtf8(op.getInfile());
            values = op.getParams(BS_arg);
            foreach(const QByteArray& v, values )
                params << QString::fromUtf8(v);
            break;
        case BS_Copy:
            params << QString::fromUtf8(op.getInfile());
            params << QString::fromUtf8(op.getOutfile());
            break;
        }
        d_arguments = params;
    }

    void run()
    {
        if( d_program == "copy" )
        {
            bool ok = true;
            QFileInfo info(d_arguments[1]);
            if( info.exists() )
                ok = QFile::remove(d_arguments[1]);
            QDir(d_workdir).mkpath(info.absolutePath());
            if( ok )
                ok = QFile::copy(d_arguments[0],d_arguments[1]);
            if( !ok )
            {
                d_success = false;
                d_stdErr << "cannot copy files";
            }
        }else if( d_program == "lua" )
        {
            QByteArrayList args;
            foreach( const QString& arg, d_arguments )
                args << arg.toUtf8();
            const int argc = args.size() + 1;
            QVector<const char*> argv(argc + 1);
            argv[0] = "lua";
            for( int i = 0; i < args.size(); i++ )
                argv[i+1] = args[i].constData();
            argv[argc] = 0;

            d_success == (lua_main_with_reporter(argc, const_cast<char**>(argv.data()),
                                                Builder_lua_reporter, &d_stdErr ) == 0);
        }else
        {
            QProcess proc;
            proc.setProcessEnvironment(d_env);
            proc.setArguments(d_arguments);
            proc.setProgram(d_program);
            proc.setWorkingDirectory(d_workdir);
            proc.start();
            if( proc.waitForStarted() )
            {
                if( !proc.waitForFinished(240000) )
                {
                    d_success = false;
                    d_stdErr << "process timeout";
                }else
                {
                    d_success = proc.exitCode() == 0;
                    if( !d_success )
                        d_stdErr = convert( proc.readAllStandardError() ) + convert( proc.readAllStandardOutput() );
                }
            }else
            {
                d_success = false;
                d_stdErr << "cannot start process" << proc.errorString();
            }
        }
    }

    Runner(QObject* p):QThread(p),d_success(true) {}
};

Builder::Builder(int threadCount, bool stopOnError, bool trackHeaders, QObject *parent)
    : QThread(parent),d_stopOnError(stopOnError), d_trackHeaders(trackHeaders)
{
    connect(this,SIGNAL(started()), this, SLOT(onStarted()), Qt::QueuedConnection );
    d_pool.resize(threadCount);
    for( int i = 0; i < d_pool.size(); i++ )
    {
        d_pool[i] = new Runner(this);
        connect(d_pool[i],SIGNAL(finished()), this, SLOT(onFinished()));
    }
}

void Builder::start(const Builder::OpList& work,
                    const QString& sourcedir, const QString& workdir,
                    const QProcessEnvironment& env)
{
    if( isRunning() )
        return;
    d_work = work;
    d_env = env;
    d_workdir = workdir;
    d_sourcedir = sourcedir;
    d_success = true;
    d_cancel = false;
    d_quitting = false;
    d_curGroup = 0;
    d_done = 0;
    d_title.clear();
    d_available.clear();
    for( int i = 0; i < d_pool.size(); i++ )
        d_available.append(d_pool[i]);

    if( d_trackHeaders )
        d_deps = CppTools::CppModelManager::instance()->snapshot().dependencyTable();

    QThread::start();
}

void Builder::onCancel()
{
    d_cancel = true;
    for( int i = 0; i < d_pool.size(); i++ )
        d_pool[i]->wait();
    if( !isRunning() )
        return;
    emit taskFinished(false);
    quit();
}

void Builder::onFinished()
{
    Runner* r = (Runner*) sender();
    //qDebug() << "finished" << r;
    emit reportResult(r->d_success, r->d_stdErr );
    if( !r->d_success )
        d_success = false;
    if( d_cancel )
        return;
    d_available.push_back(r);
    select();
}

void Builder::onStarted()
{
    if( d_cancel )
        return;
    emit taskStarted("BUSY build run", d_work.size() );
    select();
}

void Builder::onQuit()
{
    emit taskFinished(d_success);
    quit();
}

void Builder::select()
{
    if( d_quitting )
        return;

    if( d_work.isEmpty() || (d_stopOnError && !d_success ) )
    {
        d_quitting = true;
        for( int i = 0; i < d_pool.size(); i++ )
            d_pool[i]->wait();
        // now all onFinished signals are in the queue
        QMetaObject::invokeMethod(this,"onQuit");
        return;
    }

    if( d_available.isEmpty() )
        return; // should never happen

    while( !d_work.isEmpty() )
    {
        bool startedThread = false;

        while( !d_work.isEmpty() && d_work.first().group != d_curGroup && d_available.size() == d_pool.size() )
        {
            // we switch group if all tasks of the old group are finished
            d_curGroup = d_work.first().group;
            startedThread = startOne();
        }

        while( !d_work.isEmpty() && d_work.first().group == d_curGroup && !d_available.isEmpty() )
        {
            // start next item in group
            startedThread = startOne();
        }

        if( startedThread ||
                // we're waiting for the last thread of the group to finish:
                ( !d_work.isEmpty() && !startedThread && d_work.first().group != d_curGroup
                  && d_available.size() != d_pool.size() ) )
            break;
    }

    if( d_work.isEmpty() && d_available.size() == d_pool.size() )
    {
        // we get here if non of the work was due and no work is left
        d_quitting = true;
        QMetaObject::invokeMethod(this,"onQuit");
    }
}

static void dump(const Builder::Operation& op, int nr, bool due )
{
    QByteArray prefix;
    switch(op.op)
    {
    case BS_Compile:
        prefix = "COMPILE: ";
        break;
    case BS_LinkExe:
    case BS_LinkDll:
    case BS_LinkLib:
        prefix = "LINK: ";
        break;
    case BS_RunMoc:
        prefix = "MOC: ";
        break;
    case BS_RunRcc:
        prefix = "RCC: ";
        break;
    case BS_RunUic:
        prefix = "UIC: ";
        break;
    case BS_RunLua:
        prefix = "LUA: ";
        break;
    case BS_Copy:
        prefix = "COPY: ";
        break;
    default:
        prefix = "BEGIN OP: " + QByteArray::number(op.op) + " ";
        break;
    }
    qDebug() << (due ? "!" : "." ) << nr << prefix.constData() << op.group << op.getOutfile().constData();
}

bool Builder::startOne()
{
    Operation op = d_work.takeFirst();
    emit taskProgress(++d_done);

    if( op.op == BS_EnteringProduct )
    {
        d_title = QString::fromUtf8(op.cmd);
        return false;
    }

    const bool due = isDue(op);
    //dump(op, d_done-1,due);
    if( !due )
        return false;

    if( !d_title.isEmpty() )
    {
        emit reportCommandDescription(QString(), QString("    # running %1").arg(d_title) );
        d_title.clear();
    }
    Q_ASSERT( op.op != BS_EnteringProduct );

    Runner* r = d_available.takeFirst();
    //qDebug() << "started" << r;
    r->d_env = d_env;
    r->d_workdir = d_workdir;
    r->prepare(op);
    const QString cmdline = r->d_program + QChar(' ') + r->d_arguments.join(' ');
    emit reportCommandDescription(QString(), QString(4,QChar(' ')) + cmdline );

    r->start();
    return true;
}

bool Builder::isDue(const Builder::Operation& op)
{
    if( op.op == BS_RunLua )
        return true;

    const QByteArray outfile = op.getOutfile();
    if( outfile.isEmpty() )
        return true; // cause an error message by the command
    QFileInfo outinfo( QString::fromUtf8(outfile) );
    if( !outinfo.exists() )
        return true;
    const QDateTime ref = outinfo.lastModified();

    const QByteArrayList infiles = op.getInFiles();
    foreach( const QByteArray& infile, infiles )
    {
        QFileInfo info( QString::fromUtf8(infile) );
        if( infile.isEmpty() || !info.exists() )
            return true; // cause an error message by the command
        if( info.lastModified() > ref )
            return true; // at least one input is newer than existing output
        QString reason;
        if( d_trackHeaders && op.op == BS_Compile &&
                d_deps.anyNewerDeps(info.absoluteFilePath(),ref.toTime_t(), &reason) )
        {
            // also check with include headers (possibly restrict to sourcedir)
            qDebug() << "compiled" << info.fileName() << "because of modified header" << reason; // TEST
            return true;
        }
    }
    return false;
}

QByteArray Builder::Operation::getOutfile() const
{
    return getParam(BS_outfile);
}

QByteArray Builder::Operation::getInfile() const
{
    return getParam(BS_infile);
}

QByteArray Builder::Operation::getParam(int kind) const
{
    foreach( const Parameter& p, params )
    {
        if( p.kind == kind )
            return p.value;
    }
    return QByteArray();
}

QByteArrayList Builder::Operation::getInFiles() const
{
    return getParams(BS_infile);
}

QByteArrayList Builder::Operation::getParams(int kind) const
{
    QByteArrayList res;
    foreach( const Parameter& p, params )
    {
        if( p.kind == kind )
            res << p.value;
    }
    return res;
}
