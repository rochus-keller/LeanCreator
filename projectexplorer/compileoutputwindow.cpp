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

#include "compileoutputwindow.h"
#include "buildmanager.h"
#include "showoutputtaskhandler.h"
#include "task.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "taskhub.h"

#include <core/outputwindow.h>
#include <core/find/basetextfind.h>
#include <core/icore.h>
#include <core/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/behaviorsettings.h>
#include <utils/ansiescapecodehandler.h>
#include <utils/theme/theme.h>

#include <QIcon>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextCursor>
#include <QPlainTextEdit>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const int MAX_LINECOUNT = 100000;
const char SETTINGS_KEY[] = "ProjectExplorer/CompileOutput/Zoom";
}

namespace ProjectExplorer {
namespace Internal {

class CompileOutputTextEdit : public Core::OutputWindow
{
    Q_OBJECT
public:
    CompileOutputTextEdit(const Core::Context &context)
        : Core::OutputWindow(context)
    {
        setWheelZoomEnabled(true);

        QSettings *settings = Core::ICore::settings();
        float zoom = settings->value(QLatin1String(SETTINGS_KEY), 0).toFloat();
        setFontZoom(zoom);

        fontSettingsChanged();

        connect(TextEditor::TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
                this, SLOT(fontSettingsChanged()));

        connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
                this, &CompileOutputTextEdit::saveSettings);

        setMouseTracking(true);
    }

    void saveSettings()
    {
        QSettings *settings = Core::ICore::settings();
        settings->setValue(QLatin1String(SETTINGS_KEY), fontZoom());
    }

    void addTask(const Task &task, int blocknumber)
    {
        m_taskids.insert(blocknumber, task.taskId);
    }

    void clearTasks()
    {
        m_taskids.clear();
    }
private slots:
    void fontSettingsChanged()
    {
        setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
    }

protected:
    void mouseMoveEvent(QMouseEvent *ev)
    {
        int line = cursorForPosition(ev->pos()).block().blockNumber();
        if (m_taskids.value(line, 0))
            viewport()->setCursor(Qt::PointingHandCursor);
        else
            viewport()->setCursor(Qt::IBeamCursor);
        QPlainTextEdit::mouseMoveEvent(ev);
    }

    void mousePressEvent(QMouseEvent *ev)
    {
        m_mousePressPosition = ev->pos();
        QPlainTextEdit::mousePressEvent(ev);
    }

    void mouseReleaseEvent(QMouseEvent *ev)
    {
        if ((m_mousePressPosition - ev->pos()).manhattanLength() < 4) {
            int line = cursorForPosition(ev->pos()).block().blockNumber();
            if (unsigned taskid = m_taskids.value(line, 0))
                TaskHub::showTaskInEditor(taskid);
        }

        QPlainTextEdit::mouseReleaseEvent(ev);
    }

private:
    QHash<int, unsigned int> m_taskids;   //Map blocknumber to taskId
    QPoint m_mousePressPosition;
};

} // namespace Internal
} // namespace ProjectExplorer

CompileOutputWindow::CompileOutputWindow(QAction *cancelBuildAction) :
    m_cancelBuildButton(new QToolButton),
    m_zoomInButton(new QToolButton),
    m_zoomOutButton(new QToolButton),
    m_escapeCodeHandler(new Utils::AnsiEscapeCodeHandler)
{
    Core::Context context(Constants::C_COMPILE_OUTPUT);
    m_outputWindow = new CompileOutputTextEdit(context);
    m_outputWindow->setWindowTitle(tr("Compile Output"));
    m_outputWindow->setWindowIcon(QIcon(QLatin1String(Constants::ICON_WINDOW)));
    m_outputWindow->setReadOnly(true);
    m_outputWindow->setUndoRedoEnabled(false);
    m_outputWindow->setMaxLineCount(MAX_LINECOUNT);

    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_outputWindow->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_outputWindow->setPalette(p);

    m_cancelBuildButton->setDefaultAction(cancelBuildAction);
    m_zoomInButton->setToolTip(tr("Increase Font Size"));
    m_zoomInButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    m_zoomOutButton->setToolTip(tr("Decrease Font Size"));
    m_zoomOutButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));

    updateZoomEnabled();

    connect(TextEditor::TextEditorSettings::instance(),
            &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            this, &CompileOutputWindow::updateZoomEnabled);

    connect(m_zoomInButton, &QToolButton::clicked,
            this, [this]() { m_outputWindow->zoomIn(1); });
    connect(m_zoomOutButton, &QToolButton::clicked,
            this, [this]() { m_outputWindow->zoomOut(1); });

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_outputWindow);
    agg->add(new Core::BaseTextFind(m_outputWindow));

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");

    m_handler = new ShowOutputTaskHandler(this);
    ExtensionSystem::PluginManager::addObject(m_handler);
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateWordWrapMode()));
    updateWordWrapMode();
}

