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

#ifndef OUTLINE_H
#define OUTLINE_H

#include <texteditor/ioutlinewidget.h>
#include <core/inavigationwidgetfactory.h>
#include <QStackedWidget>
#include <QMenu>

namespace Core { class IEditor; }

namespace TextEditor {
namespace Internal {

class OutlineFactory;

class OutlineWidgetStack : public QStackedWidget
{
    Q_OBJECT
public:
    OutlineWidgetStack(OutlineFactory *factory);
    ~OutlineWidgetStack();

    QToolButton *toggleSyncButton();
    QToolButton *filterButton();

    void saveSettings(int position);
    void restoreSettings(int position);

private:
    bool isCursorSynchronized() const;
    QWidget *dummyWidget() const;
    void updateFilterMenu();

private slots:
    void toggleCursorSynchronization();
    void updateCurrentEditor(Core::IEditor *editor);

private:
    QStackedWidget *m_widgetStack;
    OutlineFactory *m_factory;
    QToolButton *m_toggleSync;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
    QVariantMap m_widgetSettings;
    bool m_syncWithEditor;
};

class OutlineFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    OutlineFactory();

    QList<IOutlineWidgetFactory*> widgetFactories() const;
    void setWidgetFactories(QList<IOutlineWidgetFactory*> factories);

    // from INavigationWidgetFactory
    virtual Core::NavigationView createWidget();
    virtual void saveSettings(int position, QWidget *widget);
    virtual void restoreSettings(int position, QWidget *widget);
private:
    QList<IOutlineWidgetFactory*> m_factories;
};

} // namespace Internal
} // namespace TextEditor

#endif // OUTLINE_H
