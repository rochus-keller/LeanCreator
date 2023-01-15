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

#ifndef BUSYPROJECTPARSER_H
#define BUSYPROJECTPARSER_H

#include <utils/environment.h>

#include <QFutureInterface>
#include <QObject>

#include <busy/busyapi.h>

namespace BusyProjectManager {
namespace Internal {

class BusyProject;

class BusyProjectParser : public QObject
{
    Q_OBJECT

public:
    BusyProjectParser(BusyProjectManager::Internal::BusyProject *project,
                     QFutureInterface<bool> *fi);
    ~BusyProjectParser();

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir);
    void cancel();

    busy::Module busyProject() const;
    busy::ErrorInfo error();

signals:
    void done(bool success);

private slots:
    void handleBusyParsingDone(bool success);
    void handleBusyParsingProgress(int progress);
    void handleBusyParsingTaskSetup(const QString &description, int maximumProgressValue);

private:
    QString pluginsBaseDirectory() const;
    QString resourcesBaseDirectory() const;
    QString libExecDirectory() const;

    QString m_projectFilePath;
    busy::SetupProjectJob *m_qbsSetupProjectJob;
    busy::ErrorInfo m_error;
    busy::Module m_project;

    QFutureInterface<bool> *m_fi;
    int m_currentProgressBase;
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYPROJECTPARSER_H
