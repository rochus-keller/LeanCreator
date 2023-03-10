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

#ifndef COMMAND_H
#define COMMAND_H

#include <core/core_global.h>
#include <core/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QKeySequence;
class QToolButton;
QT_END_NAMESPACE


namespace Core {

class Context;

#ifdef Q_OS_MAC
enum { UseMacShortcuts = 1 };
#else
enum { UseMacShortcuts = 0 };
#endif

class CORE_EXPORT Command : public QObject
{
    Q_OBJECT
public:
    enum CommandAttribute {
        CA_Hide = 1,
        CA_UpdateText = 2,
        CA_UpdateIcon = 4,
        CA_NonConfigurable = 8
    };
    Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

    virtual void setDefaultKeySequence(const QKeySequence &key) = 0;
    virtual QKeySequence defaultKeySequence() const = 0;
    virtual QKeySequence keySequence() const = 0;
    // explicitly set the description (used e.g. in shortcut settings)
    // default is to use the action text for actions, or the whatsThis for shortcuts,
    // or, as a last fall back if these are empty, the command ID string
    // override the default e.g. if the text is context dependent and contains file names etc
    virtual void setDescription(const QString &text) = 0;
    virtual QString description() const = 0;

    virtual Id id() const = 0;

    virtual QAction *action() const = 0;
    virtual Context context() const = 0;

    virtual void setAttribute(CommandAttribute attr) = 0;
    virtual void removeAttribute(CommandAttribute attr) = 0;
    virtual bool hasAttribute(CommandAttribute attr) const = 0;

    virtual bool isActive() const = 0;

    virtual void setKeySequence(const QKeySequence &key) = 0;
    virtual QString stringWithAppendedShortcut(const QString &str) const = 0;
    void augmentActionWithShortcutToolTip(QAction *action) const;
    static QToolButton *toolButtonWithAppendedShortcut(QAction *action, Command *cmd);

    virtual bool isScriptable() const = 0;
    virtual bool isScriptable(const Context &) const = 0;

signals:
    void keySequenceChanged();
    void activeStateChanged();
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)

#endif // COMMAND_H
