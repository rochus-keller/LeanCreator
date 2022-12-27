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

#ifndef COMMANDMAPPINGS_H
#define COMMANDMAPPINGS_H

#include <core/core_global.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {

namespace Internal { class CommandMappingsPrivate; }

class CORE_EXPORT CommandMappings : public QWidget
{
    Q_OBJECT

public:
    CommandMappings(QWidget *parent = 0);
    ~CommandMappings();

signals:
    void currentCommandChanged(QTreeWidgetItem *current);

protected:
    virtual void defaultAction() = 0;

    virtual void exportAction() {}
    virtual void importAction() {}

    virtual bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const;

    void filterChanged(const QString &f);

    // access to m_page
    void setImportExportEnabled(bool enabled);
    QTreeWidget *commandList() const;
    QString filterText() const;
    void setFilterText(const QString &text);
    void setPageTitle(const QString &s);
    void setTargetHeader(const QString &s);
    void setModified(QTreeWidgetItem *item, bool modified);

private:
    bool filter(const QString &filterString, QTreeWidgetItem *item);

    friend class Internal::CommandMappingsPrivate;
    Internal::CommandMappingsPrivate *d;
};

} // namespace Core

#endif // COMMANDMAPPINGS_H
