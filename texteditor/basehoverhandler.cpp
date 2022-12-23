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

#include "basehoverhandler.h"
#include "texteditor.h"

#include <core/icore.h>
#include <utils/tooltip.h>

#include <QPoint>

using namespace Core;

namespace TextEditor {

BaseHoverHandler::BaseHoverHandler() : m_diagnosticTooltip(false)
{
}

BaseHoverHandler::~BaseHoverHandler()
{}

void BaseHoverHandler::showToolTip(TextEditorWidget *widget, const QPoint &point, int pos)
{
    widget->setContextHelpId(QString());

    process(widget, pos);
    operateTooltip(widget, point);
}

QString BaseHoverHandler::contextHelpId(TextEditorWidget *widget, int pos)
{
    // If the tooltip is visible and there is a help match, this match is used to update
    // the help id. Otherwise, let the identification process happen.
    if (!Utils::ToolTip::isVisible() || !lastHelpItemIdentified().isValid())
        process(widget, pos);

    if (lastHelpItemIdentified().isValid())
        return lastHelpItemIdentified().helpId();
    return QString();
}

void BaseHoverHandler::setToolTip(const QString &tooltip)
{
    m_toolTip = tooltip;
}

const QString &BaseHoverHandler::toolTip() const
{
    return m_toolTip;
}

void BaseHoverHandler::appendToolTip(const QString &extension)
{
    m_toolTip.append(extension);
}

void BaseHoverHandler::setIsDiagnosticTooltip(bool isDiagnosticTooltip)
{
    m_diagnosticTooltip = isDiagnosticTooltip;
}

bool BaseHoverHandler::isDiagnosticTooltip() const
{
    return m_diagnosticTooltip;
}

void BaseHoverHandler::setLastHelpItemIdentified(const HelpItem &help)
{
    m_lastHelpItemIdentified = help;
}

const HelpItem &BaseHoverHandler::lastHelpItemIdentified() const
{
    return m_lastHelpItemIdentified;
}

void BaseHoverHandler::clear()
{
    m_diagnosticTooltip = false;
    m_toolTip.clear();
    m_lastHelpItemIdentified = HelpItem();
}

void BaseHoverHandler::process(TextEditorWidget *widget, int pos)
{
    clear();
    identifyMatch(widget, pos);
    decorateToolTip();
}

void BaseHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    if (!isDiagnosticTooltip() && lastHelpItemIdentified().isValid()) {
        const QString &contents = lastHelpItemIdentified().extractContent(false);
        if (!contents.isEmpty()) {
            setToolTip(toolTip().toHtmlEscaped());
            appendToolTip(contents);
        }
    }
}

void BaseHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_toolTip.isEmpty())
        Utils::ToolTip::hide();
    else
        Utils::ToolTip::show(point, m_toolTip, editorWidget, m_lastHelpItemIdentified.isValid()
                             ? m_lastHelpItemIdentified.helpId()
                             : QString());
}

} // namespace TextEditor
