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

#include "linenumberfilter.h"
#include "texteditor.h"

#include <core/coreconstants.h>
#include <core/editormanager/editormanager.h>
#include <core/icore.h>
#include <core/modemanager.h>

#include <QMetaType>
#include <QPair>
#include <QVariant>

typedef QPair<int,int> LineColumn;
Q_DECLARE_METATYPE(LineColumn)

using namespace Core;
using namespace TextEditor;
using namespace TextEditor::Internal;

LineNumberFilter::LineNumberFilter(QObject *parent)
  : ILocatorFilter(parent)
{
    setId("Line in current document");
    setDisplayName(tr("Line in Current Document"));
    setPriority(High);
    setShortcutString(QString(QLatin1Char('l')));
    setIncludedByDefault(true);
}

void LineNumberFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_hasCurrentEditor = EditorManager::currentEditor() != 0;
}

QList<LocatorFilterEntry> LineNumberFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &, const QString &entry)
{
    QList<LocatorFilterEntry> value;
    QStringList lineAndColumn = entry.split(QLatin1Char(':'));
    int sectionCount = lineAndColumn.size();
    int line = 0;
    int column = 0;
    bool ok = false;
    if (sectionCount > 0)
        line = lineAndColumn.at(0).toInt(&ok);
    if (ok && sectionCount > 1)
        column = lineAndColumn.at(1).toInt(&ok);
    if (!ok)
        return value;
    if (m_hasCurrentEditor && (line > 0 || column > 0)) {
        LineColumn data;
        data.first = line;
        data.second = column - 1;  // column API is 0-based
        QString text;
        if (line > 0 && column > 0)
            text = tr("Line %1, Column %2").arg(line).arg(column);
        else if (line > 0)
            text = tr("Line %1").arg(line);
        else
            text = tr("Column %1").arg(column);
        value.append(LocatorFilterEntry(this, text, QVariant::fromValue(data)));
    }
    return value;
}

void LineNumberFilter::accept(LocatorFilterEntry selection) const
{
    IEditor *editor = EditorManager::currentEditor();
    if (editor) {
        EditorManager::addCurrentPositionToNavigationHistory();
        LineColumn data = selection.internalData.value<LineColumn>();
        if (data.first < 1)  // jump to column in same line
            data.first = editor->currentLine();
        editor->gotoLine(data.first, data.second);
        EditorManager::activateEditor(editor);
    }
}
