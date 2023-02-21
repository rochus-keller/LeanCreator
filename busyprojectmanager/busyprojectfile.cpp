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

#include "busyprojectfile.h"

#include "busyproject.h"
#include "busyprojectmanagerconstants.h"

namespace BusyProjectManager {
namespace Internal {

BusyProjectFile::BusyProjectFile(BusyProject *parent, QString fileName) : Core::IDocument(parent),
    m_project(parent)
{
    setId("Busy.ProjectFile");
    setMimeType(QLatin1String(Constants::PROJECT_MIME_TYPE));
    setFilePath(Utils::FileName::fromString(fileName));
}

bool BusyProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

QString BusyProjectFile::defaultPath() const
{
    return QString();
}

QString BusyProjectFile::suggestedFileName() const
{
    return QString();
}

bool BusyProjectFile::isModified() const
{
    return false;
}

bool BusyProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior BusyProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state);
    Q_UNUSED(type);
    return BehaviorSilent;
}

bool BusyProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_project->delayParsing();
    return true;
}

} // namespace Internal
} // namespace BusyProjectManager

