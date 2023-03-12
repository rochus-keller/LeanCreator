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

#include "busynodes.h"

#include "busyproject.h"
#include "busyprojectmanagerconstants.h"
#include "busyrunconfiguration.h"

#include <core/fileiconprovider.h>
#include <core/idocument.h>
#include <core/documentmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
//#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
extern "C" {
#include <bsrunner.h>
}

#include <QtDebug>
#include <QDir>
#include <QStyle>

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

static QString displayNameFromPath(const QString &path, const QString &base)
{
    QString dir = base;
    if (!base.endsWith(QLatin1Char('/')))
        dir.append(QLatin1Char('/'));

    QString name = path;
    if (name.startsWith(dir)) {
        name = name.mid(dir.count());
    } else {
        QFileInfo fi = QFileInfo(path);
        name = QCoreApplication::translate("Busy::BusyProjectNode", "%1 in %2")
                .arg(fi.fileName(), fi.absolutePath());
    }

    return name;
}

static QIcon generateIcon(const QString &overlay)
{
    const QSize desiredSize = QSize(16, 16);
    const QIcon overlayIcon(overlay);
    const QPixmap pixmap
            = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon, overlayIcon, desiredSize);

    QIcon result;
    result.addPixmap(pixmap);

    return result;
}

namespace BusyProjectManager {
namespace Internal {

//QIcon BusyGroupNode::m_groupIcon;
QIcon BusyProjectNode::m_projectIcon;
QIcon BusyProductNode::m_productIcon;

static BusyProjectNode *parentBusyProjectNode(ProjectExplorer::Node *node)
{
    for (ProjectExplorer::FolderNode *pn = node->projectNode(); pn; pn = pn->parentFolderNode()) {
        BusyProjectNode *prjNode = dynamic_cast<BusyProjectNode *>(pn);
        if (prjNode)
            return prjNode;
    }
    return 0;
}

static BusyProductNode *parentBusyProductNode(ProjectExplorer::Node *node)
{
    for (; node; node = node->parentFolderNode()) {
        BusyProductNode *prdNode = dynamic_cast<BusyProductNode *>(node);
        if (prdNode)
            return prdNode;
    }
    return 0;
}

#if 0
static busy::GroupData findMainBusyGroup(const busy::Product &productData)
{
    foreach (const busy::GroupData &grp, productData.groups()) {
        if (grp.name() == productData.name() && grp.location() == productData.location())
            return grp;
    }
    return busy::GroupData();
}
#endif

class FileTreeNode {
public:
    explicit FileTreeNode(const QString &n = QString(), FileTreeNode *p = 0, bool f = false) :
        parent(p), name(n), m_isFile(f)
    {
        if (p)
            p->children.append(this);
    }

    ~FileTreeNode()
    {
        qDeleteAll(children);
    }

    FileTreeNode *addPart(const QString &n, bool isFile)
    {
        foreach (FileTreeNode *c, children) {
            if (c->name == n)
                return c;
        }
        return new FileTreeNode(n, this, isFile);
    }

    bool isFile() const { return m_isFile; }

    static FileTreeNode *moveChildrenUp(FileTreeNode *node)
    {
        QTC_ASSERT(node, return 0);

        FileTreeNode *newParent = node->parent;
        if (!newParent)
            return 0;

        // disconnect node and parent:
        node->parent = 0;
        newParent->children.removeOne(node);

        foreach (FileTreeNode *c, node->children) {
            // update path, make sure there will be no / before "C:" on windows:
            if (Utils::HostOsInfo::isWindowsHost() && node->name.isEmpty())
                c->name = node->name;
            else
                c->name = node->name + QLatin1Char('/') + c->name;

            newParent->children.append(c);
            c->parent = newParent;
        }

        // Delete node
        node->children.clear();
        delete node;
        return newParent;
    }

    // Moves the children of the node pointing to basedir to the root of the tree.
    static void reorder(FileTreeNode *node, const QString &basedir)
    {
        QTC_CHECK(!basedir.isEmpty());
        QString prefix = basedir;
        if (basedir.startsWith(QLatin1Char('/')))
            prefix = basedir.mid(1);
        prefix.append(QLatin1Char('/'));

        if (node->path() == basedir) {
            // Find root node:
            FileTreeNode *root = node;
            while (root->parent)
                root = root->parent;

            foreach (FileTreeNode *c, node->children) {
                // Update children names by prepending basedir
                c->name = prefix + c->name;
                // Update parent information:
                c->parent = root;

                root->children.append(c);
            }

            // Clean up node:
            node->children.clear();
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;

            return;
        }

        foreach (FileTreeNode *n, node->children)
            reorder(n, basedir);
    }

