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

#include "busyprojectmanagerplugin.h"

#include "busybuildconfiguration.h"
#include "busybuildstep.h"
#include "busycleanstep.h"
#include "busydeployconfigurationfactory.h"
#include "busyinstallstep.h"
#include "busynodes.h"
#include "busyprofilessettingspage.h"
#include "busyproject.h"
#include "busyprojectmanager.h"
#include "busyprojectmanagerconstants.h"
#include "busyrunconfiguration.h"
#include "busyeditor.h"

#include <core/actionmanager/actioncontainer.h>
#include <core/actionmanager/actionmanager.h>
#include <core/coreconstants.h>
#include <core/editormanager/editormanager.h>
#include <core/editormanager/ieditor.h>
#include <core/featureprovider.h>
#include <core/icore.h>
#include <core/idocument.h>
#include <core/fileiconprovider.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <texteditor/texteditorsettings.h>
//#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QAction>
#include <QtPlugin>

using namespace ProjectExplorer;

namespace BusyProjectManager {
namespace Internal {

static Node *currentEditorNode()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? SessionManager::nodeForFile(doc->filePath()) : 0;
}

static BusyProject *currentEditorProject()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? qobject_cast<BusyProject *>(SessionManager::projectForFile(doc->filePath())) : 0;
}

BusyProjectManagerPlugin::BusyProjectManagerPlugin() :
    m_selectedProject(0),
    m_selectedNode(0),
    m_currentProject(0),
    m_editorProject(0),
    m_editorNode(0)
{
}

bool BusyProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    const Core::Context projectContext(::BusyProjectManager::Constants::PROJECT_ID);

    // TODO ModelManager::instance()->setParent(this);

    // Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "qbs");
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/busyprojectmanager/BusyProjectManager.mimetypes.xml"));
    TextEditor::TextEditorSettings::registerMimeTypeForLanguageId(Constants::MIME_TYPE, Constants::LANG);

    //create and register objects
    addAutoReleasedObject(new EditorFactory);
    addAutoReleasedObject(new BusyManager);
    addAutoReleasedObject(new BusyBuildConfigurationFactory);
    addAutoReleasedObject(new BusyBuildStepFactory);
    addAutoReleasedObject(new BusyCleanStepFactory);
    addAutoReleasedObject(new BusyInstallStepFactory);
    addAutoReleasedObject(new BusyDeployConfigurationFactory);
    addAutoReleasedObject(new BusyRunConfigurationFactory);
    addAutoReleasedObject(new BusyProfilesSettingsPage);

    //menus
    // Build Menu:
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    // PE Context menu for projects
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
             Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);


    //register actions
    Core::Command *command;

    m_reparseBusy = new QAction(tr("Reparse Busy"), this);
    command = Core::ActionManager::registerAction(m_reparseBusy, Constants::ACTION_REPARSE_BUSY, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reparseBusy, SIGNAL(triggered()), this, SLOT(reparseCurrentProject()));

    m_reparseBusyCtx = new QAction(tr("Reparse Busy"), this);
    command = Core::ActionManager::registerAction(m_reparseBusyCtx, Constants::ACTION_REPARSE_BUSY_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_reparseBusyCtx, SIGNAL(triggered()), this, SLOT(reparseSelectedProject()));

    m_buildFileCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileCtx, Constants::ACTION_BUILD_FILE_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileCtx, SIGNAL(triggered()), this, SLOT(buildFileContextMenu()));

    m_buildFile = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildFile, Constants::ACTION_BUILD_FILE);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFile, SIGNAL(triggered()), this, SLOT(buildFile()));

    m_buildProductCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildProductCtx, Constants::ACTION_BUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildProductCtx, SIGNAL(triggered()), this, SLOT(buildProductContextMenu()));

    m_buildProduct = new Utils::ParameterAction(tr("Build Product"), tr("Build Product \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildProduct, Constants::ACTION_BUILD_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildProduct, SIGNAL(triggered()), this, SLOT(buildProduct()));

    m_buildSubprojectCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildSubprojectCtx, Constants::ACTION_BUILD_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubprojectCtx, SIGNAL(triggered()), this, SLOT(buildSubprojectContextMenu()));

    m_buildSubproject = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildSubproject, Constants::ACTION_BUILD_SUBPROJECT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubproject, SIGNAL(triggered()), this, SLOT(buildSubproject()));

    // Connect
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &BusyProjectManagerPlugin::nodeSelectionChanged);

    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged()));

    connect(SessionManager::instance(), SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectWasAdded(ProjectExplorer::Project*)));
    connect(SessionManager::instance(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectWasRemoved()));
    connect(SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectWasChanged(ProjectExplorer::Project*)));

    // Run initial setup routines
    updateContextActions();
    updateReparseBusyAction();
    updateBuildActions();

    return true;
}

