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

#ifndef PROJECTEXPLORER_ITASKHANDLER_H
#define PROJECTEXPLORER_ITASKHANDLER_H

#include "projectexplorer_export.h"

#include <core/id.h>

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Task;

class PROJECTEXPLORER_EXPORT ITaskHandler : public QObject
{
    Q_OBJECT

public:
    explicit ITaskHandler() {}

    virtual bool isDefaultHandler() const { return false; }
    virtual bool canHandle(const Task &) const = 0;
    virtual void handle(const Task &) = 0;
    virtual Core::Id actionManagerId() const { return Core::Id(); }
    virtual QAction *createAction(QObject *parent) const = 0;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_ITASKHANDLER_H
