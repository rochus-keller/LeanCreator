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

#include "configtaskhandler.h"

#include "task.h"
#include "taskhub.h"

#include <core/icore.h>

#include <QAction>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ConfigTaskHandler::ConfigTaskHandler(const Task &pattern, Core::Id page) :
    m_pattern(pattern),
    m_targetPage(page)
{ }

bool ConfigTaskHandler::canHandle(const Task &task) const
{
    return task.description == m_pattern.description
            && task.category == m_pattern.category;
}

void ConfigTaskHandler::handle(const Task &task)
{
    Q_UNUSED(task);
    Core::ICore::showOptionsDialog(m_targetPage);
}

QAction *ConfigTaskHandler::createAction(QObject *parent) const
{
    QAction *action = new QAction(Core::ICore::msgShowOptionsDialog(), parent);
    action->setToolTip(Core::ICore::msgShowOptionsDialogToolTip());
    return action;
}
