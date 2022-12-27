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

#include "commandlocator.h"

#include <core/actionmanager/command.h>

#include <utils/qtcassert.h>

#include <QAction>

namespace Core {

struct CommandLocatorPrivate
{
    QList<Command *> commands;
};

CommandLocator::CommandLocator(Id id,
                               const QString &displayName,
                               const QString &shortCutString,
                               QObject *parent) :
    ILocatorFilter(parent),
    d(new CommandLocatorPrivate)
{
    setId(id);
    setDisplayName(displayName);
    setShortcutString(shortCutString);
}

CommandLocator::~CommandLocator()
{
    delete d;
}

void CommandLocator::appendCommand(Command *cmd)
{
    d->commands.push_back(cmd);
}

QList<LocatorFilterEntry> CommandLocator::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    const QChar ampersand = QLatin1Char('&');
    const Qt::CaseSensitivity caseSensitivity_ = caseSensitivity(entry);
    const int count = d->commands.size();
    for (int i = 0; i < count; i++) {
        if (future.isCanceled())
            break;
        if (d->commands.at(i)->isActive()) {
            if (QAction *action = d->commands.at(i)->action())
                if (action->isEnabled()) {
                QString text = action->text();
                text.remove(ampersand);
                if (text.startsWith(entry, caseSensitivity_))
                    betterEntries.append(LocatorFilterEntry(this, text, QVariant(i)));
                else if (text.contains(entry, caseSensitivity_))
                    goodEntries.append(LocatorFilterEntry(this, text, QVariant(i)));
            }
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void CommandLocator::accept(LocatorFilterEntry entry) const
{
    // Retrieve action via index.
    const int index = entry.internalData.toInt();
    QTC_ASSERT(index >= 0 && index < d->commands.size(), return);
    QAction *action = d->commands.at(index)->action();
    QTC_ASSERT(action->isEnabled(), return);
    action->trigger();
}

void CommandLocator::refresh(QFutureInterface<void> &)
{
}

}  // namespace Core
