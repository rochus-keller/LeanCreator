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

#include "editorwindow.h"

#include "editorarea.h"
#include "editormanager_p.h"

#include <core/icontext.h>
#include <core/icore.h>

#include <QVBoxLayout>

namespace Core {
namespace Internal {

EditorWindow::EditorWindow(QWidget *parent) :
    QWidget(parent)
{
    m_area = new EditorArea;
    auto layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(m_area);
    setFocusProxy(m_area);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false); // don't prevent LeanCreator from closing
    resize(QSize(800, 600));

    static int windowId = 0;
    ICore::registerWindow(this, Context(Id("EditorManager.ExternalWindow.").withSuffix(++windowId)));

    connect(m_area, &EditorArea::windowTitleNeedsUpdate,
            this, &EditorWindow::updateWindowTitle);
    // editor area can be deleted by editor manager
    connect(m_area, &EditorArea::destroyed, this, [this]() {
        m_area = nullptr;
        deleteLater();
    });
    updateWindowTitle();
}

EditorWindow::~EditorWindow()
{
    if (m_area)
        disconnect(m_area, 0, this, 0);
}

EditorArea *EditorWindow::editorArea() const
{
    return m_area;
}

void EditorWindow::updateWindowTitle()
{
    EditorManagerPrivate::updateWindowTitleForDocument(m_area->currentDocument(), this);
}

} // Internal
} // Core
