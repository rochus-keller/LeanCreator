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

#include "classwizard.h"
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <core/icore.h>
#include <cpptools/cppfilesettingspage.h>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextStream>
#include <QFile>

Wizards::ClassWizardFactory::ClassWizardFactory()
{
    setId("ClassWizardFactory");
    setWizardKind(FileWizard);
    setDisplayName("C++ Class");
    setDescription("Creates a C++ header and a source file for a new class that you can add to a C++ project.");
    setCategory("O.C++");
    setDisplayCategory("C++");
    setFlags( PlatformIndependent );
}

Core::BaseFileWizard *Wizards::ClassWizardFactory::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    ClassWizardDialog* dlg = new ClassWizardDialog(this,parent);
    dlg->d_path = parameters.defaultPath();
    dlg->d_where = dynamic_cast<ProjectExplorer::FolderNode*>(parameters.extraValues().value("ProjectExplorer.PreferredProjectNode").
            value<ProjectExplorer::Node*>());
    return dlg;
}

Core::GeneratedFiles Wizards::ClassWizardFactory::generateFiles(const QWizard *w, QString *errorMessage) const
{
    ClassWizardDialog* dlg = (ClassWizardDialog*)w;

    Core::GeneratedFiles res;
    CppTools::Internal::CppFileSettings set;
    set.fromSettings(Core::ICore::settings());
    const bool LowerCaseFiles = set.lowerCaseFiles;
    const QString hsuff = set.headerSuffix;
    const QString bsuff = set.sourceSuffix;

    const QString name = dlg->m_firstPage->d_name->text();
    const QString fileName = LowerCaseFiles ? name.toLower() : name;
    const QString ns = dlg->m_firstPage->d_ns->text();
    const QString guard = QString("%1_%2_H").arg( !ns.isEmpty() ? ns.toUpper() : QString() ).arg(name.toUpper());

    const QString license = CppTools::Internal::CppFileSettings::licenseTemplate();

    Core::GeneratedFile header( QString("%1/%2.%3").arg(dlg->d_path).arg(fileName).arg(hsuff) );
    QString tmp1;
    QTextStream hout(&tmp1);

    hout << "#ifndef " << guard << endl;
    hout << "#define " << guard << endl << endl;

    if( !license.isEmpty() )
        hout << license << endl << endl;

    if( !ns.isEmpty() )
        hout << "namespace " << ns << " {" << endl << endl;

    hout << "class " << name << " {" << endl;

    hout << "public:" << endl;

    hout << "    " << name << "();" << endl;
    hout << "     ~" << name << "();" << endl;

    hout << "};" << endl << endl;

    if( !ns.isEmpty() )
        hout << "}" << endl << endl;

    hout << "#endif // " << guard << endl;

    header.setContents(tmp1);
    header.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    Core::GeneratedFile body( QString("%1/%2.%3").arg(dlg->d_path).arg(fileName).arg(bsuff) );

    QString tmp2;
    QTextStream bout(&tmp2);

    if( !license.isEmpty() )
        bout << license << endl << endl;

    bout << "#include \"" << fileName << "." << hsuff << "\"" << endl;
    if( !ns.isEmpty() )
        bout << "using namespace " << ns << ";" << endl;

    bout << endl;

    bout << name << "::" << name << "() {}" << endl << endl;

    bout << name << "::~" << name << "() {}" << endl << endl;

    body.setContents(tmp2);
    body.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    res << header << body;
    return res;
}

bool Wizards::ClassWizardFactory::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    ClassWizardDialog* dlg = (ClassWizardDialog*)w;
    QStringList paths;
    if( !l.isEmpty() )
        paths << l.last().path();
    if( dlg->d_where )
    {
        QStringList done;
        if( !dlg->d_where->addFiles(paths, &done) )
        {
            *errorMessage = "cannot add files to project node";
            foreach( const Core::GeneratedFile& f, l )
                QFile::remove(f.path());
            return false;
        }
    }
    return BaseFileWizardFactory::postGenerateOpenEditors(l, errorMessage);
}

Wizards::ClassWizardPage::ClassWizardPage(Wizards::ClassWizardDialog * dlg, QWidget * parent)
{
    setTitle(tr("Create C++ Class"));
    setProperty("shortTitle", tr("Details"));

    QFormLayout* form = new QFormLayout(this);
    d_name = new QLineEdit(this);
    form->addRow(tr("Class name:"), d_name);
    connect(d_name,SIGNAL(textChanged(QString)),this,SLOT(onChanged()));
    d_ns = new QLineEdit(this);
    form->addRow(tr("Namespace (optional):"), d_ns);
    connect(d_ns,SIGNAL(textChanged(QString)),this,SLOT(onChanged()));
}

bool Wizards::ClassWizardPage::isComplete() const
{
    return !d_name->text().isEmpty();
}

static bool validIdent(const QString& ident)
{
    if( ident.isEmpty() )
        return false;
    ushort ch = ident[0].unicode();
    if( ch > 127 )
        return false;
    if( ch != '_' && !::isalpha(ch) )
        return false;
    for( int i = 1; i < ident.size(); i++ )
    {
        ch = ident[i].unicode();
        if( ch > 127 )
            return false;
        if( ch != '_' && !::isalnum(ch) )
            return false;
    }
    return true;
}

bool Wizards::ClassWizardPage::validatePage()
{
    return (!d_name->text().isEmpty() && validIdent(d_name->text())) &&
            (d_ns->text().isEmpty() || validIdent(d_ns->text()));
}

void Wizards::ClassWizardPage::onChanged()
{
    emit completeChanged();
}

Wizards::ClassWizardDialog::ClassWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent)
    : Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    // setWizardStyle(QWizard::AeroStyle);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/wizards/images/genericfilewizard.png"));

    m_firstPage = new ClassWizardPage(this,parent);
    addPage(m_firstPage);
}
