/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef OPENEDITORSVIEW_H
#define OPENEDITORSVIEW_H

#include <core/inavigationwidgetfactory.h>
#include <core/opendocumentstreeview.h>

#include <QAbstractProxyModel>

namespace Core {
class IEditor;

namespace Internal {

class ProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent = 0);
    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex & proxyIndex) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void setSourceModel(QAbstractItemModel *sourceModel);

    // QAbstractProxyModel::sibling is broken in Qt 5
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
    // QAbstractProxyModel::supportedDragActions delegation is missing in Qt 5
    Qt::DropActions supportedDragActions() const;

private slots:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
};

class OpenEditorsWidget : public OpenDocumentsTreeView
{
    Q_OBJECT

public:
    OpenEditorsWidget();
    ~OpenEditorsWidget();

private:
    void handleActivated(const QModelIndex &);
    void updateCurrentItem(IEditor*);
    void contextMenuRequested(QPoint pos);
    void activateEditor(const QModelIndex &index);
    void closeDocument(const QModelIndex &index);

    ProxyModel *m_model;
};

class OpenEditorsViewFactory : public INavigationWidgetFactory
{
    Q_OBJECT

public:
    OpenEditorsViewFactory();

    NavigationView createWidget();
};

} // namespace Internal
} // namespace Core


#endif // OPENEDITORSVIEW_H
