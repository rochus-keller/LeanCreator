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

#include "commandsfile.h"
#include "command_p.h"
#include <core/dialogs/shortcutsettings.h>

#include <app/app_version.h>

#include <utils/qtcassert.h>

#include <utils/fileutils.h>

#include <QKeySequence>
#include <QFile>
#include <QXmlStreamAttributes>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDebug>
#include <QDateTime>

namespace Core {
namespace Internal {

struct Context // XML parsing context with strings.
{
    Context();

    const QString mappingElement;
    const QString shortCutElement;
    const QString idAttribute;
    const QString keyElement;
    const QString valueAttribute;
};

Context::Context() :
    mappingElement(QLatin1String("mapping")),
    shortCutElement(QLatin1String("shortcut")),
    idAttribute(QLatin1String("id")),
    keyElement(QLatin1String("key")),
    valueAttribute(QLatin1String("value"))
{
}

/*!
    \class CommandsFile
    \brief The CommandsFile class provides a collection of import and export commands.
    \inheaderfile commandsfile.h
*/

/*!
    ...
*/
CommandsFile::CommandsFile(const QString &filename)
    : m_filename(filename)
{

}

/*!
    ...
*/
QMap<QString, QKeySequence> CommandsFile::importCommands() const
{
    QMap<QString, QKeySequence> result;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return result;

    Context ctx;
    QXmlStreamReader r(&file);

    QString currentId;

    while (!r.atEnd()) {
        switch (r.readNext()) {
        case QXmlStreamReader::StartElement: {
            const QStringRef name = r.name();
            if (name == ctx.shortCutElement) {
                if (!currentId.isEmpty()) // shortcut element without key element == empty shortcut
                    result.insert(currentId, QKeySequence());
                currentId = r.attributes().value(ctx.idAttribute).toString();
            } else if (name == ctx.keyElement) {
                QTC_ASSERT(!currentId.isEmpty(), return result);
                const QXmlStreamAttributes attributes = r.attributes();
                if (attributes.hasAttribute(ctx.valueAttribute)) {
                    const QString keyString = attributes.value(ctx.valueAttribute).toString();
                    result.insert(currentId, QKeySequence(keyString));
                } else {
                    result.insert(currentId, QKeySequence());
                }
                currentId.clear();
            } // if key element
        } // case QXmlStreamReader::StartElement
        default:
            break;
        } // switch
    } // while !atEnd
    file.close();
    return result;
}

/*!
    ...
*/

bool CommandsFile::exportCommands(const QList<ShortcutItem *> &items)
{
    Utils::FileSaver saver(m_filename, QIODevice::Text);
    if (!saver.hasError()) {
        const Context ctx;
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(1); // Historical, used to be QDom.
        w.writeStartDocument();
        w.writeDTD(QLatin1String("<!DOCTYPE KeyboardMappingScheme>"));
        w.writeComment(QString::fromLatin1(" Written by LeanCreator %1, %2. ").
                       arg(QLatin1String(Constants::IDE_VERSION_LONG),
                           QDateTime::currentDateTime().toString(Qt::ISODate)));
        w.writeStartElement(ctx.mappingElement);
        foreach (const ShortcutItem *item, items) {
            const Id id = item->m_cmd->id();
            if (item->m_key.isEmpty()) {
                w.writeEmptyElement(ctx.shortCutElement);
                w.writeAttribute(ctx.idAttribute, id.toString());
            } else {
                w.writeStartElement(ctx.shortCutElement);
                w.writeAttribute(ctx.idAttribute, id.toString());
                w.writeEmptyElement(ctx.keyElement);
                w.writeAttribute(ctx.valueAttribute, item->m_key.toString());
                w.writeEndElement(); // Shortcut
            }
        }
        w.writeEndElement();
        w.writeEndDocument();

        saver.setResult(&w);
    }
    return saver.finalize();
}

} // namespace Internal
} // namespace Core
