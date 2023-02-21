/*
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
*/

#include "busyeditor.h"
#include "busyprojectmanagerconstants.h"
#include "busyhighlighter.h"
#include "busyindenter.h"
#include "busyeditoroutline.h"
#include <busytools/Engine.h>
extern "C" {
#include <bsparser.h>
}

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>
#include <core/editormanager/editormanager.h>
#include <core/actionmanager/actionmanager.h>
#include <core/actionmanager/command.h>
#include <core/actionmanager/actioncontainer.h>
#include <core/find/searchresultwindow.h>

#include <QFileInfo>
#include <QMenu>
#include <QtDebug>
#include <QDir>

using namespace BusyProjectManager;


Editor::Editor()
{
    addContext(Constants::LANG);
}


EditorFactory::EditorFactory()
{
    setId(Constants::BUSY_EDITOR_ID);
    addMimeType(Constants::PROJECT_MIME_TYPE);
    addMimeType(Constants::INCLUDE_MIME_TYPE);

    setDocumentCreator([]() { return new EditorDocument; });
    setIndenterCreator([]() { return new Indenter; });
    setEditorWidgetCreator([]() { return new EditorWidget; });
    setEditorCreator([]() { return new Editor; });
    //setAutoCompleterCreator([]() { return new AutoCompleter; });
    //setCompletionAssistProvider(new CompletionAssistProvider);
    setSyntaxHighlighterCreator([]() { return new Highlighter; });
    setCommentStyle(Utils::CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
    setMarksVisible(true);
    //addHoverHandler(new VerilogHoverHandler);

    setEditorActionHandlers(TextEditor::TextEditorActionHandler::Format
                          | TextEditor::TextEditorActionHandler::UnCommentSelection
                          | TextEditor::TextEditorActionHandler::UnCollapseAll
                          | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor
                            );
}


EditorDocument::EditorDocument()
{
    setId(Constants::BUSY_EDITOR_ID);

}

Core::IDocument::OpenResult EditorDocument::open(QString* errorString, const QString& fileName, const QString& realFileName)
{
    QFileInfo info(fileName);
    QString module = info.absoluteDir().dirName();
    if( module.isEmpty() )
        module = "/";
    const QString file = info.fileName();

    setPreferredDisplayName(QString("%1/%2").arg(module).arg(file));

    return TextDocument::open(errorString, fileName, realFileName );
}

bool EditorDocument::save(QString* errorString, const QString& fileName, bool autoSave)
{
    return TextDocument::save(errorString,fileName, autoSave);
}


EditorWidget::EditorWidget():d_outline(0),d_mdl(0)
{

}

EditorWidget::~EditorWidget()
{
    if( d_outline )
        delete d_outline;
}

void EditorWidget::finalizeInitialization()
{
    d_outline = new Utils::TreeViewComboBox();
    d_outline->setMinimumContentsLength(22);
    QSizePolicy policy = d_outline->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    d_outline->setSizePolicy(policy);
    d_outline->setMaxVisibleItems(40);

    d_outline->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect( textDocument(), SIGNAL(filePathChanged(Utils::FileName,Utils::FileName)),
             this, SLOT(onDocReady(Utils::FileName,Utils::FileName)) );
    connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursor()) );
    connect(d_outline, SIGNAL(activated(int)), this, SLOT(gotoSymbolInEditor()));
    connect(d_outline, SIGNAL(currentIndexChanged(int)), this, SLOT(updateToolTip()));

    d_mdl = new busy::EditorOutline(this);
    d_outline->setModel(d_mdl);
    //connect( outline, SIGNAL(modelReset()), this, SLOT(onCursor()) );

    insertExtraToolBarWidget(TextEditorWidget::Left, d_outline );
}

void EditorWidget::onDocReady(Utils::FileName oldName,Utils::FileName newName)
{
    d_mdl->setFileName(newName.toString());
}

static bool lessThan2(const QTextEdit::ExtraSelection &s1, const QTextEdit::ExtraSelection &s2)
{
    return s1.cursor.position() < s2.cursor.position();
}