    static void simplify(FileTreeNode *node)
    {
        foreach (FileTreeNode *c, node->children)
            simplify(c);

        if (!node->parent)
            return;

        if (node->children.isEmpty() && !node->isFile()) {
            // Clean up empty folder nodes:
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;
        } else if (node->children.count() == 1 && !node->children.at(0)->isFile()) {
            // Compact folder nodes with one child only:
            moveChildrenUp(node);
        }
    }

    QString path() const
    {
        QString p = name;
        FileTreeNode *node = parent;
        while (node) {
            if (!Utils::HostOsInfo::isWindowsHost() || !node->name.isEmpty())
                p = node->name + QLatin1Char('/') + p;
            node = node->parent;
        }
        return p;
    }

    QList<FileTreeNode *> children;
    FileTreeNode *parent;
    QString name;
    bool m_isFile;
};


static QList<ProjectExplorer::ProjectAction> supportedNodeActions(ProjectExplorer::Node *node,
                                                                  bool managesFiles)
{
    QList<ProjectExplorer::ProjectAction> actions;
    const BusyProject * const project = parentBusyProjectNode(node)->project();
    if (!project->isProjectEditable())
        return actions;
    if (managesFiles)
    {
#if 0
        // doesn't work since this function is not called everytime when the menu is opened
        QList<Core::IDocument *> mods = Core::DocumentManager::modifiedDocuments();
        foreach( Core::IDocument * doc, mods )
        {
            if( doc->filePath() == node->path() )
                return actions; // don't allow this on BUSY files with changes in editor
        }
#endif
        actions << ProjectExplorer::AddNewFile << ProjectExplorer::AddExistingFile;
    }
#if 0
    // we don't support rename and remove on files in BUSY projects
    if (node->nodeType() == ProjectExplorer::FileNodeType
            && !project->busyProject().buildSystemFiles().contains(node->path().toString())) {
        actions << ProjectExplorer::RemoveFile << ProjectExplorer::Rename;
    }
#endif
    return actions;
}

// ----------------------------------------------------------------------
// BusyFileNode:
// ----------------------------------------------------------------------

BusyFileNode::BusyFileNode(const Utils::FileName &filePath,
                         const ProjectExplorer::FileType fileType,
                         bool generated,
                         int line) :
    ProjectExplorer::FileNode(filePath, fileType, generated, line)
{ }

QString BusyFileNode::displayName() const
{
    int l = line();
    if (l < 0)
        return ProjectExplorer::FileNode::displayName();
    return ProjectExplorer::FileNode::displayName() + QLatin1Char(':') + QString::number(l);
}


BusyFolderNode::BusyFolderNode(const Utils::FileName &folderPath, ProjectExplorer::NodeType nodeType,
                             const QString &displayName)
    : ProjectExplorer::FolderNode(folderPath, nodeType, displayName)
{
}

QList<ProjectExplorer::ProjectAction> BusyFolderNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, false);
}

// ---------------------------------------------------------------------------
// BusyBaseProjectNode:
// ---------------------------------------------------------------------------

BusyBaseProjectNode::BusyBaseProjectNode(const Utils::FileName &path) :
    ProjectExplorer::ProjectNode(path)
{ }

bool BusyBaseProjectNode::showInSimpleTree() const
{
    return false;
}

QList<ProjectExplorer::ProjectAction> BusyBaseProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::ProjectAction>();
}

// --------------------------------------------------------------------
// BusyGroupNode:
// --------------------------------------------------------------------

void BusyGroupNode::setupFiles(ProjectExplorer::FolderNode *root, const busy::Product & product,
        const QStringList &files, const QString &productPath, bool updateExisting)
{
    // Build up a tree of nodes:
    FileTreeNode tree;

    foreach (const QString &path, files) {
        QStringList pathSegments = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

        FileTreeNode *root = &tree;
        while (!pathSegments.isEmpty()) {
            bool isFile = pathSegments.count() == 1;
            root = root->addPart(pathSegments.takeFirst(), isFile);
        }
    }

    FileTreeNode::reorder(&tree, productPath);
    FileTreeNode::simplify(&tree);

    setupFolder(root, product, &tree, productPath, updateExisting);
}

