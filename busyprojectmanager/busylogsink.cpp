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

#include "busylogsink.h"

#include <busytools/busyapi.h>

#include <core/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QTimer>

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BusyLogSink:
// --------------------------------------------------------------------

BusyLogSink::BusyLogSink(QObject *parent) : QObject(parent)
{
    connect(this, SIGNAL(newTask(ProjectExplorer::Task)),
            ProjectExplorer::TaskHub::instance(),
            SLOT(addTask(ProjectExplorer::Task)), Qt::QueuedConnection);
}

void BusyLogSink::sendMessages()
{
    QStringList toSend;
    {
        QMutexLocker l(&m_mutex);
        toSend = m_messages;
        m_messages.clear();
    }

    foreach (const QString &msg, toSend)
        Core::MessageManager::write(msg);
}

void BusyLogSink::doPrintWarning(const busy::ErrorInfo &warning)
{
    foreach (const busy::ErrorItem &item, warning.items())
        emit newTask(ProjectExplorer::Task(ProjectExplorer::Task::Warning,
                                           item.description(),
                                           Utils::FileName::fromString(item.codeLocation().filePath()),
                                           item.codeLocation().line(),
                                           ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void BusyLogSink::doPrintMessage(busy::LoggerLevel level, const QString &message, const QString &tag)
{
    Q_UNUSED(tag);

    {
        QMutexLocker l(&m_mutex);
        m_messages.append(busy::ILogSink::logLevelTag(level) + message);
    }
    QMetaObject::invokeMethod(this, "sendMessages", Qt::QueuedConnection);
}

} // namespace Internal
} // namespace BusyProjectManager
