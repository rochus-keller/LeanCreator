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

#include "ansiescapecodehandler.h"
#include "outputformatter.h"
#include "theme.h"

#include <QPlainTextEdit>
#include <QTextCursor>

namespace Utils {

namespace Internal {

class OutputFormatterPrivate
{
public:
    OutputFormatterPrivate()
        : plainTextEdit(0), overwriteOutput(false)
    {}

    QPlainTextEdit *plainTextEdit;
    QTextCharFormat formats[NumberOfFormats];
    QFont font;
    QTextCursor cursor;
    AnsiEscapeCodeHandler escapeCodeHandler;
    bool overwriteOutput;
};

} // namespace Internal

OutputFormatter::OutputFormatter()
    : d(new Internal::OutputFormatterPrivate)
{
}

OutputFormatter::~OutputFormatter()
{
    delete d;
}

QPlainTextEdit *OutputFormatter::plainTextEdit() const
{
    return d->plainTextEdit;
}

void OutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    d->plainTextEdit = plainText;
    d->cursor = plainText ? plainText->textCursor() : QTextCursor();
    initFormats();
}

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
    appendMessage(text, d->formats[format]);
}

void OutputFormatter::appendMessage(const QString &text, const QTextCharFormat &format)
{
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);

    foreach (const FormattedText &output, parseAnsi(text, format)) {
        int startPos = 0;
        int crPos = -1;
        while ((crPos = output.text.indexOf(QLatin1Char('\r'), startPos)) >= 0)  {
            append(d->cursor, output.text.mid(startPos, crPos - startPos), output.format);
            startPos = crPos + 1;
            d->overwriteOutput = true;
        }
        if (startPos < output.text.count())
            append(d->cursor, output.text.mid(startPos), output.format);
    }
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return d->formats[format];
}

QList<FormattedText> OutputFormatter::parseAnsi(const QString &text, const QTextCharFormat &format)
{
    return d->escapeCodeHandler.parseText(FormattedText(text, format));
}

void OutputFormatter::append(QTextCursor &cursor, const QString &text,
                             const QTextCharFormat &format)
{
    if (d->overwriteOutput) {
        cursor.clearSelection();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        d->overwriteOutput = false;
    }
    cursor.insertText(text, format);
}

void OutputFormatter::clearLastLine()
{
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    d->cursor.removeSelectedText();
}

void OutputFormatter::initFormats()
{
    if (!plainTextEdit())
        return;

    QFont boldFont;
    boldFont.setBold(true);

    Theme *theme = creatorTheme();

    // NormalMessageFormat
    d->formats[NormalMessageFormat].setFont(boldFont, QTextCharFormat::FontPropertiesSpecifiedOnly);
    d->formats[NormalMessageFormat].setForeground(theme->color(Theme::OutputPanes_NormalMessageTextColor));

    // ErrorMessageFormat
    d->formats[ErrorMessageFormat].setFont(boldFont, QTextCharFormat::FontPropertiesSpecifiedOnly);
    d->formats[ErrorMessageFormat].setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));

    // StdOutFormat
    d->formats[StdOutFormat].setFont(d->font, QTextCharFormat::FontPropertiesSpecifiedOnly);
    d->formats[StdOutFormat].setForeground(theme->color(Theme::OutputPanes_StdOutTextColor));
    d->formats[StdOutFormatSameLine] = d->formats[StdOutFormat];

    // StdErrFormat
    d->formats[StdErrFormat].setFont(d->font, QTextCharFormat::FontPropertiesSpecifiedOnly);
    d->formats[StdErrFormat].setForeground(theme->color(Theme::OutputPanes_StdErrTextColor));
    d->formats[StdErrFormatSameLine] = d->formats[StdErrFormat];

    d->formats[DebugFormat].setFont(d->font, QTextCharFormat::FontPropertiesSpecifiedOnly);
    d->formats[DebugFormat].setForeground(theme->color(Theme::OutputPanes_DebugTextColor));
}

void OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href);
}

void OutputFormatter::flush()
{
    d->escapeCodeHandler.endFormatScope();
}

} // namespace Utils
