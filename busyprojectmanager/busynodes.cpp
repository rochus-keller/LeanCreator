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
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
//#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

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

QIcon BusyGroupNode::m_groupIcon;
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

static busy::GroupData findMainBusyGroup(const busy::ProductData &productData)
{
    foreach (const busy::GroupData &grp, productData.groups()) {
        if (grp.name() == productData.name() && grp.location() == productData.location())
            return grp;
    }
    return busy::GroupData();
}

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
        actions << ProjectExplorer::AddNewFile << ProjectExplorer::AddExistingFile;
    if (node->nodeType() == ProjectExplorer::FileNodeType
            && !project->busyProject().buildSystemFiles().contains(node->path().toString())) {
        actions << ProjectExplorer::RemoveFile << ProjectExplorer::Rename;
    }
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

BusyGroupNode::BusyGroupNode(const busy::GroupData &grp, const QString &productPath) :
    BusyBaseProjectNode(Utils::FileName())
{
    if (m_groupIcon.isNull())
        m_groupIcon = QIcon(QString::fromLatin1(Constants::BUSY_GROUP_ICON));

    setIcon(m_groupIcon);

    BusyFileNode *idx = new BusyFileNode(Utils::FileName::fromString(grp.location().filePath()),
                                       ProjectExplorer::ProjectFileType, false,
                                       grp.location().line());
    addFileNodes(QList<ProjectExplorer::FileNode *>() << idx);

    updateBusyGroupData(grp, productPath, true, true);
}

bool BusyGroupNode::isEnabled() const
{
    if (!parentFolderNode() || !m_qbsGroupData.isValid())
        return false;
    return static_cast<BusyProductNode *>(parentFolderNode())->isEnabled()
            && m_qbsGroupData.isEnabled();
}

QList<ProjectExplorer::ProjectAction> BusyGroupNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true);
}

bool BusyGroupNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    BusyProjectNode *prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    BusyProductNode *prdNode = parentBusyProductNode(this);
    if (!prdNode || !prdNode->busyProductData().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    return prjNode->project()->addFilesToProduct(this, filePaths, prdNode->busyProductData(),
                                                 m_qbsGroupData, notAdded);
}

bool BusyGroupNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    BusyProjectNode *prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    BusyProductNode *prdNode = parentBusyProductNode(this);
    if (!prdNode || !prdNode->busyProductData().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    return prjNode->project()->removeFilesFromProduct(this, filePaths, prdNode->busyProductData(),
                                                      m_qbsGroupData, notRemoved);
}

bool BusyGroupNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    BusyProjectNode * const prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid())
        return false;
    BusyProductNode * const prdNode = parentBusyProductNode(this);
    if (!prdNode || !prdNode->busyProductData().isValid())
        return false;

    return prjNode->project()->renameFileInProduct(this, filePath, newFilePath,
                                                   prdNode->busyProductData(), m_qbsGroupData);
}

void BusyGroupNode::updateBusyGroupData(const busy::GroupData &grp, const QString &productPath,
                                      bool productWasEnabled, bool productIsEnabled)
{
    QTC_ASSERT(grp.isValid(), return);

    if (grp == m_qbsGroupData && productPath == m_productPath)
        return;

    bool groupWasEnabled = productWasEnabled && m_qbsGroupData.isValid()
            && m_qbsGroupData.isEnabled();
    bool groupIsEnabled = productIsEnabled && grp.isEnabled();
    bool updateExisting = groupWasEnabled != groupIsEnabled;

    m_productPath = productPath;
    m_qbsGroupData = grp;

    setPath(Utils::FileName::fromString(grp.location().filePath()));
    setDisplayName(grp.name());

    BusyFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = dynamic_cast<BusyFileNode *>(fn);
        if (idx)
            break;
    }
    QTC_ASSERT(idx, return);
    idx->setPathAndLine(Utils::FileName::fromString(grp.location().filePath()),
                        grp.location().line());

    setupFiles(this, grp, grp.allFilePaths(), productPath, updateExisting);

    if (updateExisting)
        emitNodeUpdated();
}

void BusyGroupNode::setupFiles(ProjectExplorer::FolderNode *root, const busy::GroupData &group,
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

    setupFolder(root, group, &tree, productPath, updateExisting);
}

