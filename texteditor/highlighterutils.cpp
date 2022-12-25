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

#include "highlighterutils.h"
#include "generichighlighter/highlighter.h"
#include "generichighlighter/highlightdefinition.h"
#include "generichighlighter/manager.h"
#include <core/icore.h>
#include <utils/mimetypes/mimetype.h>

using namespace TextEditor;
using namespace Internal;

void TextEditor::setMimeTypeForHighlighter(Highlighter *highlighter, const Utils::MimeType &mimeType,
                                           const QString &filePath, QString *foundDefinitionId)
{
    QString definitionId = Manager::instance()->definitionIdByMimeTypeAndFile(mimeType, filePath);
    if (!definitionId.isEmpty()) {
        const QSharedPointer<HighlightDefinition> &definition =
            Manager::instance()->definition(definitionId);
        if (!definition.isNull() && definition->isValid())
            highlighter->setDefaultContext(definition->initialContext());
    }

    if (foundDefinitionId)
        *foundDefinitionId = definitionId;
}

