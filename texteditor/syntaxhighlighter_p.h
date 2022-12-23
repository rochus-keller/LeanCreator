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

#ifndef TEXTEDITOR_SYNTAXHIGHLIGHTER_P_H
#define TEXTEDITOR_SYNTAXHIGHLIGHTER_P_H

#include "texteditor.h"
#include "syntaxhighlighter.h"
#include "textdocument.h"
#include <QTextDocument>
#include <QPointer>
#include <QTextCharFormat>
#include "textdocumentlayout.h"
#include "texteditorsettings.h"
#include "fontsettings.h"

namespace TextEditor {

class SyntaxHighlighterPrivate
{
    SyntaxHighlighter *q_ptr;
    Q_DECLARE_PUBLIC(SyntaxHighlighter)
public:
    inline SyntaxHighlighterPrivate()
        : q_ptr(0), rehighlightPending(false), inReformatBlocks(false)
    {}

    QPointer<QTextDocument> doc;

    void _q_reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlock(const QTextBlock &block, int from, int charsRemoved, int charsAdded);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation) {
        inReformatBlocks = true;
        int from = cursor.position();
        cursor.movePosition(operation);
        reformatBlocks(from, 0, cursor.position() - from);
        inReformatBlocks = false;
    }

	inline void _q_delayedRehighlight() {
		if (!rehighlightPending)
		    return;
		rehighlightPending = false;
		q_func()->rehighlight();
	}

    void applyFormatChanges(int from, int charsRemoved, int charsAdded);
    void updateFormatsForCategories(const FontSettings &fontSettings);

    QVector<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending;
    bool inReformatBlocks;
    TextDocumentLayout::FoldValidator foldValidator;
    QVector<QTextCharFormat> formats;
    QVector<TextStyle> formatCategories;
};


} // namespace TextEditor

#endif // TEXTEDITOR_SYNTAXHIGHLIGHTER_H
