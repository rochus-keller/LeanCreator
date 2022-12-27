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

#ifndef COMMAND_P_H
#define COMMAND_P_H

#include "command.h"

#include <core/id.h>
#include <core/icontext.h>

#include <utils/proxyaction.h>

#include <QList>
#include <QMultiMap>
#include <QPointer>
#include <QMap>
#include <QKeySequence>

namespace Core {
namespace Internal {

class Action : public Command
{
    Q_OBJECT
public:
    Action(Id id);

    Id id() const;

    void setDefaultKeySequence(const QKeySequence &key);
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    void setDescription(const QString &text);
    QString description() const;

    QAction *action() const;

    QString stringWithAppendedShortcut(const QString &str) const;

    Context context() const;
    void setCurrentContext(const Context &context);

    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

    bool isScriptable() const;
    bool isScriptable(const Context &context) const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

private slots:
    void updateActiveState();

private:
    void setActive(bool state);

    Context m_context;
    CommandAttributes m_attributes;
    Id m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;

    Utils::ProxyAction *m_action;
    QString m_toolTip;

    QMap<Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H
