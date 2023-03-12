/****************************************************************************
**
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#include "busyprowizard.h"
#include <projectexplorer/projectexplorer.h>
#include <utils/filewizardpage.h>
#include <cpptools/cppfilesettingspage.h>
#include <core/icore.h>
#include <QTextStream>
#include <QFile>
#include <QDir>
using namespace Wizards;


BusyProWizardDialog::BusyProWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent)
    :Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    m_firstPage = new Utils::FileWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));
    m_firstPage->setFileNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));
    addPage(m_firstPage);

}

BusyProWizardFactory::BusyProWizardFactory()
{
    setId("BusyProWizardFactory");
    setWizardKind(ProjectWizard);
    setDisplayName("BUSY Hello World");
    setDescription("Creates a project containing a single main.cpp file with a stub implementation.");
    setCategory("R.BUSY.A");
    setDisplayCategory("BUSY Applications");
    setFlags( PlatformIndependent );
}

Core::BaseFileWizard *BusyProWizardFactory::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    BusyProWizardDialog* dlg = new BusyProWizardDialog(this,parent);
    dlg->m_firstPage->setPath( parameters.defaultPath() );

    return dlg;
}

Core::GeneratedFiles BusyProWizardFactory::generateFiles(const QWizard *w, QString *errorMessage) const
{
    BusyProWizardDialog* dlg = (BusyProWizardDialog*)w;

    Core::GeneratedFiles res;

    CppTools::Internal::CppFileSettings set;
    set.fromSettings(Core::ICore::settings());

    const QString projectPath = QString("%1/%2").arg(dlg->m_firstPage->path()).arg(dlg->m_firstPage->fileName());
    const QDir dir(projectPath);


    const QString license = CppTools::Internal::CppFileSettings::licenseTemplate();

    Core::GeneratedFile busy( dir.absoluteFilePath("BUSY") );

    QString text =
            "let config : Config {\n"
            "    if (target_os == `linux) || (target_os == `macos) {\n"
            "        .lib_names += [ \"m\" \"stdc++\" ]\n"
            "        .ldflags += \"-shared-libgcc\"\n"
            "    }else if target_os == `win32 {\n"
            "        .lib_names = [ \"Gdi32\" \"User32\" \"Shell32\" \"Comdlg32\" ]\n"
            "    }else {\n"
            "        error(\"target os not supported\")\n"
            "    }\n"
            "}\n\n";

    text +=
           "let hello ! : Executable {\n"
           "    .sources += ./main.cpp\n"
           "    .configs += config\n"
           "}\n";


    busy.setContents(text);
    busy.setBinary(true); // otherwise all leading white space is deleted!
    busy.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFile body( dir.absoluteFilePath(QString("main.%1").arg(set.sourceSuffix) ) );

    QString tmp2;
    QTextStream bout(&tmp2);

    if( !license.isEmpty() )
        bout << license << endl << endl;

    bout << "#include <iostream>" << endl << endl;

    bout << "int main(int argc, char *argv[])" << endl;
    bout << "{" << endl;
    bout << "    std::cout << \"Hello World!\" << std::endl;" << endl;
    bout << "    return 0;" << endl;
    bout << "}" << endl;

    body.setContents(tmp2);
    body.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    res << busy << body;
    return res;
}

bool BusyProWizardFactory::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    foreach (const Core::GeneratedFile &file, l) {
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            ProjectExplorer::ProjectExplorerPlugin::OpenProjectResult result
                    = ProjectExplorer::ProjectExplorerPlugin::openProject(file.path());
            if (!result) {
                return false;
                *errorMessage = result.errorMessage();
            }
        }
    }
    return BaseFileWizardFactory::postGenerateOpenEditors(l, errorMessage);
}
