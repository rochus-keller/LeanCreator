/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OPENDOCUMENTSTREEVIEW_H
#define OPENDOCUMENTSTREEVIEW_H

#include "core_global.h"

#include <utils/itemviews.h>

namespace Core {
namespace Internal { class OpenDocumentsDelegate; }

class CORE_EXPORT OpenDocumentsTreeView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit OpenDocumentsTreeView(QWidget *parent = 0);

    void setModel(QAbstractItemModel *model);
    void setCloseButtonVisible(bool visible);

signals:
    void closeActivated(const QModelIndex &index);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Internal::OpenDocumentsDelegate *m_delegate;
};

} // namespace Core

#endif // OPENDOCUMENTSTREEVIEW_H
