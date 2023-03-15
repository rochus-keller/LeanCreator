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

#ifndef QHELPENGINE_H
#define QHELPENGINE_H

#include <help/qhelpenginecore.h>

QT_BEGIN_NAMESPACE


class QHelpContentModel;
class QHelpContentWidget;
class QHelpIndexModel;
class QHelpIndexWidget;
class QHelpEnginePrivate;
class QHelpSearchEngine;

class QHELP_EXPORT QHelpEngine : public QHelpEngineCore
{
    Q_OBJECT

public:
    explicit QHelpEngine(const QString &collectionFile, QObject *parent = Q_NULLPTR);
    ~QHelpEngine();

    QHelpContentModel *contentModel() const;
    QHelpIndexModel *indexModel() const;

    QHelpContentWidget *contentWidget();
    QHelpIndexWidget *indexWidget();

    QHelpSearchEngine *searchEngine();

private:
    QHelpEnginePrivate *d;

    friend class HelpEngineWrapper;
};

QT_END_NAMESPACE

#endif
