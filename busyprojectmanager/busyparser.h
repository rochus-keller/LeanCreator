/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUSYPARSER_H
#define BUSYPARSER_H

#include "busyprojectmanager_global.h"

#include <projectexplorer/ioutputparser.h>

#include <QDir>

namespace BusyProjectManager {
namespace Internal {

class BusyParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit BusyParser();

    void setWorkingDirectory(const QString &workingDirectory);

public slots:
    void taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skipLines);

private:
    QDir m_workingDirectory;
};

class BusyParamParser
{
public:
    struct Result
    {
        QList<QPair<QByteArray,QByteArray> > d_params;
        QString d_err;
        int d_row, d_col, d_len; // error token
        Result():d_row(0),d_col(0),d_len(0) {}
        bool hasErrors() const { return !d_err.isEmpty(); }
    };

    static Result parse(const QString& params);
private:
    class Imp;
};

class BusyTargetParser : public QObject
{
public:
    struct Result
    {
        QByteArrayList d_targets;
        QString d_err;
        int d_row, d_col, d_len; // error token
        Result():d_row(0),d_col(0),d_len(0) {}
        bool hasErrors() const { return !d_err.isEmpty(); }
    };

    static Result parse(const QString& targets);
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYPARSER_H
