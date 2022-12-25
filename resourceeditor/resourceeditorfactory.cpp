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

#include "resourceeditorfactory.h"
#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <core/fileiconprovider.h>
#include <core/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <qdebug.h>

using namespace ResourceEditor::Internal;
using namespace ResourceEditor::Constants;

ResourceEditorFactory::ResourceEditorFactory(ResourceEditorPlugin *plugin) :
    Core::IEditorFactory(plugin),
    m_plugin(plugin)
{
    setId(RESOURCEEDITOR_ID);
    setMimeTypes(QStringList(QLatin1String(C_RESOURCE_MIMETYPE)));
    setDisplayName(qApp->translate("OpenWith::Editors", C_RESOURCEEDITOR_DISPLAY_NAME));

    Core::FileIconProvider::registerIconOverlayForSuffix(":/resourceeditor/images/qt_qrc.png", "qrc");
}

Core::IEditor *ResourceEditorFactory::createEditor()
{
    Core::Context context(C_RESOURCEEDITOR);
    return new ResourceEditorW(context, m_plugin);
}
