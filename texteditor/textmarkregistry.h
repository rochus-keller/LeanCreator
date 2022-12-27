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

#ifndef BASETEXTMARKREGISTRY_H
#define BASETEXTMARKREGISTRY_H

#include <utils/fileutils.h>

#include <QColor>
#include <QHash>
#include <QObject>
#include <QSet>

#include <core/id.h>
#include <utils/theme.h>

namespace Core {
class IEditor;
class IDocument;
}

namespace TextEditor {
class TextMark;
namespace Internal {

class TextMarkRegistry : public QObject
{
    Q_OBJECT
public:
    TextMarkRegistry(QObject *parent);

    void add(TextMark *mark);
    bool remove(TextMark *mark);
    Utils::Theme::Color categoryColor(Core::Id category);
    bool categoryHasColor(Core::Id category);
    void setCategoryColor(Core::Id category, Utils::Theme::Color color);
private slots:
    void editorOpened(Core::IEditor *editor);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void allDocumentsRenamed(const QString &oldName, const QString &newName);
private:
    QHash<Utils::FileName, QSet<TextMark *> > m_marks;
    QHash<Core::Id, Utils::Theme::Color> m_colors;
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTMARKREGISTRY_H
