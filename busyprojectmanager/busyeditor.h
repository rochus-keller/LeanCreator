#ifndef LC_BUSY_EDITOR
#define LC_BUSY_EDITOR

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

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <utils/treeviewcombobox.h>
#include <utils/fileutils.h>

namespace Core { class SearchResultItem; }
namespace busy { class EditorOutline; }

namespace BusyProjectManager
{
    class Editor : public TextEditor::BaseTextEditor
    {
        Q_OBJECT
    public:
        explicit Editor();

    };

    class EditorFactory : public TextEditor::TextEditorFactory
    {
        Q_OBJECT
    public:
        EditorFactory();
    };

    class EditorDocument : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument();

        // overrides
        TextDocument::OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName);
        bool save(QString *errorString, const QString &fileName, bool autoSave);
    };

    class EditorWidget : public TextEditor::TextEditorWidget
    {
        Q_OBJECT
    public:
        EditorWidget();
        ~EditorWidget();

        void finalizeInitialization();

    signals:
        void sigGotoSymbol( quint32 line, quint16 col );

    public slots:
        void onFindUsages();

    protected slots:
        void onDocReady(Utils::FileName oldName,Utils::FileName newName);
        void onCursor();
        void gotoSymbolInEditor();
        void updateToolTip();
        void onOpenEditor(const Core::SearchResultItem &);

    protected:
        Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                        bool inNextSplit = false);
        void contextMenuEvent(QContextMenuEvent *e);
    private:
        Utils::TreeViewComboBox* d_outline;
        busy::EditorOutline* d_mdl;
    };
}

#endif
