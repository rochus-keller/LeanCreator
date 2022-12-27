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

#ifndef KITCONFIGWIDGET_H
#define KITCONFIGWIDGET_H

#include "projectexplorer_export.h"

#include <core/id.h>

#include <QWidget>

namespace ProjectExplorer {

class Kit;
class KitInformation;

// --------------------------------------------------------------------------
// KitConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT KitConfigWidget : public QObject
{
    Q_OBJECT

public:
    KitConfigWidget(Kit *kit, const KitInformation *ki);

    Core::Id kitInformationId() const;

    virtual QString displayName() const = 0;
    virtual QString toolTip() const { return QString(); }
    virtual void makeReadOnly() = 0;
    virtual void refresh() = 0;
    virtual bool visibleInKit() { return true; }

    virtual QWidget *mainWidget() const = 0;
    virtual QWidget *buttonWidget() const { return 0; }

    bool isSticky() const { return m_isSticky; }
    bool isMutable() const;
    void setMutable(bool b);

    static QString msgManage();

    Kit *kit() const { return m_kit; }

    virtual void setPalette(const QPalette &p);
    virtual void setStyle(QStyle *s);

signals:
    void dirty();

protected:
    Kit *m_kit;
    const KitInformation *m_kitInformation;
    bool m_isSticky;
};

} // namespace ProjectExplorer

#endif // KITCONFIGWIDGET_H
