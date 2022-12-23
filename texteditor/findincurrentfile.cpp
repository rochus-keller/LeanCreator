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

#include "findincurrentfile.h"
#include "texteditor.h"
#include "textdocument.h"

#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <core/icore.h>
#include <core/editormanager/ieditor.h>
#include <core/editormanager/editormanager.h>

#include <QSettings>

using namespace TextEditor;
using namespace TextEditor::Internal;

FindInCurrentFile::FindInCurrentFile()
  : m_currentDocument(0)
{
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(handleFileChange(Core::IEditor*)));
    handleFileChange(Core::EditorManager::currentEditor());
}

QString FindInCurrentFile::id() const
{
    return QLatin1String("Current File");
}

QString FindInCurrentFile::displayName() const
{
    return tr("Current File");
}

Utils::FileIterator *FindInCurrentFile::files(const QStringList &nameFilters,
                                              const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    QString fileName = additionalParameters.toString();
    QMap<QString, QTextCodec *> openEditorEncodings = TextDocument::openedTextDocumentEncodings();
    QTextCodec *codec = openEditorEncodings.value(fileName);
    if (!codec)
        codec = Core::EditorManager::defaultTextCodec();
    return new Utils::FileListIterator(QStringList(fileName), QList<QTextCodec *>() << codec);
}

QVariant FindInCurrentFile::additionalParameters() const
{
    return qVariantFromValue(m_currentDocument->filePath().toString());
}

QString FindInCurrentFile::label() const
{
    return tr("File \"%1\":").arg(m_currentDocument->filePath().fileName());
}

QString FindInCurrentFile::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return tr("File path: %1\n%2").arg(m_currentDocument->filePath().toUserOutput());
}

bool FindInCurrentFile::isEnabled() const
{
    return m_currentDocument && !m_currentDocument->filePath().isEmpty();
}

void FindInCurrentFile::handleFileChange(Core::IEditor *editor)
{
    if (!editor) {
        if (m_currentDocument) {
            m_currentDocument = 0;
            emit enabledChanged(isEnabled());
        }
    } else {
        Core::IDocument *document = editor->document();
        if (document != m_currentDocument) {
            m_currentDocument = document;
            emit enabledChanged(isEnabled());
        }
    }
}


void FindInCurrentFile::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInCurrentFile::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    readCommonSettings(settings, QLatin1String("*"));
    settings->endGroup();
}
