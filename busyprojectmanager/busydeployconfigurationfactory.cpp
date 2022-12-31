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

#include "busydeployconfigurationfactory.h"

#include "busyinstallstep.h"
#include "busyproject.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

static QString genericBusyDisplayName() {
    return QCoreApplication::translate("Busy", "Busy Install");
}

static Core::Id genericBusyDeployConfigurationId()
{
    return "Busy.Deploy";
}

// --------------------------------------------------------------------
// BusyDeployConfiguration:
// --------------------------------------------------------------------

BusyInstallStep *BusyDeployConfiguration::busyInstallStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, stepList()->steps()) {
        if (BusyInstallStep *install = qobject_cast<BusyInstallStep *>(bs))
            return install;
    }
    return 0;
}

BusyDeployConfiguration::BusyDeployConfiguration(ProjectExplorer::Target *target, Core::Id id) :
    ProjectExplorer::DeployConfiguration(target, id)
{ }

BusyDeployConfiguration::BusyDeployConfiguration(ProjectExplorer::Target *target,
                                               ProjectExplorer::DeployConfiguration *source) :
    ProjectExplorer::DeployConfiguration(target, source)
{
    cloneSteps(source);
}

// --------------------------------------------------------------------
// BusyDeployConfigurationFactory:
// --------------------------------------------------------------------

BusyDeployConfigurationFactory::BusyDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("BusyDeployConfiguration"));
}

QList<Core::Id> BusyDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    const Core::Id deviceId = ProjectExplorer::DeviceKitInformation::deviceId(parent->kit());
    if (qobject_cast<BusyProject *>(parent->project())
            && deviceId == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        ids << genericBusyDeployConfigurationId();
    }
    return ids;
}

QString BusyDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == genericBusyDeployConfigurationId())
        return genericBusyDisplayName();
    return QString();
}

bool BusyDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                              const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration
*BusyDeployConfigurationFactory::create(ProjectExplorer::Target *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    BusyDeployConfiguration *dc = new BusyDeployConfiguration(parent, id);
    dc->setDisplayName(genericBusyDisplayName());
    return dc;
}

bool BusyDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                               const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration
*BusyDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);
    BusyDeployConfiguration *dc = new BusyDeployConfiguration(parent, id);
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool BusyDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::DeployConfiguration
*BusyDeployConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                      ProjectExplorer::DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new BusyDeployConfiguration(parent, qobject_cast<BusyDeployConfiguration *>(product));
}

} // namespace Internal
} // namespace BusyProjectManager
