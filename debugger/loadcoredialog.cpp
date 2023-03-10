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

#include "loadcoredialog.h"

#include "debuggerstartparameters.h"
#include "debuggerdialogs.h"
#include "debuggerkitinformation.h"
#include "gdb/coregdbadapter.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#ifndef QT_NO_SSH
#include <ssh/sftpfilesystemmodel.h>
#endif
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QTemporaryFile>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTextBrowser>
#include <QTreeView>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// SelectRemoteFileDialog
//
///////////////////////////////////////////////////////////////////////

class SelectRemoteFileDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::SelectRemoteFileDialog)

public:
    explicit SelectRemoteFileDialog(QWidget *parent);

    void attachToDevice(Kit *k);
    QString localFile() const { return m_localFile; }
    QString remoteFile() const { return m_remoteFile; }

private:
#ifndef QT_NO_SSH
    void handleSftpOperationFinished(SftpJobId, const QString &error);
#endif
    void handleSftpOperationFailed(const QString &errorMessage);
    void handleConnectionError(const QString &errorMessage);
    void handleRemoteError(const QString &errorMessage);
    void selectFile();

    QSortFilterProxyModel m_model;
#ifndef QT_NO_SSH
    SftpFileSystemModel m_fileSystemModel;
    SftpJobId m_sftpJobId;
#endif
    QTreeView *m_fileSystemView;
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;
    QString m_localFile;
    QString m_remoteFile;
};

SelectRemoteFileDialog::SelectRemoteFileDialog(QWidget *parent)
    : QDialog(parent)
{
#ifndef QT_NO_SSH
    m_model.setSourceModel(&m_fileSystemModel);
#endif

    m_fileSystemView = new QTreeView(this);
    m_fileSystemView->setModel(&m_model);
    m_fileSystemView->setSortingEnabled(true);
    m_fileSystemView->sortByColumn(1, Qt::AscendingOrder);
    m_fileSystemView->setUniformRowHeights(true);
    m_fileSystemView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileSystemView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileSystemView->header()->setDefaultSectionSize(100);
    m_fileSystemView->header()->setStretchLastSection(true);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setEnabled(false);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_fileSystemView);
    layout->addWidget(m_textBrowser);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &SelectRemoteFileDialog::selectFile);

#ifndef QT_NO_SSH
    connect(&m_fileSystemModel, &SftpFileSystemModel::sftpOperationFailed,
            this, &SelectRemoteFileDialog::handleSftpOperationFailed);
    connect(&m_fileSystemModel, &SftpFileSystemModel::connectionError,
            this, &SelectRemoteFileDialog::handleConnectionError);
#endif
}

void SelectRemoteFileDialog::attachToDevice(Kit *k)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    QTC_ASSERT(k, return);
    IDevice::ConstPtr device = DeviceKitInformation::device(k);
    QTC_ASSERT(device, return);
#ifndef QT_NO_SSH
    SshConnectionParameters sshParams = device->sshParameters();
    m_fileSystemModel.setSshConnection(sshParams);
#endif
}

void SelectRemoteFileDialog::handleSftpOperationFailed(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
    //reject();
}

void SelectRemoteFileDialog::handleConnectionError(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
    //reject();
}

#ifndef QT_NO_SSH
void SelectRemoteFileDialog::handleSftpOperationFinished(SftpJobId, const QString &error)
{
    if (error.isEmpty()) {
        m_textBrowser->append(tr("Download of remote file succeeded."));
        accept();
    } else {
        m_textBrowser->append(error);
        //reject();
    }
}
#endif

void SelectRemoteFileDialog::handleRemoteError(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
}

void SelectRemoteFileDialog::selectFile()
{
    QModelIndex idx = m_model.mapToSource(m_fileSystemView->currentIndex());
    if (!idx.isValid())
        return;

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_fileSystemView->setEnabled(false);

#ifndef QT_NO_SSH
    connect(&m_fileSystemModel, &SftpFileSystemModel::sftpOperationFinished,
            this, &SelectRemoteFileDialog::handleSftpOperationFinished);
#endif
    {
        QTemporaryFile localFile(QDir::tempPath() + QLatin1String("/remotecore-XXXXXX"));
        localFile.open();
        m_localFile = localFile.fileName();
    }

    idx = idx.sibling(idx.row(), 1);
#ifndef QT_NO_SSH
    m_remoteFile = m_fileSystemModel.data(idx, SftpFileSystemModel::PathRole).toString();
    m_sftpJobId = m_fileSystemModel.downloadFile(idx, m_localFile);
#endif
}