typedef QList<QTextEdit::ExtraSelection> ExtraSelections;
static ExtraSelections toExtraSelections(const QList<busy::Engine::Loc>& uses,
                                                           TextEditor::TextStyle style, TextEditor::TextDocument* document )
{
    ExtraSelections result;

    QTextDocument* doc = document->document();

    foreach (const busy::Engine::Loc& use, uses)
    {
        const int position = doc->findBlockByNumber(use.d_row - 1).position() + use.d_col - 2;
        const int anchor = position + use.d_len;

        QTextEdit::ExtraSelection sel;
        sel.format = document->fontSettings().toTextCharFormat(style);
        sel.cursor = QTextCursor(doc);
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        result.append(sel);
    }

    std::sort(result.begin(), result.end(), lessThan2);
    return result;
}

void EditorWidget::onCursor()
{
    if( d_mdl->getEngine() == 0 )
        return;

    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::StartOfWord);
    const int line = cur.blockNumber() + 1;
    const int col = cur.columnNumber() + 1;

    QModelIndex pos = d_mdl->findByPosition(line,col);
    if( !pos.isValid() )
        pos = d_mdl->index(0,0);
    const bool blocked = d_outline->blockSignals(true);
    d_outline->setCurrentIndex(pos);
    updateToolTip();
    d_outline->blockSignals(blocked);

    QList<int> found = d_mdl->getEngine()->findDeclByPos(d_mdl->getFileName(),line,col);
    if( !found.isEmpty() )
    {
        QList<busy::Engine::Loc> pos = d_mdl->getEngine()->
                findDeclInstsInFile(d_mdl->getFileName(),found.first());
        setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection,
                           toExtraSelections(pos, TextEditor::C_OCCURRENCES, textDocument() ) );
    }else
        setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, ExtraSelections() );
}

void EditorWidget::gotoSymbolInEditor()
{
    const QModelIndex modelIndex = d_outline->view()->currentIndex();

    int row, col;
    if( d_mdl->getRowCol(modelIndex,row,col) && row > 0 )
    {
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();
        gotoLine( row, col );
        emit sigGotoSymbol( row, col );
        activateEditor();
    }
}

void EditorWidget::updateToolTip()
{
    d_outline->setToolTip(d_outline->currentText());
}

void EditorWidget::onOpenEditor(const Core::SearchResultItem& item)
{
    Core::EditorManager::openEditorAt( item.path.first(), item.lineNumber, item.textMarkPos);
}

void EditorWidget::onFindUsages()
{
    if( d_mdl->getEngine() == 0 )
        return;

    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::StartOfWord);
    const int line = cur.blockNumber() + 1;
    const int col = cur.columnNumber() + 1;

    QList<int> found = d_mdl->getEngine()->findDeclByPos(d_mdl->getFileName(),line,col);
    if( found.isEmpty() )
        return;

    cur.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    typedef QMap<QPair<int,int>, QString> PosText;
    typedef QMap<QString,PosText> FilePosText;
    FilePosText order;
    int len = 0;
    for( int i = 0; i < found.size(); i++ )
    {
        QList<busy::Engine::AllLocsInFile> res = d_mdl->getEngine()->findAllLocsOf(found[i]);
        for( int j = 0; j < res.size(); j++ )
        {
            PosText& pt = order[res[j].d_file];
            for(int k = 0; k < res[j].d_locs.size(); k++ )
            {
                const busy::Engine::Loc loc = res[j].d_locs[k];
                pt[ qMakePair(loc.d_row,loc.d_col) ] = QString();
                if( len == 0 )
                    len = loc.d_len;
            }
        }
    }
    Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
                                                tr("BUSY Usages:"),
                                                QString(),
                                                cur.selectedText(),
                                                Core::SearchResultWindow::SearchOnly,
                                                Core::SearchResultWindow::PreserveCaseDisabled,
                                                QString());

    for( FilePosText::const_iterator i = order.begin(); i != order.end(); ++i )
    {
        // TODO: make this more efficient and support not-yet saved files
        QFile in(i.key());
        in.open(QIODevice::ReadOnly);
        const QByteArrayList lines = in.readAll().split('\n');
        for( PosText::const_iterator j = i.value().begin(); j != i.value().end(); ++j )
            search->addResult( i.key(), j.key().first,
                           lines[j.key().first-1],
                           j.key().second - 2, len);
    }
    search->finishSearch(false);
    connect(search, SIGNAL(activated(Core::SearchResultItem)), this, SLOT(onOpenEditor(Core::SearchResultItem)));

    Core::SearchResultWindow::instance()->popup(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus);
    search->popup();

}