void BusyProjectManagerPlugin::extensionsInitialized()
{ }

void BusyProjectManagerPlugin::projectWasAdded(Project *project)
{
    BusyProject *qbsProject = qobject_cast<BusyProject *>(project);

    if (!qbsProject)
        return;

    connect(qbsProject, SIGNAL(projectParsingStarted()), this, SLOT(parsingStateChanged()));
    connect(qbsProject, SIGNAL(projectParsingDone(bool)), this, SLOT(parsingStateChanged()));
}

void BusyProjectManagerPlugin::currentProjectWasChanged(Project *project)
{
    m_currentProject = qobject_cast<BusyProject *>(project);

    updateReparseBusyAction();
}

void BusyProjectManagerPlugin::projectWasRemoved()
{
    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();

    updateBuildActions();
}

void BusyProjectManagerPlugin::nodeSelectionChanged(Node *node, Project *project)
{
    m_selectedNode = node;
    m_selectedProject = qobject_cast<Internal::BusyProject *>(project);

    updateContextActions();
}

void BusyProjectManagerPlugin::updateContextActions()
{
    bool isEnabled = !BuildManager::isBuilding(m_selectedProject)
            && m_selectedProject && !m_selectedProject->isParsing()
            && m_selectedNode && m_selectedNode->isEnabled();

    bool isFile = m_selectedProject && m_selectedNode && (m_selectedNode->nodeType() == FileNodeType);
    bool isProduct = m_selectedProject && m_selectedNode && dynamic_cast<BusyProductNode *>(m_selectedNode->projectNode());
    BusyProjectNode *subproject = dynamic_cast<BusyProjectNode *>(m_selectedNode);
    bool isSubproject = m_selectedProject && subproject && subproject != m_selectedProject->rootProjectNode();

    m_reparseBusyCtx->setEnabled(isEnabled);
    m_buildFileCtx->setEnabled(isEnabled && isFile);
    m_buildProductCtx->setVisible(isEnabled && isProduct);
    m_buildSubprojectCtx->setVisible(isEnabled && isSubproject);
}

void BusyProjectManagerPlugin::updateReparseBusyAction()
{
    m_reparseBusy->setEnabled(m_currentProject
                             && !BuildManager::isBuilding(m_currentProject)
                             && !m_currentProject->isParsing());
}

void BusyProjectManagerPlugin::updateBuildActions()
{
    bool enabled = false;
    bool fileVisible = false;
    bool productVisible = false;
    bool subprojectVisible = false;

    QString fileName;
    QString productName;
    QString subprojectName;

    if (m_editorNode) {
        enabled = m_editorProject
                && !BuildManager::isBuilding(m_editorProject)
                && !m_editorProject->isParsing();

        fileName = m_editorNode->path().fileName();
        fileVisible = m_editorProject && m_editorNode && dynamic_cast<BusyBaseProjectNode *>(m_editorNode->projectNode());

        BusyProductNode *productNode
                = dynamic_cast<BusyProductNode *>(m_editorNode ? m_editorNode->projectNode() : 0);
        if (productNode) {
            productVisible = true;
            productName = productNode->displayName();
        }
        BusyProjectNode *subprojectNode
                = dynamic_cast<BusyProjectNode *>(productNode ? productNode->parentFolderNode() : 0);
        if (subprojectNode && m_editorProject && subprojectNode != m_editorProject->rootProjectNode()) {
            subprojectVisible = true;
            subprojectName = subprojectNode->displayName();
        }
    }

    m_buildFile->setEnabled(enabled);
    m_buildFile->setVisible(fileVisible);
    m_buildFile->setParameter(fileName);

    m_buildProduct->setEnabled(enabled);
    m_buildProduct->setVisible(productVisible);
    m_buildProduct->setParameter(productName);

    m_buildSubproject->setEnabled(enabled);
    m_buildSubproject->setVisible(subprojectVisible);
    m_buildSubproject->setParameter(subprojectName);
}

void BusyProjectManagerPlugin::buildStateChanged(Project *project)
{
    if (project == m_currentProject)
        updateReparseBusyAction();

    if (project == m_selectedProject)
        updateContextActions();

    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();
    if (project == m_editorProject)
        updateBuildActions();
}