void BusyGroupNode::setupFolder(ProjectExplorer::FolderNode *root, const busy::GroupData &group,
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
        const ProjectExplorer::FileType newFileType = fileType(group, *c);
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
                root->addFolderNodes(QList<FolderNode *>() << fn);
            } else {
                foldersToRemove.removeOne(fn);
                if (updateExisting)
                    fn->emitNodeUpdated();
                fn->setDisplayName(displayNameFromPath(c->path(), baseDir));
            }

            if (isQrcFile)
                static_cast<ResourceTopLevelNode *>(fn)->update();
            else
                setupFolder(fn, group, c, c->path(), updateExisting);

        }
    }
    root->removeFileNodes(filesToRemove);
    root->removeFolderNodes(foldersToRemove);
    root->addFileNodes(filesToAdd);
}

ProjectExplorer::FileType BusyGroupNode::fileType(const busy::GroupData &group,
                                                 const FileTreeNode &fileNode)
{
    if (!group.isValid())
        return ProjectExplorer::UnknownFileType;
    const busy::SourceArtifact artifact = Utils::findOrDefault(group.allSourceArtifacts(),
            [&fileNode](const busy::SourceArtifact &sa) { return sa.filePath() == fileNode.path(); });
    QTC_ASSERT(artifact.isValid() || !fileNode.isFile(),
               qDebug() << fileNode.path() << group.name(); return ProjectExplorer::UnknownFileType);
    if (!artifact.isValid())
        return ProjectExplorer::UnknownFileType;

    if (artifact.fileTags().contains(QLatin1String("c"))
            || artifact.fileTags().contains(QLatin1String("cpp"))
            || artifact.fileTags().contains(QLatin1String("objc"))
            || artifact.fileTags().contains(QLatin1String("objcpp"))) {
        return ProjectExplorer::SourceType;
    }
    if (artifact.fileTags().contains(QLatin1String("hpp")))
        return ProjectExplorer::HeaderType;
    if (artifact.fileTags().contains(QLatin1String("qrc")))
        return ProjectExplorer::ResourceType;
    if (artifact.fileTags().contains(QLatin1String("ui")))
        return ProjectExplorer::FormType;
    return ProjectExplorer::UnknownFileType;
}

// --------------------------------------------------------------------
// BusyProductNode:
// --------------------------------------------------------------------

BusyProductNode::BusyProductNode(const busy::Module &project, const busy::ProductData &prd) :
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
    return supportedNodeActions(node, true);
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

    busy::GroupData grp = findMainBusyGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->addFilesToProduct(this, filePaths, m_qbsProductData, grp,
                                                     notAdded);
    }

    QTC_ASSERT(false, return false);
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

    busy::GroupData grp = findMainBusyGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->removeFilesFromProduct(this, filePaths, m_qbsProductData, grp,
                                                          notRemoved);
    }

    QTC_ASSERT(false, return false);
}

bool BusyProductNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    BusyProjectNode * const prjNode = parentBusyProjectNode(this);
    if (!prjNode || !prjNode->busyProject().isValid())
        return false;
    const busy::GroupData grp = findMainBusyGroup(m_qbsProductData);
    QTC_ASSERT(grp.isValid(), return false);
    return prjNode->project()->renameFileInProduct(this, filePath, newFilePath, m_qbsProductData,
                                                   grp);
}

void BusyProductNode::setBusyProductData(const busy::Module &project, const busy::ProductData prd)
{
    if (m_qbsProductData == prd)
        return;

    bool productWasEnabled = m_qbsProductData.isValid() && m_qbsProductData.isEnabled();
    bool productIsEnabled = prd.isEnabled();
    bool updateExisting = productWasEnabled != productIsEnabled;

    setDisplayName(BusyProject::productDisplayName(project, prd));
    setPath(Utils::FileName::fromString(prd.location().filePath()));
    const QString &productPath = QFileInfo(prd.location().filePath()).absolutePath();

    // Find the BusyFileNode we added earlier:
    BusyFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = dynamic_cast<BusyFileNode *>(fn);
        if (idx)
            break;
    }
    QTC_ASSERT(idx, return);
    idx->setPathAndLine(Utils::FileName::fromString(prd.location().filePath()),
                        prd.location().line());

    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    foreach (const busy::GroupData &grp, prd.groups()) {
        if (grp.name() == prd.name() && grp.location() == prd.location()) {
            // Set implicit product group right onto this node:
            BusyGroupNode::setupFiles(this, grp, grp.allFilePaths(), productPath, updateExisting);
            continue;
        }
        BusyGroupNode *gn = findGroupNode(grp.name());
        if (gn) {
            toRemove.removeOne(gn);
            gn->updateBusyGroupData(grp, productPath, productWasEnabled, productIsEnabled);
        } else {
            gn = new BusyGroupNode(grp, productPath);
            toAdd.append(gn);
        }
    }

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