static int findStart( const QString& str, int pos )
{
    enum { Idle, Slash, DotSlash, SlashSlash, DotDotSlash } status = Idle;
    while( pos >= 0 && pos < str.size() )
    {
        const char ch = str[pos].toLatin1();
        switch(status)
        {
        case Idle:
            if( ch == '/' )
                status = Slash;
            else if( ch == '\'' )
                return pos;
            break;
        case Slash:
            if( ch == '/' )
                status = SlashSlash;
            else if( ch == '.' )
                status = DotSlash;
            else
                status = Idle;
            break;
        case SlashSlash:
        case DotDotSlash:
            if( ch == '\'')
                return pos;
            else
                return pos + 1;
            break;
        case DotSlash:
            if( ch == '.' )
                status = DotDotSlash;
            else if( ch == '\'' )
                return pos;
            else
                return pos + 1;
            break;
        }
        pos--;
    }
    return -1;
}

static int findLen( const QString& str, int start )
{
    int i = start + 1;
    if( str[start] == '\'' )
    {
        while( i < str.length() )
        {
            if( str[i] == '\'' )
                break;
            i++;
        }
    }else
    {
        while( i < str.length() )
        {
            if( str[i].isSpace() )
                break;
            i++;
        }
    }
    return i - start;
}

static bool findPath( QTextCursor& cur )
{
    const QString line = cur.block().text();
    const int start = findStart( line, cur.positionInBlock() );
    if( start >= 0 )
    {
        const int len = findLen( line, start);
        cur.setPosition(cur.block().position() + start + len);
        cur.setPosition(cur.block().position() + start, QTextCursor::KeepAnchor);
        return true;
    }
    return false;
}

TextEditor::TextEditorWidget::Link EditorWidget::findLinkAt(const QTextCursor& in, bool resolveTarget, bool inNextSplit)
{
    Q_UNUSED(resolveTarget);
    Q_UNUSED(inNextSplit);
    if( d_mdl->getEngine() == 0 )
        return Link();

    QTextCursor cur = in;
    cur.movePosition(QTextCursor::StartOfWord);
    const int line = cur.blockNumber() + 1;
    const int col = cur.columnNumber() + 1;

    QList<int> found = d_mdl->getEngine()->findDeclByPos(d_mdl->getFileName(),line,col);
    if( !found.isEmpty() )
    {
        const QString path = d_mdl->getEngine()->getPath(
                    d_mdl->getEngine()->getOwningModule(found.first()),"#file");
        int r = d_mdl->getEngine()->getInteger(found.first(),"#row");
        int c = d_mdl->getEngine()->getInteger(found.first(),"#col") - 1;
        const QString name = QString::fromUtf8(d_mdl->getEngine()->getString(found.first(),"#name"));
        if( path.isEmpty() )
            return Link();
        if( r == 0 )
        {
            r = 1;
            c = 0;
        }
        if( path == d_mdl->getFileName() && line == r && col == c + 1 )
        {
            // we point to the declaration itself
            if( d_mdl->getEngine()->getInteger(found.first(),"#kind") == BS_ModuleDef )
            {
                // this is a submod, go to the module files
                Link l( d_mdl->getEngine()->getPath(found.first(),"#file"), 1, 0);
                l.linkTextStart = cur.position();
                l.linkTextEnd = cur.position() + name.size();
                return l;
            }
        }else
        {
            // this is an ordinary symbol
            Link l( path, r, c);
            l.linkTextStart = cur.position();
            l.linkTextEnd = cur.position() + name.size();
            return l;
        }
    // TODO selector for found.size() > 1
    }else
    {
        QTextCursor cur = in;
        if( findPath(cur) )
        {
            const int line = cur.blockNumber() + 1;
            const QString sel = cur.selectedText();
            const int col = cur.columnNumber() + 1;
            const QString path = d_mdl->getEngine()->findPathByPos(d_mdl->getFileName(),line,col);
            if( !path.isEmpty() && !QFileInfo(path).isDir() )
            {
                Link l( path, 1, 0);
                l.linkTextStart = cur.position();
                l.linkTextEnd = cur.position() + sel.length();
                return l;
            }
        }
    }
    return Link();
}

void EditorWidget::contextMenuEvent(QContextMenuEvent* e)
{
    QPointer<QMenu> menu(new QMenu(this));

    Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(
                BusyProjectManager::Constants::EditorContextMenuId);
    Q_ASSERT(mcontext);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions()) {
        menu->addAction(action);
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (!menu)
        return;
    delete menu;
}
