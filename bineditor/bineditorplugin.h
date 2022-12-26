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

#ifndef BINEDITORPLUGIN_H
#define BINEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <core/editormanager/ieditorfactory.h>
#include <core/icontext.h>

#include <QPointer>
#include <QStringList>
#include <QAction>

namespace BinEditor {
class BinEditorWidget;

namespace Internal {
class BinEditorFactory;

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

public:
    BinEditorPlugin();
    ~BinEditorPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

    // Connect editor to settings changed signals.
    void initializeEditor(BinEditorWidget *editor);

private slots:
    void undoAction();
    void redoAction();
    void copyAction();
    void selectAllAction();
    void updateActions();

    void updateCurrentEditor(Core::IEditor *editor);

private:
    Core::Context m_context;
    QAction *registerNewAction(Core::Id id, const QString &title = QString());
    QAction *registerNewAction(Core::Id id, QObject *receiver, const char *slot,
                               const QString &title = QString());
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_copyAction;
    QAction *m_selectAllAction;

    QPointer<BinEditorWidget> m_currentEditor;
};

class BinEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    explicit BinEditorFactory(BinEditorPlugin *owner);

    Core::IEditor *createEditor();

private:
    BinEditorPlugin *m_owner;
};

} // namespace Internal
} // namespace BinEditor

#endif // BINEDITORPLUGIN_H
