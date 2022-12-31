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

#ifndef BUSYPROJECTMANAGER_H
#define BUSYPROJECTMANAGER_H

#include "busyprojectmanager_global.h"

#include <projectexplorer/iprojectmanager.h>

#include <QList>

namespace busy {
class Settings;
class Preferences;
}

#include <QString>
#include <QVariantMap>

namespace ProjectExplorer {
class Kit;
class Project;
class ProjectExplorerPlugin;
} // namespace ProjectExplorer

namespace BusyProjectManager {
namespace Internal {
class DefaultPropertyProvider;
class BusyLogSink;
class BusyProject;

class BusyManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    BusyManager();
    ~BusyManager();

    QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    // BUSY profiles management:
    QString profileForKit(const ProjectExplorer::Kit *k);
    void setProfileForKit(const QString &name, const ProjectExplorer::Kit *k);

    void updateProfileIfNecessary(const ProjectExplorer::Kit *kit);

    static busy::Settings *settings() { return m_settings; }
    static Internal::BusyLogSink *logSink() { return m_logSink; }
    static BusyManager *instance() { return m_instance; }

private:
    void addProfile(const QString &name, const QVariantMap &data);
    void addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k);
    void addProfileFromKit(const ProjectExplorer::Kit *k);

    void handleKitUpdate(ProjectExplorer::Kit *kit);
    void handleKitRemoval(ProjectExplorer::Kit *kit);

    static BusyLogSink *m_logSink;
    static busy::Settings *m_settings;

    DefaultPropertyProvider *m_defaultPropertyProvider;
    QList<ProjectExplorer::Kit *> m_kitsToBeSetupForBusy;
    static BusyManager *m_instance;
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYPROJECTMANAGER_H
