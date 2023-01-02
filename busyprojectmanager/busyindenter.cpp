/*
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
*/

#include "busyindenter.h"
#include "busyhighlighter.h"
#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>
using namespace BusyProjectManager;

static const int TAB_SIZE = 4;

Indenter::Indenter()
{

}

bool Indenter::isElectricCharacter(const QChar& ch) const
{
    Q_UNUSED(ch);
    return false;
}

void Indenter::indentBlock(QTextDocument* doc, const QTextBlock& block, const QChar& typedChar,
                           const TextEditor::TabSettings& tabSettings)
{
    // aus QtCreator texteditor normalindenter adaptiert
    // TODO: später mal einen intelligenteren Indenter wie bei C++, ist aber nice to have.
    Q_UNUSED(typedChar)

    // At beginning: Leave as is.
    if (block == doc->begin())
        return;

    const QTextBlock previous = block.previous();
    const QString previousText = previous.text();
    // Empty line indicates a start of a new paragraph. Leave as is.
    if (previousText.isEmpty() || previousText.trimmed().isEmpty())
        return;

    // Just use previous line.
    // Skip blank characters when determining the indentation
    int i = 0;
    while (i < previousText.size()) {
        if (!previousText.at(i).isSpace()) {
            tabSettings.indentLine(block, tabSettings.columnAt(previousText, i));
            break;
        }
        ++i;
    }
}