void BusyProjectManagerPlugin::parsingStateChanged()
{
    BusyProject *project = qobject_cast<BusyProject *>(sender());

    if (!project || project == m_currentProject)
        updateReparseBusyAction();

    if (!project || project == m_selectedProject)
        updateContextActions();

    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();
    if (!project || project == m_editorProject)
        updateBuildActions();
}

void BusyProjectManagerPlugin::currentEditorChanged()
{
    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();

    updateBuildActions();
}

void BusyProjectManagerPlugin::buildFileContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    buildSingleFile(m_selectedProject, m_selectedNode->path().toString());
}

void BusyProjectManagerPlugin::buildFile()
{
    if (!m_editorProject || !m_editorNode)
        return;

    buildSingleFile(m_editorProject, m_editorNode->path().toString());
}

void BusyProjectManagerPlugin::buildProductContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    const BusyProductNode * const productNode = dynamic_cast<BusyProductNode *>(m_selectedNode);
    QTC_ASSERT(productNode, return);

    buildProducts(m_selectedProject,
                  QStringList(BusyProject::uniqueProductName(productNode->busyProductData())));
}

void BusyProjectManagerPlugin::buildProduct()
{
    if (!m_editorProject || !m_editorNode)
        return;

    BusyProductNode *product = dynamic_cast<BusyProductNode *>(m_editorNode->projectNode());

    if (!product)
        return;

    buildProducts(m_editorProject,
                  QStringList(BusyProject::uniqueProductName(product->busyProductData())));
}

void BusyProjectManagerPlugin::buildSubprojectContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    BusyProjectNode *subProject = dynamic_cast<BusyProjectNode *>(m_selectedNode);
    QTC_ASSERT(subProject, return);

    QStringList toBuild;
    foreach (const busy::Product &data, subProject->busyProjectData().allProducts())
        toBuild << BusyProject::uniqueProductName(data);

    buildProducts(m_selectedProject, toBuild);
}

void BusyProjectManagerPlugin::buildSubproject()
{
    if (!m_editorNode || !m_editorProject)
        return;

    BusyProjectNode *subproject = 0;
    BusyBaseProjectNode *start = dynamic_cast<BusyBaseProjectNode *>(m_editorNode->projectNode());
    while (start && start != m_editorProject->rootProjectNode()) {
        BusyProjectNode *tmp = dynamic_cast<BusyProjectNode *>(start);
        if (tmp) {
            subproject = tmp;
            break;
        }
        start = dynamic_cast<BusyProjectNode *>(start->parentFolderNode());
    }

    if (!subproject)
        return;

    QStringList toBuild;
    foreach (const busy::Product &data, subproject->busyProjectData().allProducts())
        toBuild << BusyProject::uniqueProductName(data);

    buildProducts(m_editorProject, toBuild);
}

void BusyProjectManagerPlugin::buildFiles(BusyProject *project, const QStringList &files,
                                         const QStringList &activeFileTags)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!files.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    BusyBuildConfiguration *bc = qobject_cast<BusyBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    bc->setChangedFiles(files);
    bc->setActiveFileTags(activeFileTags);
    bc->setProducts(QStringList());

    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;

    const QString name = ProjectExplorerPlugin::displayNameForStepId(buildStep);
    BuildManager::buildList(bc->stepList(buildStep), name);

    bc->setChangedFiles(QStringList());
    bc->setActiveFileTags(QStringList());
}

void BusyProjectManagerPlugin::buildSingleFile(BusyProject *project, const QString &file)
{
    buildFiles(project, QStringList(file), QStringList()
               << QLatin1String("obj") << QLatin1String("hpp"));
}

void BusyProjectManagerPlugin::buildProducts(BusyProject *project, const QStringList &products)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!products.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    BusyBuildConfiguration *bc = qobject_cast<BusyBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    bc->setChangedFiles(QStringList());
    bc->setProducts(products);

    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;

    const QString name = ProjectExplorerPlugin::displayNameForStepId(buildStep);
    BuildManager::buildList(bc->stepList(buildStep), name);

    bc->setProducts(QStringList());
}

void BusyProjectManagerPlugin::reparseSelectedProject()
{
    reparseProject(m_selectedProject);
}

void BusyProjectManagerPlugin::reparseCurrentProject()
{
    reparseProject(m_currentProject);
}

void BusyProjectManagerPlugin::reparseProject(BusyProject *project)
{
    if (!project)
        return;

    // Busy does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(project))
        project->scheduleParsing();
    else
        project->parseCurrentBuildConfiguration();
}

} // namespace Internal
} // namespace BusyProjectManager
