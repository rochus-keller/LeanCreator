/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHELPCONTENTWIDGET_H
#define QHELPCONTENTWIDGET_H

#include <help/qhelp_global.h>

#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtWidgets/QTreeView>

QT_BEGIN_NAMESPACE


class QHelpEnginePrivate;
class QHelpDBReader;
class QHelpContentItemPrivate;
class QHelpContentModelPrivate;
class QHelpEngine;
class QHelpContentProvider;

class QHELP_EXPORT QHelpContentItem
{
public:
    ~QHelpContentItem();

    QHelpContentItem *child(int row) const;
    int childCount() const;
    QString title() const;
    QUrl url() const;
    int row() const;
    QHelpContentItem *parent() const;
    int childPosition(QHelpContentItem *child) const;

private:
    QHelpContentItem(const QString &name, const QString &link,
        QHelpDBReader *reader, QHelpContentItem *parent = Q_NULLPTR);
    void appendChild(QHelpContentItem *child);

    QHelpContentItemPrivate *d;
    friend class QHelpContentProvider;
};

class QHELP_EXPORT QHelpContentModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ~QHelpContentModel();

    void createContents(const QString &customFilterName);
    QHelpContentItem *contentItemAt(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column,
        const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool isCreatingContents() const;

Q_SIGNALS:
    void contentsCreationStarted();
    void contentsCreated();

private Q_SLOTS:
    void insertContents();
    void invalidateContents(bool onShutDown = false);

private:
    QHelpContentModel(QHelpEnginePrivate *helpEngine);
    QHelpContentModelPrivate *d;
    friend class QHelpEnginePrivate;
};

class QHELP_EXPORT QHelpContentWidget : public QTreeView
{
    Q_OBJECT

public:
    QModelIndex indexOf(const QUrl &link);

Q_SIGNALS:
    void linkActivated(const QUrl &link);

private Q_SLOTS:
    void showLink(const QModelIndex &index);

private:
    bool searchContentItem(QHelpContentModel *model,
        const QModelIndex &parent, const QString &path);
    QModelIndex m_syncIndex;

private:
    QHelpContentWidget();
    friend class QHelpEngine;
};

QT_END_NAMESPACE

#endif

