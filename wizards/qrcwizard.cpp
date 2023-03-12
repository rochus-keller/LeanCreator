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

#include "qrcwizard.h"
#include <utils/filewizardpage.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <QTextStream>
#include <QFile>
using namespace Wizards;


QrcWizardFactory::QrcWizardFactory()
{
    setId("QrcWizardFactory");
    setWizardKind(FileWizard);
    setDisplayName("Qt Resource File");
    setDescription("Creates a Qt Resource file (.qrc).");
    setCategory("R.Qt");
    setDisplayCategory("Qt");
    setFlags( PlatformIndependent );
}

Core::BaseFileWizard * QrcWizardFactory::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    QrcWizardDialog* dlg = new QrcWizardDialog(this,parent);
    dlg->d_where = dynamic_cast<ProjectExplorer::FolderNode*>(parameters.extraValues().value("ProjectExplorer.PreferredProjectNode").
            value<ProjectExplorer::Node*>());
    dlg->m_firstPage->setPath( parameters.defaultPath() );

    return dlg;
}

Core::GeneratedFiles QrcWizardFactory::generateFiles(const QWizard *w, QString *errorMessage) const
{
    QrcWizardDialog* dlg = (QrcWizardDialog*)w;

    Core::GeneratedFiles res;

    QString path = QString("%1/%2").arg(dlg->m_firstPage->path()).arg(dlg->m_firstPage->fileName());
    if( !path.endsWith(".qrc") )
        path += ".qrc";

    Core::GeneratedFile file( path );
    QString tmp1;
    QTextStream out(&tmp1);

    out << "<RCC>" << endl;
    out << "    <qresource>" << endl;
    out << "    </qresource>" << endl;
    out << "</RCC>" << endl;

    file.setContents(tmp1);
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    res << file;
    return res;
}

bool QrcWizardFactory::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    QrcWizardDialog* dlg = (QrcWizardDialog*)w;
    QStringList paths;
    if( !l.isEmpty() )
        paths << l.first().path();
    if( dlg->d_where )
    {
        QStringList done;
        if( !dlg->d_where->addFiles(paths, &done) )
        {
            *errorMessage = "cannot add file to project node";
            foreach( const Core::GeneratedFile& f, l )
                QFile::remove(f.path());
            return false;
        }
    }
    return BaseFileWizardFactory::postGenerateOpenEditors(l, errorMessage);
}

QrcWizardDialog::QrcWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent)
: Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    setPixmap(QWizard::LogoPixmap, QPixmap(":/wizards/images/genericfilewizard.png"));

    m_firstPage = new Utils::FileWizardPage(parent);
    m_firstPage->setTitle(tr("Location"));
    m_firstPage->setFileNameLabel(tr("Name:"));
    m_firstPage->setPathLabel(tr("Location:"));

    addPage(m_firstPage);
}
