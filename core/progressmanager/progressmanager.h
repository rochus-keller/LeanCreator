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

#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include <core/core_global.h>
#include <core/id.h>

#include <QObject>
#include <QFuture>

namespace Core {
class FutureProgress;

namespace Internal { class ProgressManagerPrivate; }

class CORE_EXPORT ProgressManager : public QObject
{
    Q_OBJECT
public:
    enum ProgressFlag {
        KeepOnFinish = 0x01,
        ShowInApplicationIcon = 0x02
    };
    Q_DECLARE_FLAGS(ProgressFlags, ProgressFlag)

    static ProgressManager *instance();

    static FutureProgress *addTask(const QFuture<void> &future, const QString &title,
                                   Id type, ProgressFlags flags = 0);
    static FutureProgress *addTimedTask(const QFutureInterface<void> &fi, const QString &title,
                                   Id type, int expectedSeconds, ProgressFlags flags = 0);
    static void setApplicationLabel(const QString &text);

public slots:
    static void cancelTasks(Id type);

signals:
    void taskStarted(Core::Id type);
    void allTasksFinished(Core::Id type);

private:
    ProgressManager();
    ~ProgressManager();

    friend class Core::Internal::ProgressManagerPrivate;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::ProgressManager::ProgressFlags)

#endif //PROGRESSMANAGER_H