void BusyGroupNode::setupFolder(ProjectExplorer::FolderNode *root, const busy::Product &product,
                               const FileTreeNode *fileTree, const QString &baseDir,
                               bool updateExisting)
{
    // We only need to care about FileNodes and FolderNodes here. Everything else is
    // handled elsewhere.
    // BusyGroupNodes are managed by the BusyProductNode.
    // The buildsystem file is either managed by BusyProductNode or by updateBusyGroupData(...).

    QList<ProjectExplorer::FileNode *> filesToRemove;
    foreach (ProjectExplorer::FileNode *fn, root->fileNodes()) {
        if (!dynamic_cast<BusyFileNode *>(fn))
            filesToRemove << fn;
    }
    QList<ProjectExplorer::FileNode *> filesToAdd;

    QList<ProjectExplorer::FolderNode *> foldersToRemove;
    foreach (ProjectExplorer::FolderNode *fn, root->subFolderNodes()) {
        if (fn->nodeType() == ProjectExplorer::ProjectNodeType)
            continue; // Skip ProjectNodes mixed into the folders...
        foldersToRemove.append(fn);
    }

    foreach (FileTreeNode *c, fileTree->children) {
        Utils::FileName path = Utils::FileName::fromString(c->path());
        const ProjectExplorer::FileType newFileType = fileType(product, *c);
        const bool isQrcFile = newFileType == ProjectExplorer::ResourceType;

        // Handle files:
        if (c->isFile() && !isQrcFile) {
            ProjectExplorer::FileNode *fn = 0;
            foreach (ProjectExplorer::FileNode *f, root->fileNodes()) {
                // There can be one match only here!
                if (f->path() != path || f->fileType() != newFileType)
                    continue;
                fn = f;
                break;
            }
            if (fn) {
                filesToRemove.removeOne(fn);
                if (updateExisting)
                    fn->emitNodeUpdated();
            } else {
                fn = new ProjectExplorer::FileNode(path, newFileType, false);
                filesToAdd.append(fn);
            }
            continue;
        } else {
            ProjectExplorer::FolderNode *fn = 0;
            foreach (ProjectExplorer::FolderNode *f, root->subFolderNodes()) {
                // There can be one match only here!
                if (f->path() != path)
                    continue;
                fn = f;
                break;
            }
            using ResourceEditor::ResourceTopLevelNode;
            if (!fn) {
                if (isQrcFile) {
                    fn = new ResourceTopLevelNode(Utils::FileName::fromString(c->path()), root);
                } else {
                    fn = new BusyFolderNode(Utils::FileName::fromString(c->path()),
                                           ProjectExplorer::FolderNodeType,
                                           displayNameFromPath(c->path(), baseDir));
                }
                root->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << fn);
            } else {
                foldersToRemove.removeOne(fn);
                if (updateExisting)
                    fn->emitNodeUpdated();
                fn->setDisplayName(displayNameFromPath(c->path(), baseDir));
            }

            if (isQrcFile)
                static_cast<ResourceTopLevelNode *>(fn)->update();
            else
                setupFolder(fn, product, c, c->path(), updateExisting);

        }
    }
    root->removeFileNodes(filesToRemove);
    root->removeFolderNodes(foldersToRemove);
    root->addFileNodes(filesToAdd);
}

ProjectExplorer::FileType BusyGroupNode::fileType(const busy::Product &product,
                                                 const FileTreeNode &fileNode)
{
    if (!product.isValid())
        return ProjectExplorer::UnknownFileType;
    const QByteArray suffix = QFileInfo(fileNode.path()).suffix().toLower().toUtf8();
    const int lang = bs_guessLang(suffix.constData());

    if( lang == BS_header )
        return ProjectExplorer::HeaderType;
    else if( lang == BS_unknownLang )
    {
        if( suffix == "qrc" )
            return ProjectExplorer::ResourceType;
        else if( suffix == "ui" )
            return ProjectExplorer::FormType;
        else
            return ProjectExplorer::UnknownFileType;
    }else
        return ProjectExplorer::SourceType;
}

// --------------------------------------------------------------------
// BusyProductNode:
// --------------------------------------------------------------------

