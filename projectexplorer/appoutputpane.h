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

#ifndef APPOUTPUTPANE_H
#define APPOUTPUTPANE_H

#include <core/ioutputpane.h>

#include <utils/outputformat.h>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QToolButton;
class QAction;
class QPoint;
QT_END_NAMESPACE

namespace Core { class OutputWindow; }

namespace ProjectExplorer {

class RunControl;
class Project;

namespace Internal {

class AppOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    enum BehaviorOnOutput {
        Flash,
        Popup
    };

    AppOutputPane();
    virtual ~AppOutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void createNewOutputWindow(RunControl *rc);
    void showTabFor(RunControl *rc);
    void setBehaviorOnOutput(RunControl *rc, BehaviorOnOutput mode);

    bool aboutToClose() const;
    bool closeTabs(CloseTabMode mode);

    QList<RunControl *> allRunControls() const;

signals:
     void allRunControlsFinished();
     void runControlStarted(ProjectExplorer::RunControl *rc);
     void runControlFinished(ProjectExplorer::RunControl *rc);

public slots:
    // ApplicationOutput specifics
    void projectRemoved();

    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);

private slots:
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    bool closeTab(int index);
    void tabChanged(int);
    void contextMenuRequested(const QPoint &pos, int index);
    void slotRunControlStarted();
    void slotRunControlFinished();
    void slotRunControlFinished2(ProjectExplorer::RunControl *sender);

    void aboutToUnloadSession();
    void updateFromSettings();
    void enableButtons();

    void zoomIn();
    void zoomOut();

private:
    void enableButtons(const RunControl *rc, bool isRunning);

    struct RunControlTab {
        explicit RunControlTab(RunControl *runControl = 0,
                               Core::OutputWindow *window = 0);
        RunControl* runControl;
        Core::OutputWindow *window;
        // Is the run control stopping asynchronously, close the tab once it finishes
        bool asyncClosing;
        BehaviorOnOutput behaviorOnOutput;
    };

    bool isRunning() const;
    bool closeTab(int index, CloseTabMode cm);
    bool optionallyPromptToStop(RunControl *runControl);

    int indexOf(const RunControl *) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    RunControl *currentRunControl() const;
    int tabWidgetIndexOf(int runControlIndex) const;
    void handleOldOutput(Core::OutputWindow *window) const;
    void updateCloseActions();
    void updateFontSettings();
    void saveSettings();
    void updateBehaviorSettings();

    QWidget *m_mainWidget;
    class TabWidget *m_tabWidget;
    QList<RunControlTab> m_runControlTabs;
    QAction *m_stopAction;
    QAction *m_closeCurrentTabAction;
    QAction *m_closeAllTabsAction;
    QAction *m_closeOtherTabsAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QToolButton *m_attachButton;
    QToolButton *m_zoomInButton;
    QToolButton *m_zoomOutButton;
    float m_zoom;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPOUTPUTPANE_H