CompileOutputWindow::~CompileOutputWindow()
{
    ExtensionSystem::PluginManager::removeObject(m_handler);
    delete m_handler;
    delete m_cancelBuildButton;
    delete m_zoomInButton;
    delete m_zoomOutButton;
    delete m_escapeCodeHandler;
}

void CompileOutputWindow::updateZoomEnabled()
{
    const TextEditor::BehaviorSettings &settings
            = TextEditor::TextEditorSettings::behaviorSettings();
    bool zoomEnabled  = settings.m_scrollWheelZooming;
    m_zoomInButton->setEnabled(zoomEnabled);
    m_zoomOutButton->setEnabled(zoomEnabled);
    m_outputWindow->setWheelZoomEnabled(zoomEnabled);
}

void CompileOutputWindow::updateWordWrapMode()
{
    m_outputWindow->setWordWrapEnabled(ProjectExplorerPlugin::projectExplorerSettings().wrapAppOutput);
}

bool CompileOutputWindow::hasFocus() const
{
    return m_outputWindow->window()->focusWidget() == m_outputWindow;
}

bool CompileOutputWindow::canFocus() const
{
    return true;
}

void CompileOutputWindow::setFocus()
{
    m_outputWindow->setFocus();
}

QWidget *CompileOutputWindow::outputWidget(QWidget *)
{
    return m_outputWindow;
}

QList<QWidget *> CompileOutputWindow::toolBarWidgets() const
{
     return QList<QWidget *>() << m_cancelBuildButton
                               << m_zoomInButton
                               << m_zoomOutButton;
}

void CompileOutputWindow::appendText(const QString &text, BuildStep::OutputFormat format)
{
    using Utils::Theme;
    Theme *theme = Utils::creatorTheme();
    QTextCharFormat textFormat;
    switch (format) {
    case BuildStep::NormalOutput:
        textFormat.setForeground(theme->color(Theme::TextColorNormal));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::ErrorOutput:
        textFormat.setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::MessageOutput:
        textFormat.setForeground(theme->color(Theme::OutputPanes_MessageOutput));
        break;
    case BuildStep::ErrorMessageOutput:
        textFormat.setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));
        textFormat.setFontWeight(QFont::Bold);
        break;

    }

    foreach (const Utils::FormattedText &output,
             m_escapeCodeHandler->parseText(Utils::FormattedText(text, textFormat)))
        m_outputWindow->appendText(output.text, output.format);
}

void CompileOutputWindow::clearContents()
{
    m_outputWindow->clear();
    m_outputWindow->clearTasks();
    m_taskPositions.clear();
}

void CompileOutputWindow::visibilityChanged(bool)
{

}

int CompileOutputWindow::priorityInStatusBar() const
{
    return 50;
}

bool CompileOutputWindow::canNext() const
{
    return false;
}

bool CompileOutputWindow::canPrevious() const
{
    return false;
}

void CompileOutputWindow::goToNext()
{

}

void CompileOutputWindow::goToPrev()
{

}

bool CompileOutputWindow::canNavigate() const
{
    return false;
}

void CompileOutputWindow::registerPositionOf(const Task &task, int linkedOutputLines, int skipLines)
{
    if (linkedOutputLines <= 0)
        return;
    int blocknumber = m_outputWindow->document()->blockCount();
    if (blocknumber > MAX_LINECOUNT)
        return;

    const int startLine = blocknumber - linkedOutputLines + 1 - skipLines;
    const int endLine = blocknumber - skipLines;

    m_taskPositions.insert(task.taskId, qMakePair(startLine, endLine));

    for (int i = startLine; i <= endLine; ++i)
        m_outputWindow->addTask(task, i);
}

bool CompileOutputWindow::knowsPositionOf(const Task &task)
{
    return (m_taskPositions.contains(task.taskId));
}

void CompileOutputWindow::showPositionOf(const Task &task)
{
    QPair<int, int> position = m_taskPositions.value(task.taskId);
    QTextCursor newCursor(m_outputWindow->document()->findBlockByNumber(position.second));

    // Move cursor to end of last line of interest:
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    m_outputWindow->setTextCursor(newCursor);

    // Move cursor and select lines:
    newCursor.setPosition(m_outputWindow->document()->findBlockByNumber(position.first).position(),
                          QTextCursor::KeepAnchor);
    m_outputWindow->setTextCursor(newCursor);

    // Center cursor now:
    m_outputWindow->centerCursor();
}

void CompileOutputWindow::flush()
{
    if (m_escapeCodeHandler)
        m_escapeCodeHandler->endFormatScope();
}

#include "compileoutputwindow.moc"