BusyProductNode::BusyProductNode(const busy::Project& project, const busy::Product &prd) :
    BusyBaseProjectNode(Utils::FileName::fromString(prd.location().filePath()))
{
    if (m_productIcon.isNull())
        m_productIcon = generateIcon(QString::fromLatin1(Constants::BUSY_PRODUCT_OVERLAY_ICON));

    setIcon(m_productIcon);

    auto idx = new BusyFileNode(Utils::FileName::fromString(prd.location().filePath()),
                               ProjectExplorer::ProjectFileType, false,
                               prd.location().line());
    addFileNodes(QList<ProjectExplorer::FileNode *>() << idx);

    setBusyProductData(project, prd);
}

bool BusyProductNode::isEnabled() const
{
    return m_qbsProductData.isEnabled();
}

bool BusyProductNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectExplorer::ProjectAction> BusyProductNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true); // too restricitve: m_qbsProductData.isCompiled()
}

bool BusyProductNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    BusyProjectNode *prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    return prjNode->project()->addFilesToProduct(this, filePaths, m_qbsProductData, notAdded);
}

bool BusyProductNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    BusyProjectNode *prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    return prjNode->project()->removeFilesFromProduct(this, filePaths, m_qbsProductData, notRemoved);
 }

bool BusyProductNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    BusyProjectNode * const prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid())
        return false;
    return prjNode->project()->renameFileInProduct(this, filePath, newFilePath, m_qbsProductData);
}

void BusyProductNode::setBusyProductData(const busy::Project& project, const busy::Product prd)
{
    //if (m_qbsProductData == prd)
    //    return;

    bool productWasEnabled = m_qbsProductData.isValid() && m_qbsProductData.isEnabled();
    bool productIsEnabled = prd.isEnabled();
    bool updateExisting = productWasEnabled != productIsEnabled;

    QString name = prd.name();
#if 0
    const QString alt = prd.name(true);
    if( name != alt )
        name = QString("%1 (%2)").arg(name).arg(alt);
#endif
    setDisplayName(name);
    busy::CodeLocation loc = prd.location();
    setPath(Utils::FileName::fromString(loc.filePath()));
    const QString productPath = QFileInfo(loc.filePath()).absolutePath();

    // Find the BusyFileNode we added earlier:
    BusyFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = dynamic_cast<BusyFileNode *>(fn);
        if (idx)
            break;
    }
    QTC_ASSERT(idx, return);
    idx->setPathAndLine(Utils::FileName::fromString(loc.filePath()), loc.line());

    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    BusyGroupNode::setupFiles(this, prd, prd.allFilePaths(prd.isCompiled()), productPath, updateExisting);

    addProjectNodes(toAdd);
    removeProjectNodes(toRemove);

    m_qbsProductData = prd;
    if (updateExisting)
        emitNodeUpdated();
}

QList<ProjectExplorer::RunConfiguration *> BusyProductNode::runConfigurations() const
{
    QList<ProjectExplorer::RunConfiguration *> result;
    BusyProjectNode *pn = dynamic_cast<BusyProjectNode *>(projectNode());
    if (!isEnabled() || !pn || !pn->busyProject().isValid()
            || pn->busyProject().targetExecutable(m_qbsProductData, busy::InstallOptions()).isEmpty()) {
        return result;
    }

    foreach (ProjectExplorer::RunConfiguration *rc, pn->project()->activeTarget()->runConfigurations()) {
        BusyRunConfiguration *qbsRc = qobject_cast<BusyRunConfiguration *>(rc);
        if (!qbsRc)
            continue;
        if (qbsRc->uniqueProductName() == BusyProject::uniqueProductName(busyProductData()))
            result << qbsRc;
    }

    return result;
}

#if 0
BusyGroupNode *BusyProductNode::findGroupNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        BusyGroupNode *qn = static_cast<BusyGroupNode *>(n);
        if (qn->busyGroupData().name() == name)
            return qn;
    }
    return 0;
}
#endif

// --------------------------------------------------------------------
// BusyProjectNode:
// --------------------------------------------------------------------

BusyProjectNode::BusyProjectNode(const Utils::FileName &path) :
    BusyBaseProjectNode(path)
{
    ctor();
}

BusyProjectNode::~BusyProjectNode()
{
    // do not delete m_project
}

