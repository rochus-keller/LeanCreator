#ifndef BUSYBUILDER_H
#define BUSYBUILDER_H

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

#include <QThread>
#include <QProcessEnvironment>
#include <QVector>
//#include <cplusplus/DependencyTable.h>

namespace busy
{
class Builder : public QThread
{
    Q_OBJECT
public:
    struct Parameter
    {
        quint8 kind; // BSBuildParam
        QByteArray value;
    };

    struct Operation
    {
        quint32 group;
        quint8 op, tc, os; // BSBuildOperation, BSToolchain, BSOperatingSystem
        QByteArray cmd;
        QList<Parameter> params;

        QByteArray getOutfile() const;
        QByteArray getInfile() const;
        QByteArray getParam(int kind) const;
        QByteArrayList getInFiles() const;
        QByteArrayList getParams(int kind) const;
    };

    typedef QList<Operation> OpList;

    explicit Builder(int threadCount = 1, QObject *parent = 0);

    void start( const OpList&, const QString& sourcedir, const QString& workdir,
                const QProcessEnvironment& env);

signals:
    void taskStarted(const QString& description,int maxValue);
    void taskProgress(int curValue);
    void taskFinished(bool success);
    void reportCommandDescription(const QString& highlight, const QString& message);
    void reportResult( bool success, const QStringList& stdErr );

public slots:
    void onCancel();

protected slots:
    void onFinished();
    void onStarted();
    void onQuit();

protected:
    void select();
    bool startOne();
    bool isDue(const Operation& op);

private:
    class Runner;
    OpList d_work;
    QString d_sourcedir;
    QString d_workdir;
    QProcessEnvironment d_env;
    QVector<Runner*> d_pool;
    QList<Runner*> d_available;
    quint32 d_curGroup;
    quint32 d_done;
    //CPlusPlus::DependencyTable d_deps;
    bool d_success;
    bool d_cancel;
    bool d_quitting;
    bool d_checkHeaders;
};
}

#endif // BUSYBUILDER_H
