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

#ifndef FUTUREPROGRESS_H
#define FUTUREPROGRESS_H

#include <core/core_global.h>

#include <QString>
#include <QFuture>
#include <QWidget>

namespace Core {
class Id;
class FutureProgressPrivate;

class CORE_EXPORT FutureProgress : public QWidget
{
    Q_OBJECT

public:
    enum KeepOnFinishType {
        HideOnFinish = 0,
        KeepOnFinishTillUserInteraction = 1,
        KeepOnFinish = 2
    };
    explicit FutureProgress(QWidget *parent = 0);
    virtual ~FutureProgress();

    virtual bool eventFilter(QObject *object, QEvent *);

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const;

    void setTitle(const QString &title);
    QString title() const;

    void setType(Id type);
    Id type() const;

    void setKeepOnFinish(KeepOnFinishType keepType);
    bool keepOnFinish() const;

    bool hasError() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setStatusBarWidget(QWidget *widget);
    QWidget *statusBarWidget() const;

    bool isFading() const;

    QSize sizeHint() const;

signals:
    void clicked();
    void finished();
    void canceled();
    void removeMe();
    void hasErrorChanged();
    void fadeStarted();

    void statusBarWidgetChanged();

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *);

private slots:
    void updateToolTip(const QString &);
    void cancel();
    void setStarted();
    void setFinished();
    void setProgressRange(int min, int max);
    void setProgressValue(int val);
    void setProgressText(const QString &text);

private:
    friend class FutureProgressPrivate; // for sending signal
    FutureProgressPrivate *d;
};

} // namespace Core

#endif // FUTUREPROGRESS_H
