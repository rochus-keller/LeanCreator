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

#ifndef PROJECTFILEWIZARDEXTENSION2_H
#define PROJECTFILEWIZARDEXTENSION2_H

#include "projectexplorer_export.h"

#include <core/ifilewizardextension.h>

namespace ProjectExplorer {
class FolderNode;

namespace Internal {

class ProjectWizardContext;

class PROJECTEXPLORER_EXPORT ProjectFileWizardExtension : public Core::IFileWizardExtension
{
    Q_OBJECT
public:
    explicit ProjectFileWizardExtension();
    ~ProjectFileWizardExtension();

    QList<QWizardPage *> extensionPages(const Core::IWizardFactory *wizard);
    bool processFiles(const QList<Core::GeneratedFile> &files,
                 bool *removeOpenProjectAttribute, QString *errorMessage);
    void applyCodeStyle(Core::GeneratedFile *file) const;

public slots:
    void firstExtensionPageShown(const QList<Core::GeneratedFile> &files, const QVariantMap &extraValues);

private:
    bool processProject(const QList<Core::GeneratedFile> &files,
                        bool *removeOpenProjectAttribute, QString *errorMessage);

    ProjectWizardContext *m_context;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTFILEWIZARDEXTENSION2_H
