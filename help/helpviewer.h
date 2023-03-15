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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <core/find/textfindconstants.h>

#include <QFont>
#include <QMenu>
//#include <QPrinter>
#include <QString>
#include <QUrl>
#include <QWidget>

class QPrinter;

namespace Help {
namespace Internal {

class HelpViewer : public QWidget
{
    Q_OBJECT

public:
    explicit HelpViewer(QWidget *parent = 0);
    ~HelpViewer() { }

    virtual QFont viewerFont() const = 0;
    virtual void setViewerFont(const QFont &font) = 0;

    virtual qreal scale() const = 0;
    virtual void setScale(qreal scale) = 0;

    virtual QString title() const = 0;

    virtual QUrl source() const = 0;
    // metacall in HelpPlugin::updateSideBarSource
    Q_INVOKABLE virtual void setSource(const QUrl &url) = 0;
    virtual void scrollToAnchor(const QString &anchor) = 0;
    virtual void highlightId(const QString &id) { Q_UNUSED(id) }

    virtual void setHtml(const QString &html) = 0;

    virtual QString selectedText() const = 0;
    virtual bool isForwardAvailable() const = 0;
    virtual bool isBackwardAvailable() const = 0;
    virtual void addBackHistoryItems(QMenu *backMenu) = 0;
    virtual void addForwardHistoryItems(QMenu *forwardMenu) = 0;
    virtual void setOpenInNewPageActionVisible(bool visible) = 0;

    virtual bool findText(const QString &text, Core::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0) = 0;

    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

public slots:
    void home();

    virtual void scaleUp() = 0;
    virtual void scaleDown() = 0;
    virtual void resetScale() = 0;
    virtual void copy() = 0;
    virtual void stop() = 0;
    virtual void forward() = 0;
    virtual void backward() = 0;
    virtual void print(QPrinter *printer) = 0;

signals:
    void sourceChanged(const QUrl &);
    void titleChanged();
    void printRequested();
    void forwardAvailable(bool);
    void backwardAvailable(bool);
    void loadFinished();

protected slots:
    void slotLoadStarted();
    void slotLoadFinished();
};

}   // namespace Internal
}   // namespace Help

#endif  // HELPVIEWER_H