BusyGroupNode *BusyProductNode::findGroupNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        BusyGroupNode *qn = static_cast<BusyGroupNode *>(n);
        if (qn->busyGroupData().name() == name)
            return qn;
    }
    return 0;
}

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

void BusyProjectNode::update(const busy::Module &qbsProject, const busy::ProjectData &prjData)
{
    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    foreach (const busy::ProjectData &subData, prjData.subProjects()) {
        BusyProjectNode *qn = findProjectNode(subData.name());
        if (!qn) {
            auto subProject =
                    new BusyProjectNode(Utils::FileName::fromString(subData.location().filePath()));
            subProject->update(qbsProject, subData);
            toAdd << subProject;
        } else {
            qn->update(qbsProject, subData);
            toRemove.removeOne(qn);
        }
    }

    foreach (const busy::ProductData &prd, prjData.products()) {
        BusyProductNode *qn = findProductNode(BusyProject::uniqueProductName(prd));
        if (!qn) {
            toAdd << new BusyProductNode(qbsProject, prd);
        } else {
            qn->setBusyProductData(qbsProject, prd);
            toRemove.removeOne(qn);
        }
    }

    if (!prjData.name().isEmpty())
        setDisplayName(prjData.name());
    else
        setDisplayName(project()->displayName());

    removeProjectNodes(toRemove);
    addProjectNodes(toAdd);
    m_projectData = prjData;
}

BusyProject *BusyProjectNode::project() const
{
    return static_cast<BusyProjectNode *>(parentFolderNode())->project();
}

const busy::Module BusyProjectNode::busyProject() const
{
    return project()->busyProject();
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
        if (qn && qn->busyProjectData().name() == name)
            return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// BusyRootProjectNode:
// --------------------------------------------------------------------

BusyRootProjectNode::BusyRootProjectNode(BusyProject *project) :
    BusyProjectNode(project->projectFilePath()),
    m_project(project),
    m_buildSystemFiles(new ProjectExplorer::FolderNode(project->projectDirectory(),
                                                       ProjectExplorer::FolderNodeType,
                                                       QCoreApplication::translate("BusyRootProjectNode", "Busy files")))
{
    addFolderNodes(QList<FolderNode *>() << m_buildSystemFiles);
}

void BusyRootProjectNode::update()
{
    QStringList buildSystemFiles = unreferencedBuildSystemFiles(m_project->busyProject());

    QStringList projectBuildSystemFiles;
    Utils::FileName base = m_project->projectDirectory();
    foreach (const QString &f, buildSystemFiles) {
        if (Utils::FileName::fromString(f).isChildOf(base))
                projectBuildSystemFiles.append(f);
    }
    BusyGroupNode::setupFiles(m_buildSystemFiles, busy::GroupData(), projectBuildSystemFiles,
                             base.toString(), false);

    update(m_project->busyProject(), m_project->busyProjectData());
}

static QSet<QString> referencedBuildSystemFiles(const busy::ProjectData &data)
{
    QSet<QString> result;
    result.insert(data.location().filePath());
    foreach (const busy::ProjectData &subProject, data.subProjects())
        result.unite(referencedBuildSystemFiles(subProject));
    foreach (const busy::ProductData &product, data.products()) {
        result.insert(product.location().filePath());
        foreach (const busy::GroupData &group, product.groups())
            result.insert(group.location().filePath());
    }

    return result;
}

QStringList BusyRootProjectNode::unreferencedBuildSystemFiles(const busy::Module &p) const
{
    return p.buildSystemFiles().subtract(referencedBuildSystemFiles(p.projectData())).toList();
}

} // namespace Internal
} // namespace BusyProjectManager
