/****************************************************************************
**
** Copyright (C) 2015 Konstantin Tokarev.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of Qt Creator.
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

#ifndef COMMANDBUTTON_H
#define COMMANDBUTTON_H

#include <core/core_global.h>

#include <QPointer>
#include <QString>
#include <QToolButton>

namespace Core {

class Command;
class Id;

class CORE_EXPORT CommandButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QString toolTipBase READ toolTipBase WRITE setToolTipBase)
public:
    explicit CommandButton(QWidget *parent = 0);
    explicit CommandButton(Id id, QWidget *parent = 0);
    void setCommandId(Id id);
    QString toolTipBase() const;
    void setToolTipBase(const QString &toolTipBase);

private slots:
    void updateToolTip();

private:
    QPointer<Command> m_command;
    QString m_toolTipBase;
};

}

#endif // COMMANDBUTTON_H
