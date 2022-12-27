/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DISPLAYSETTINGS_H
#define DISPLAYSETTINGS_H

#include "texteditor_global.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT DisplaySettings
{
public:
    DisplaySettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    bool m_displayLineNumbers;
    bool m_textWrapping;
    bool m_visualizeWhitespace;
    bool m_displayFoldingMarkers;
    bool m_highlightCurrentLine;
    bool m_highlightBlocks;
    bool m_animateMatchingParentheses;
    bool m_highlightMatchingParentheses;
    bool m_markTextChanges;
    bool m_autoFoldFirstComment;
    bool m_centerCursorOnScroll;
    bool m_openLinksInNextSplit;
    bool m_forceOpenLinksInNextSplit;
    bool m_displayFileEncoding;
    bool m_scrollBarHighlights;

    bool equals(const DisplaySettings &ds) const;
};

inline bool operator==(const DisplaySettings &t1, const DisplaySettings &t2) { return t1.equals(t2); }
inline bool operator!=(const DisplaySettings &t1, const DisplaySettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // DISPLAYSETTINGS_H