void BusyProjectNode::update(const busy::Project& qbsProject, const busy::Module &module)
{
    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    foreach (const busy::Module &subData, module.subModules()) {
        BusyProjectNode *qn = findProjectNode(subData.name());
        if (!qn) {
            auto subProject =
                    new BusyProjectNode(Utils::FileName::fromString(subData.busyFile()));
            subProject->update(qbsProject, subData);
            toAdd << subProject;
        } else {
            qn->update(qbsProject, subData);
            toRemove.removeOne(qn);
        }
    }

    foreach (const busy::Product &prd, module.products()) {
        BusyProductNode *qn = findProductNode(BusyProject::uniqueProductName(prd));
        if (!qn) {
            toAdd << new BusyProductNode(qbsProject, prd);
        } else {
            qn->setBusyProductData(qbsProject, prd);
            toRemove.removeOne(qn);
        }
    }

    if (!module.name().isEmpty())
        setDisplayName(module.name());
    else
    {
        BusyProject* p = project();
        if(p)
            setDisplayName(p->displayName());
        else
            setDisplayName("<unknown>");
    }

    removeProjectNodes(toRemove);
    addProjectNodes(toAdd);
    m_project = qbsProject;
    m_module = module;
}

BusyProject *BusyProjectNode::project() const
{
    FolderNode* f = parentFolderNode();
    if(f)
        return static_cast<BusyProjectNode *>(f)->project();
    else
        return 0;
}

const busy::Project BusyProjectNode::busyProject() const
{
    return m_project; // project()->busyProject();
}

bool BusyProjectNode::showInSimpleTree() const
{
    return true;
}

void BusyProjectNode::ctor()
{
#if 0
    // TODO
    if (m_projectIcon.isNull())
        m_projectIcon = generateIcon(QString::fromLatin1(QtSupport::Constants::ICON_QT_PROJECT));
#endif

    setIcon(m_projectIcon);
    addFileNodes(QList<ProjectExplorer::FileNode *>()
                 << new ProjectExplorer::FileNode(path(), ProjectExplorer::ProjectFileType, false));
}

BusyProductNode *BusyProjectNode::findProductNode(const QString &uniqueName)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        BusyProductNode *qn = dynamic_cast<BusyProductNode *>(n);
        if (qn && BusyProject::uniqueProductName(qn->busyProductData()) == uniqueName)
            return qn;
    }
    return 0;
}

BusyProjectNode *BusyProjectNode::findProjectNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        BusyProjectNode *qn = dynamic_cast<BusyProjectNode *>(n);
        if (qn && qn->busyModule().name() == name)
            return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// BusyRootProjectNode:
// --------------------------------------------------------------------

BusyRootProjectNode::BusyRootProjectNode(BusyProject *project) :
    BusyProjectNode(project->projectFilePath()),
    m_project(project)
  #if 0
  ,
    m_buildSystemFiles(new ProjectExplorer::FolderNode(project->projectDirectory(),
                                                       ProjectExplorer::FolderNodeType,
                                                       QCoreApplication::translate(
                                                           "BusyRootProjectNode", "Busy files")))
  #endif
{
    // addFolderNodes(QList<FolderNode *>() << m_buildSystemFiles);
}

void BusyRootProjectNode::update()
{
#if 0
    QStringList buildSystemFiles = unreferencedBuildSystemFiles(m_project->busyProject());

    QStringList projectBuildSystemFiles;
    Utils::FileName base = m_project->projectDirectory();
    foreach (const QString &f, buildSystemFiles) {
        if (Utils::FileName::fromString(f).isChildOf(base))
                projectBuildSystemFiles.append(f);
    }
    BusyGroupNode::setupFiles(m_buildSystemFiles, busy::Product(), projectBuildSystemFiles, base.toString(), false);
#endif
    update(m_project->busyProject(), m_project->busyModule());
}

static QSet<QString> referencedBuildSystemFiles(const busy::Module &data)
{
    QSet<QString> result;
    result.insert(data.location().filePath());
    foreach (const busy::Module &subProject, data.subModules())
        result.unite(referencedBuildSystemFiles(subProject));
    foreach (const busy::Product &product, data.products()) {
        result.insert(product.location().filePath());
    }

    return result;
}

QStringList BusyRootProjectNode::unreferencedBuildSystemFiles(const busy::Project &p) const
{
    return p.buildSystemFiles().subtract(referencedBuildSystemFiles(p.topModule())).toList();
}

} // namespace Internal
} // namespace BusyProjectManager
