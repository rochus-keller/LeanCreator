#ifndef BUSYINDENTER_H
#define BUSYINDENTER_H

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

#include <texteditor/indenter.h>

namespace BusyProjectManager
{
    class Indenter : public TextEditor::Indenter
    {
    public:
        Indenter();

        // overrides
        bool isElectricCharacter(const QChar &ch) const;
        void indentBlock(QTextDocument *document,
                         const QTextBlock &block,
                         const QChar &typedChar,
                         const TextEditor::TabSettings &settings);
    };
}

#endif // BUSYINDENTER_H
