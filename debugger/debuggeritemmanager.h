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

#ifndef DEBUGGER_DEBUGGERITEMMANAGER_H
#define DEBUGGER_DEBUGGERITEMMANAGER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QCoreApplication>

class QTemporaryDir;

namespace Utils { class FileName; }

namespace Debugger {
class DebuggerItem;

// -----------------------------------------------------------------------
// DebuggerItemManager
// -----------------------------------------------------------------------

class DEBUGGER_EXPORT DebuggerItemManager : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerItemManager)
public:
    DebuggerItemManager();
    ~DebuggerItemManager();

    static QList<DebuggerItem> debuggers();

    static QVariant registerDebugger(const DebuggerItem &item);
    static void deregisterDebugger(const QVariant &id);

    static const DebuggerItem *findByCommand(const Utils::FileName &command);
    static const DebuggerItem *findById(const QVariant &id);
    static const DebuggerItem *findByEngineType(DebuggerEngineType engineType);

    static void restoreDebuggers();
    static QString uniqueDisplayName(const QString &base);

    static void updateOrAddDebugger(const DebuggerItem &item);
    static void saveDebuggers();

    static QByteArray pythonPath();

private:
    static void autoDetectGdbOrLldbDebuggers();
    static void autoDetectCdbDebuggers();
    static void readLegacyDebuggers(const Utils::FileName &file);
    QTemporaryDir* d_python;
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERITEMMANAGER_H
