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

#ifndef OPENDOCUMENTSFILTER_H
#define OPENDOCUMENTSFILTER_H

#include "ilocatorfilter.h"

#include <core/editormanager/documentmodel.h>

#include <QFutureInterface>
#include <QList>
#include <QMutex>
#include <QString>

namespace Core {
namespace Internal {

class OpenDocumentsFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    OpenDocumentsFilter();
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry);
    void accept(LocatorFilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

public slots:
    void refreshInternally();

private:
    class Entry
    {
    public:
        Utils::FileName fileName;
        QString displayName;
    };

    QList<Entry> editors() const;

    mutable QMutex m_mutex;
    QList<Entry> m_editors;
};

} // namespace Internal
} // namespace Core

#endif // OPENDOCUMENTSFILTER_H
