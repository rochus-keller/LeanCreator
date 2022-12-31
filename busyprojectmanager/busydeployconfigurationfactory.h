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

#ifndef BUSYDEPLOYCONFIGURATIONFACTORY_H
#define BUSYDEPLOYCONFIGURATIONFACTORY_H

#include <projectexplorer/deployconfiguration.h>

namespace BusyProjectManager {


namespace Internal {

class BusyDeployConfigurationFactory;
class BusyInstallStep;

class BusyDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT

public:
    BusyInstallStep *busyInstallStep() const;

private:
    BusyDeployConfiguration(ProjectExplorer::Target *target, Core::Id id);
    BusyDeployConfiguration(ProjectExplorer::Target *target,
                           ProjectExplorer::DeployConfiguration *source);

    friend class BusyDeployConfigurationFactory;
};

class BusyDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit BusyDeployConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(Core::Id id) const;
    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product) const;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *product);
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYDEPLOYCONFIGURATIONFACTORY_H
