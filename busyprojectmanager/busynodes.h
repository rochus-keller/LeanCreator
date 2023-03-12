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

#ifndef BUSYNODES_H
#define BUSYNODES_H

#include <projectexplorer/projectnodes.h>

#include <busytools/busyapi.h>

#include <QIcon>

namespace BusyProjectManager {
namespace Internal {

class FileTreeNode;
class BusyProject;
class BusyProjectFile;

// ----------------------------------------------------------------------
// BusyFileNode:
// ----------------------------------------------------------------------

class BusyFileNode : public ProjectExplorer::FileNode
{
public:
    BusyFileNode(const Utils::FileName &filePath, const ProjectExplorer::FileType fileType, bool generated,
                int line);

    QString displayName() const;
};

class BusyFolderNode : public ProjectExplorer::FolderNode
{
public:
    BusyFolderNode(const Utils::FileName &folderPath, ProjectExplorer::NodeType nodeType,
                  const QString &displayName);

private:
    QList<ProjectExplorer::ProjectAction> supportedActions(ProjectExplorer::Node *node) const;
};

// ---------------------------------------------------------------------------
// BusyBaseProjectNode:
// ---------------------------------------------------------------------------

class BusyGroupNode;

class BusyBaseProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit BusyBaseProjectNode(const Utils::FileName &path);

    bool showInSimpleTree() const;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;
private:
    friend class BusyGroupNode;
};

// --------------------------------------------------------------------
// BusyGroupNode:
// --------------------------------------------------------------------

class BusyGroupNode
{
public:
    static void setupFiles(ProjectExplorer::FolderNode *root, const busy::Product & product, const QStringList &files,
                           const QString &productPath, bool updateExisting);
private:
    static void setupFolder(ProjectExplorer::FolderNode *folder, const busy::Product & product,
            const FileTreeNode *subFileTree, const QString &baseDir, bool updateExisting);
    static ProjectExplorer::FileType fileType(const busy::Product &product,
                                              const FileTreeNode &fileNode);
};

// --------------------------------------------------------------------
// BusyProductNode:
// --------------------------------------------------------------------

class BusyProductNode : public BusyBaseProjectNode
{
public:
    explicit BusyProductNode(const busy::Project &project, const busy::Product &prd);

    bool isEnabled() const;
    bool showInSimpleTree() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    void setBusyProductData(const busy::Project &project, const busy::Product prd);
    const busy::Product busyProductData() const { return m_qbsProductData; }

    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const;

private:
    //BusyGroupNode *findGroupNode(const QString &name);

    busy::Product m_qbsProductData;
    static QIcon m_productIcon;
};

// ---------------------------------------------------------------------------
// BusyProjectNode:
// ---------------------------------------------------------------------------

class BusyProjectNode : public BusyBaseProjectNode
{
public:
    explicit BusyProjectNode(const Utils::FileName &path);
    ~BusyProjectNode();

    virtual BusyProject *project() const;
    const busy::Project busyProject() const;
    const busy::Module busyModule() const { return m_module; }

    bool showInSimpleTree() const;

protected:
    void update(const busy::Project &busyProject, const busy::Module &module);

private:
    void ctor();

    BusyProductNode *findProductNode(const QString &uniqueName);
    BusyProjectNode *findProjectNode(const QString &name);

    static QIcon m_projectIcon;
    busy::Module m_module;
    busy::Project m_project;
};

// --------------------------------------------------------------------
// BusyRootProjectNode:
// --------------------------------------------------------------------

class BusyRootProjectNode : public BusyProjectNode
{
public:
    explicit BusyRootProjectNode(BusyProject *project);

    using BusyProjectNode::update;
    void update();

    BusyProject *project() const { return m_project; }

private:
    QStringList unreferencedBuildSystemFiles(const busy::Project& p) const;

    BusyProject * const m_project;
#if 0
    ProjectExplorer::FolderNode *m_buildSystemFiles;
#endif
};


} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYNODES_H