///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

class AttachCoreDialogPrivate
{
public:
    KitChooser *kitChooser;

    QCheckBox *forceLocalCheckBox;
    QLabel *forceLocalLabel;
    PathChooser *localExecFileName;
    PathChooser *localCoreFileName;
    QLineEdit *remoteCoreFileName;
    QPushButton *selectRemoteCoreButton;

    PathChooser *overrideStartScriptFileName;

    QDialogButtonBox *buttonBox;

    struct State
    {
        bool isValid() const
        {
            return validKit && validLocalExecFilename && validCoreFilename;
        }

        bool validKit;
        bool validLocalExecFilename;
        bool validCoreFilename;
        bool localCoreFile;
        bool localKit;
    };

    State getDialogState(const AttachCoreDialog &p) const
    {
        State st;
        st.localCoreFile = p.useLocalCoreFile();
        st.validKit = (kitChooser->currentKit() != 0);
        st.validLocalExecFilename = localExecFileName->isValid();

        if (st.localCoreFile)
            st.validCoreFilename = localCoreFileName->isValid();
        else
            st.validCoreFilename = !p.remoteCoreFile().isEmpty();

        st.localKit = p.isLocalKit();
        return st;
    }
};

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
    : QDialog(parent), d(new AttachCoreDialogPrivate)
{
    setWindowTitle(tr("Load Core File"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    d->kitChooser = new DebuggerKitChooser(DebuggerKitChooser::AnyDebugging, this);
    d->kitChooser->populate();

    d->forceLocalCheckBox = new QCheckBox(this);
    d->forceLocalLabel = new QLabel(this);
    d->forceLocalLabel->setText(tr("Use local core file:"));
    d->forceLocalLabel->setBuddy(d->forceLocalCheckBox);

    d->remoteCoreFileName = new QLineEdit(this);
    d->selectRemoteCoreButton = new QPushButton(PathChooser::browseButtonLabel(), this);

    d->localCoreFileName = new PathChooser(this);
    d->localCoreFileName->setHistoryCompleter(QLatin1String("Debugger.CoreFile.History"));
    d->localCoreFileName->setExpectedKind(PathChooser::File);
    d->localCoreFileName->setPromptDialogTitle(tr("Select Core File"));

    d->localExecFileName = new PathChooser(this);
    d->localExecFileName->setHistoryCompleter(QLatin1String("LocalExecutable"));
    d->localExecFileName->setExpectedKind(PathChooser::File);
    d->localExecFileName->setPromptDialogTitle(tr("Select Executable"));

    d->overrideStartScriptFileName = new PathChooser(this);
    d->overrideStartScriptFileName->setHistoryCompleter(QLatin1String("Debugger.StartupScript.History"));
    d->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    d->overrideStartScriptFileName->setPromptDialogTitle(tr("Select Startup Script"));

    auto coreLayout = new QHBoxLayout;
    coreLayout->addWidget(d->localCoreFileName);
    coreLayout->addWidget(d->remoteCoreFileName);
    coreLayout->addWidget(d->selectRemoteCoreButton);

    auto formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(6);
    formLayout->setVerticalSpacing(6);
    formLayout->addRow(tr("Kit:"), d->kitChooser);
    formLayout->addRow(d->forceLocalLabel, d->forceLocalCheckBox);
    formLayout->addRow(tr("Core file:"), coreLayout);
    formLayout->addRow(tr("&Executable:"), d->localExecFileName);
    formLayout->addRow(tr("Override &start script:"), d->overrideStartScriptFileName);

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    formLayout->addRow(d->buttonBox);

    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(line);
    vboxLayout->addWidget(d->buttonBox);
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete d;
}

int AttachCoreDialog::exec()
{
    connect(d->selectRemoteCoreButton, &QAbstractButton::clicked, this, &AttachCoreDialog::selectRemoteCoreFile);
    connect(d->remoteCoreFileName, &QLineEdit::textChanged, this, &AttachCoreDialog::coreFileChanged);
    connect(d->localExecFileName, &PathChooser::rawPathChanged, this, &AttachCoreDialog::changed);
    connect(d->localCoreFileName, &PathChooser::rawPathChanged, this, &AttachCoreDialog::coreFileChanged);
    connect(d->forceLocalCheckBox, &QCheckBox::stateChanged, this, &AttachCoreDialog::changed);
    connect(d->kitChooser, &KitChooser::currentIndexChanged, this, &AttachCoreDialog::changed);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    changed();

    AttachCoreDialogPrivate::State st = d->getDialogState(*this);
    if (!st.validKit) {
        d->kitChooser->setFocus();
    } else if (!st.validCoreFilename) {
        if (st.localCoreFile)
            d->localCoreFileName->setFocus();
        else
            d->remoteCoreFileName->setFocus();
    } else if (!st.validLocalExecFilename) {
        d->localExecFileName->setFocus();
    }

    return QDialog::exec();
}

bool AttachCoreDialog::isLocalKit() const
{
    Kit *k = d->kitChooser->currentKit();
    QTC_ASSERT(k, return false);
    IDevice::ConstPtr device = DeviceKitInformation::device(k);
    QTC_ASSERT(device, return false);
    return device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

bool AttachCoreDialog::useLocalCoreFile() const
{
    return isLocalKit() || d->forceLocalCheckBox->isChecked();
}

void AttachCoreDialog::coreFileChanged(const QString &core)
{
    if (!HostOsInfo::isWindowsHost() && QFile::exists(core)) {
        Kit *k = d->kitChooser->currentKit();
        QTC_ASSERT(k, return);
        FileName cmd = DebuggerKitInformation::debuggerCommand(k);
        GdbCoreEngine::CoreInfo cinfo =
            GdbCoreEngine::readExecutableNameFromCore(cmd.toString(), core);
        if (!cinfo.foundExecutableName.isEmpty())
            d->localExecFileName->setFileName(FileName::fromString(cinfo.foundExecutableName));
        else if (!d->localExecFileName->isValid() && !cinfo.rawStringFromCore.isEmpty())
            d->localExecFileName->setFileName(FileName::fromString(cinfo.rawStringFromCore));
    }
    changed();
}

void AttachCoreDialog::changed()
{
    AttachCoreDialogPrivate::State st = d->getDialogState(*this);

    d->forceLocalLabel->setVisible(!st.localKit);
    d->forceLocalCheckBox->setVisible(!st.localKit);
    if (st.localCoreFile) {
        d->localCoreFileName->setVisible(true);
        d->remoteCoreFileName->setVisible(false);
        d->selectRemoteCoreButton->setVisible(false);
    } else {
        d->localCoreFileName->setVisible(false);
        d->remoteCoreFileName->setVisible(true);
        d->selectRemoteCoreButton->setVisible(true);
    }

    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(st.isValid());
}

void AttachCoreDialog::selectRemoteCoreFile()
{
    changed();
    QTC_ASSERT(!isLocalKit(), return);
    SelectRemoteFileDialog dlg(this);
    dlg.setWindowTitle(tr("Select Remote Core File"));
    dlg.attachToDevice(d->kitChooser->currentKit());
    if (dlg.exec() == QDialog::Rejected)
        return;
    d->localCoreFileName->setPath(dlg.localFile());
    d->remoteCoreFileName->setText(dlg.remoteFile());
    changed();
}

QString AttachCoreDialog::localCoreFile() const
{
    return d->localCoreFileName->path();
}

QString AttachCoreDialog::localExecutableFile() const
{
    return d->localExecFileName->path();
}

void AttachCoreDialog::setLocalExecutableFile(const QString &fileName)
{
    d->localExecFileName->setPath(fileName);
}

void AttachCoreDialog::setLocalCoreFile(const QString &fileName)
{
    d->localCoreFileName->setPath(fileName);
}

void AttachCoreDialog::setRemoteCoreFile(const QString &fileName)
{
    d->remoteCoreFileName->setText(fileName);
}

QString AttachCoreDialog::remoteCoreFile() const
{
    return d->remoteCoreFileName->text();
}

void AttachCoreDialog::setKitId(Id id)
{
    d->kitChooser->setCurrentKitId(id);
}

void AttachCoreDialog::setForceLocalCoreFile(bool on)
{
    d->forceLocalCheckBox->setChecked(on);
}

bool AttachCoreDialog::forcesLocalCoreFile() const
{
    return d->forceLocalCheckBox->isChecked();
}

Kit *AttachCoreDialog::kit() const
{
    return d->kitChooser->currentKit();
}

QString AttachCoreDialog::overrideStartScript() const
{
    return d->overrideStartScriptFileName->path();
}

void AttachCoreDialog::setOverrideStartScript(const QString &scriptName)
{
    d->overrideStartScriptFileName->setPath(scriptName);
}

} // namespace Internal
} // namespace Debugger
