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

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>

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
    //setIndenterCreator([]() { return new VerilogIndenter; });
    setEditorWidgetCreator([]() { return new EditorWidget; });
    setEditorCreator([]() { return new Editor; });
    //setAutoCompleterCreator([]() { return new AutoCompleter; });
    //setCompletionAssistProvider(new CompletionAssistProvider);
    //setSyntaxHighlighterCreator([]() { return new VerilogHighlighter; });
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
    //connect(d_outline, SIGNAL(activated(int)), this, SLOT(gotoSymbolInEditor()));
    //connect(d_outline, SIGNAL(currentIndexChanged(int)), this, SLOT(updateToolTip()));

    //OutlineMdl1* outline = new OutlineMdl1(this);
    //d_outline->setModel(outline);
    //connect( outline, SIGNAL(modelReset()), this, SLOT(onCursor()) );

    insertExtraToolBarWidget(TextEditorWidget::Left, d_outline );
}
