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

#include "vcsannotatetaskhandler.h"

#include "task.h"

#include <core/iversioncontrol.h>
#include <core/vcsmanager.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QFileInfo>

using namespace Core;

namespace ProjectExplorer {
namespace Internal {

bool VcsAnnotateTaskHandler::canHandle(const Task &task) const
{
    QFileInfo fi(task.file.toFileInfo());
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(fi.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const Task &task)
{
    QFileInfo fi(task.file.toFileInfo());
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(fi.absolutePath());
    QTC_ASSERT(vc, return);
    QTC_ASSERT(vc->supportsOperation(IVersionControl::AnnotateOperation), return);
    vc->vcsAnnotate(fi.absoluteFilePath(), task.movedLine);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent) const
{
    QAction *vcsannotateAction = new QAction(tr("&Annotate"), parent);
    vcsannotateAction->setToolTip(tr("Annotate using version control system."));
    return vcsannotateAction;
}

} // namespace Internal
} // namespace ProjectExplorer
