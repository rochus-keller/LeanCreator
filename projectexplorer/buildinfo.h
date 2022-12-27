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

#ifndef BUILDINFO_H
#define BUILDINFO_H

#include "projectexplorer_export.h"

#include "buildconfiguration.h"
#include "task.h"

#include <core/id.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {

class IBuildConfigurationFactory;

class PROJECTEXPLORER_EXPORT BuildInfo
{
public:
    BuildInfo(const IBuildConfigurationFactory *f) : m_factory(f) { }
    virtual ~BuildInfo();

    const IBuildConfigurationFactory *factory() const { return m_factory; }

    QString displayName;
    QString typeName;
    Utils::FileName buildDirectory;
    Core::Id kitId;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;

    bool operator==(const BuildInfo &o)
    {
        return displayName == o.displayName && typeName == o.typeName
                && buildDirectory == o.buildDirectory && kitId == o.kitId
                && buildType == o.buildType;
    }

    virtual QList<Task> reportIssues(const QString &projectPath,
                                     const QString &buildDir) const
    {
        Q_UNUSED(projectPath);
        Q_UNUSED(buildDir);
        return QList<Task>();
    }

private:
    const IBuildConfigurationFactory *m_factory;

    friend class IBuildConfigurationFactory;
};

} // namespace ProjectExplorer

#endif // BUILDINFO_H
