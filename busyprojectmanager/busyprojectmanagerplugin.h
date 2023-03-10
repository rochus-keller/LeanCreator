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

#ifndef BUSYPROJECTMANAGERPLUGIN_H
#define BUSYPROJECTMANAGERPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <utils/parameteraction.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class ProjectExplorerPlugin;
class Node;
class Target;
} // namespace ProjectExplorer

namespace BusyProjectManager {
namespace Internal {

class BusyProject;

class BusyProjectManagerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BusyProjectManager.json")

public:
    BusyProjectManagerPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

private slots:
    void projectWasAdded(ProjectExplorer::Project *project);
    void currentProjectWasChanged(ProjectExplorer::Project *project);
    void projectWasRemoved();
    void nodeSelectionChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void buildStateChanged(ProjectExplorer::Project *project);
    void parsingStateChanged();
    void currentEditorChanged();

    void buildFileContextMenu();
    void buildFile();
    void buildProductContextMenu();
    void buildProduct();
    void buildSubprojectContextMenu();
    void buildSubproject();

    void reparseSelectedProject();
    void reparseCurrentProject();
    void reparseProject(BusyProject *project);

    void onFindUsages();

private:
    void updateContextActions();
    void updateReparseBusyAction();
    void updateBuildActions();

    void buildFiles(BusyProject *project, const QStringList &files,
                    const QStringList &activeFileTags);
    void buildSingleFile(BusyProject *project, const QString &file);
    void buildProducts(BusyProject *project, const QStringList &products);

    QAction *m_reparseBusy;
    QAction *m_reparseBusyCtx;
    QAction *m_buildFileCtx;
    QAction *m_buildProductCtx;
    QAction *m_buildSubprojectCtx;
    QAction* d_findUsagesAction;
    Utils::ParameterAction *m_buildFile;
    Utils::ParameterAction *m_buildProduct;
    Utils::ParameterAction *m_buildSubproject;

    Internal::BusyProject *m_selectedProject;
    ProjectExplorer::Node *m_selectedNode;

    Internal::BusyProject *m_currentProject;

    Internal::BusyProject *m_editorProject;
    ProjectExplorer::Node *m_editorNode;
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYPROJECTMANAGERPLUGIN_H
