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

#include "snippetssettings.h"
#include "reuse.h"

#include <QSettings>

namespace {

static const QLatin1String kGroupPostfix("SnippetsSettings");
static const QLatin1String kLastUsedSnippetGroup("LastUsedSnippetGroup");

} // Anonymous

using namespace TextEditor;
using namespace Internal;

SnippetsSettings::SnippetsSettings()
{}

void SnippetsSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = category + kGroupPostfix;
    s->beginGroup(group);
    s->setValue(kLastUsedSnippetGroup, m_lastUsedSnippetGroup);
    s->endGroup();
}

void SnippetsSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = category + kGroupPostfix;
    s->beginGroup(group);
    m_lastUsedSnippetGroup = s->value(kLastUsedSnippetGroup, QString()).toString();
    s->endGroup();
}

void SnippetsSettings::setLastUsedSnippetGroup(const QString &lastUsed)
{
    m_lastUsedSnippetGroup = lastUsed;
}

const QString &SnippetsSettings::lastUsedSnippetGroup() const
{
    return m_lastUsedSnippetGroup;
}

bool SnippetsSettings::equals(const SnippetsSettings &snippetsSettings) const
{
    return m_lastUsedSnippetGroup == snippetsSettings.m_lastUsedSnippetGroup;
}
