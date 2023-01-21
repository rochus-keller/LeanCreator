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

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>
#include <core/editormanager/editormanager.h>

#include <QFileInfo>
#include <QtDebug>

using namespace BusyProjectManager;


Editor::Editor()
{
    addContext(Constants::LANG);
}


EditorFactory::EditorFactory()
{
    setId(Constants::BUSY_EDITOR_ID);
    addMimeType(Constants::MIME_TYPE);

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
    // TODO: we need the model for the virtual path; the physical path is not always what is relevant
    setPreferredDisplayName(QFileInfo(fileName).canonicalPath().split('/').last()+"/BUSY");

    return TextDocument::open(errorString, fileName, realFileName );
}

bool EditorDocument::save(QString* errorString, const QString& fileName, bool autoSave)
{
    return TextDocument::save(errorString,fileName, autoSave);
}


EditorWidget::EditorWidget():d_outline(0)
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
    //connect(d_outline, SIGNAL(currentIndexChanged(int)), this, SLOT(updateToolTip()));

    busy::EditorOutline* outline = new busy::EditorOutline(this);
    d_outline->setModel(outline);
    //connect( outline, SIGNAL(modelReset()), this, SLOT(onCursor()) );

    insertExtraToolBarWidget(TextEditorWidget::Left, d_outline );
}

void EditorWidget::onDocReady(Utils::FileName oldName,Utils::FileName newName)
{
    const QString fileName = newName.toString();
    //CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProjectOrDirPath( fileName, true);
                // in case there is no project create one with current file path and parse each Verilog file
                // found there
    //Q_ASSERT(mdl != 0 );
    //connect( mdl, SIGNAL(sigFileUpdated(QString)), this, SLOT(onFileUpdated(QString)) );

    busy::EditorOutline* outline = static_cast<busy::EditorOutline*>( d_outline->model() );
    outline->setFileName(fileName);
}

void EditorWidget::onCursor()
{
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::StartOfWord);
    const int line = cur.blockNumber() + 1;
    const int col = cur.columnNumber() + 1;

    busy::EditorOutline* mdl = static_cast<busy::EditorOutline*>( d_outline->model() );
    if( mdl )
    {
        QModelIndex pos = mdl->findByPosition(line,col);
        if( !pos.isValid() )
            pos = mdl->index(0,0);
        const bool blocked = d_outline->blockSignals(true);
        d_outline->setCurrentIndex(pos);
        //updateToolTip();
        d_outline->blockSignals(blocked);
    }
}

void EditorWidget::gotoSymbolInEditor()
{
    busy::EditorOutline* mdl = static_cast<busy::EditorOutline*>( d_outline->model() );
    const QModelIndex modelIndex = d_outline->view()->currentIndex();

    int row, col;
    if( mdl->getRowCol(modelIndex,row,col) && row > 0 )
    {
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();
        gotoLine( row, col );
        //emit sigGotoSymbol( row, col );
        activateEditor();
    }
}
