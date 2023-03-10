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

#ifndef IEDITORFACTORY_H
#define IEDITORFACTORY_H

#include <core/core_global.h>
#include <core/id.h>

#include <QObject>
#include <QStringList>

namespace Core {

class IEditor;

class CORE_EXPORT IEditorFactory : public QObject
{
    Q_OBJECT

public:
    IEditorFactory(QObject *parent = 0);
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }

    Id id() const { return m_id; }
    void setId(Id id) { m_id = id; }

    virtual IEditor *createEditor() = 0;

    QStringList mimeTypes() const { return m_mimeTypes; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const char *mimeType) { m_mimeTypes.append(QLatin1String(mimeType)); }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }
private:
    Id m_id;
    QString m_displayName;
    QStringList m_mimeTypes;
};

} // namespace Core

#endif // IEDITORFACTORY_H
