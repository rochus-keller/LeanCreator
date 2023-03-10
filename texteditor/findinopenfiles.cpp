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

#include "findinopenfiles.h"
#include "textdocument.h"
#include "texteditor.h"

#include <utils/filesearch.h>
#include <core/icore.h>
#include <core/editormanager/editormanager.h>
#include <core/editormanager/documentmodel.h>

#include <QSettings>

using namespace TextEditor;
using namespace TextEditor::Internal;

FindInOpenFiles::FindInOpenFiles()
{
    connect(Core::EditorManager::instance(), SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(updateEnabledState()));
    connect(Core::EditorManager::instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(updateEnabledState()));
}

QString FindInOpenFiles::id() const
{
    return QLatin1String("Open Files");
}

QString FindInOpenFiles::displayName() const
{
    return tr("Open Documents");
}

Utils::FileIterator *FindInOpenFiles::files(const QStringList &nameFilters,
                                              const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    Q_UNUSED(additionalParameters)
    QMap<QString, QTextCodec *> openEditorEncodings
            = TextDocument::openedTextDocumentEncodings();
    QStringList fileNames;
    QList<QTextCodec *> codecs;
    foreach (Core::DocumentModel::Entry *entry,
             Core::DocumentModel::entries()) {
        QString fileName = entry->fileName().toString();
        if (!fileName.isEmpty()) {
            fileNames.append(fileName);
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = Core::EditorManager::defaultTextCodec();
            codecs.append(codec);
        }
    }

    return new Utils::FileListIterator(fileNames, codecs);
}

QVariant FindInOpenFiles::additionalParameters() const
{
    return QVariant();
}

QString FindInOpenFiles::label() const
{
    return tr("Open documents:");
}

QString FindInOpenFiles::toolTip() const
{
    // %1 is filled by BaseFileFind::runNewSearch
    return tr("Open Documents\n%1");
}

bool FindInOpenFiles::isEnabled() const
{
    return Core::DocumentModel::entryCount() > 0;
}

void FindInOpenFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInOpenFiles::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    readCommonSettings(settings, QLatin1String("*"));
    settings->endGroup();
}

void FindInOpenFiles::updateEnabledState()
{
    emit enabledChanged(isEnabled());
}
