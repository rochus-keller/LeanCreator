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

#include "busyinstallstep.h"

#include "busybuildconfiguration.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"

#include "ui_busyinstallstepconfigwidget.h"

#include <core/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char BUSY_INSTALL_ROOT[] = "Busy.InstallRoot";
static const char BUSY_REMOVE_FIRST[] = "Busy.RemoveFirst";
static const char BUSY_DRY_RUN[] = "Busy.DryRun";
static const char BUSY_KEEP_GOING[] = "Busy.DryKeepGoing";

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BusyInstallStep:
// --------------------------------------------------------------------

BusyInstallStep::BusyInstallStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_INSTALLSTEP_ID)),
    m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("Busy Install"));
}

BusyInstallStep::BusyInstallStep(ProjectExplorer::BuildStepList *bsl, const BusyInstallStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_INSTALLSTEP_ID)),
    m_qbsInstallOptions(other->m_qbsInstallOptions), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

BusyInstallStep::~BusyInstallStep()
{
    cancel();
    if (m_job)
        m_job->deleteLater();
    m_job = 0;
}

bool BusyInstallStep::init()
{
    QTC_ASSERT(!static_cast<BusyProject *>(project())->isParsing() && !m_job, return false);
    return true;
}

void BusyInstallStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    BusyProject *pro = static_cast<BusyProject *>(project());
    m_job = pro->install(m_qbsInstallOptions);

    if (!m_job) {
        m_fi->reportResult(false);
        emit finished();
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,busy::AbstractJob*)), this, SLOT(installDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,busy::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,busy::AbstractJob*)),
            this, SLOT(handleProgress(int)));
}

ProjectExplorer::BuildStepConfigWidget *BusyInstallStep::createConfigWidget()
{
    return new BusyInstallStepConfigWidget(this);
}

bool BusyInstallStep::runInGuiThread() const
{
    return true;
}

void BusyInstallStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

QString BusyInstallStep::installRoot() const
{
    if (!m_qbsInstallOptions.installRoot().isEmpty())
        return m_qbsInstallOptions.installRoot();

    return busy::InstallOptions::defaultInstallRoot();
}

QString BusyInstallStep::absoluteInstallRoot() const
{
    const busy::Module data = static_cast<BusyProject *>(project())->busyModule();
    QString path = installRoot();
    if (data.isValid() && !data.buildDirectory().isEmpty() && !path.isEmpty())
        path = QDir(data.buildDirectory()).absoluteFilePath(path);
    return path;
}

bool BusyInstallStep::removeFirst() const
{
    return m_qbsInstallOptions.removeExistingInstallation();
}

bool BusyInstallStep::dryRun() const
{
    return m_qbsInstallOptions.dryRun();
}

bool BusyInstallStep::keepGoing() const
{
    return m_qbsInstallOptions.keepGoing();
}

bool BusyInstallStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setInstallRoot(map.value(QLatin1String(BUSY_INSTALL_ROOT)).toString());
    m_qbsInstallOptions.setRemoveExistingInstallation(map.value(QLatin1String(BUSY_REMOVE_FIRST), false).toBool());
    m_qbsInstallOptions.setDryRun(map.value(QLatin1String(BUSY_DRY_RUN), false).toBool());
    m_qbsInstallOptions.setKeepGoing(map.value(QLatin1String(BUSY_KEEP_GOING), false).toBool());

    return true;
}

QVariantMap BusyInstallStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(BUSY_INSTALL_ROOT), m_qbsInstallOptions.installRoot());
    map.insert(QLatin1String(BUSY_REMOVE_FIRST), m_qbsInstallOptions.removeExistingInstallation());
    map.insert(QLatin1String(BUSY_DRY_RUN), m_qbsInstallOptions.dryRun());
    map.insert(QLatin1String(BUSY_KEEP_GOING), m_qbsInstallOptions.keepGoing());

    return map;
}

busy::InstallOptions BusyInstallStep::installOptions() const
{
    return m_qbsInstallOptions;
}

void BusyInstallStep::installDone(bool success)
{
    // Report errors:
    foreach (const busy::ErrorItem &item, m_job->error().items()) {
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());
    }

    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(success);
    m_fi = 0; // do not delete, it is not ours
    m_job->deleteLater();
    m_job = 0;

    emit finished();
}

void BusyInstallStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);
    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void BusyInstallStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void BusyInstallStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                                         const QString &message, const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, NormalOutput);
}

void BusyInstallStep::setInstallRoot(const QString &ir)
{
    if (m_qbsInstallOptions.installRoot() == ir)
        return;
    m_qbsInstallOptions.setInstallRoot(ir);
    emit changed();
}

void BusyInstallStep::setRemoveFirst(bool rf)
{
    if (m_qbsInstallOptions.removeExistingInstallation() == rf)
        return;
    m_qbsInstallOptions.setRemoveExistingInstallation(rf);
    emit changed();
}

void BusyInstallStep::setDryRun(bool dr)
{
    if (m_qbsInstallOptions.dryRun() == dr)
        return;
    m_qbsInstallOptions.setDryRun(dr);
    emit changed();
}

void BusyInstallStep::setKeepGoing(bool kg)
{
    if (m_qbsInstallOptions.keepGoing() == kg)
        return;
    m_qbsInstallOptions.setKeepGoing(kg);
    emit changed();
}

// --------------------------------------------------------------------
// BusyInstallStepConfigWidget:
// --------------------------------------------------------------------

BusyInstallStepConfigWidget::BusyInstallStepConfigWidget(BusyInstallStep *step) :
    m_step(step), m_ignoreChange(false)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(changed()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    BusyProject *project = static_cast<BusyProject *>(m_step->project());

    m_ui = new Ui::BusyInstallStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->installRootChooser->setPromptDialogTitle(tr("Busy Install Prefix"));
    m_ui->installRootChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->installRootChooser->setHistoryCompleter(QLatin1String("Busy.InstallRoot.History"));

    connect(m_ui->installRootChooser, SIGNAL(rawPathChanged(QString)), this,
            SLOT(changeInstallRoot()));
    connect(m_ui->removeFirstCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeRemoveFirst(bool)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));

    connect(project, SIGNAL(projectParsingDone(bool)), this, SLOT(updateState()));

    updateState();
}

BusyInstallStepConfigWidget::~BusyInstallStepConfigWidget()
{
    delete m_ui;
}

QString BusyInstallStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString BusyInstallStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void BusyInstallStepConfigWidget::updateState()
{
    if (!m_ignoreChange) {
        m_ui->installRootChooser->setPath(m_step->installRoot());
        m_ui->removeFirstCheckBox->setChecked(m_step->removeFirst());
        m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    }

    m_ui->installRootChooser->setBaseFileName(m_step->target()->activeBuildConfiguration()->buildDirectory());

    QString command = BusyBuildConfiguration::equivalentCommandLine(m_step);
    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>BUSY:</b> %1").arg(command);
    if (m_summary != summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void BusyInstallStepConfigWidget::changeInstallRoot()
{
    const QString path = m_ui->installRootChooser->path();
    if (m_step->installRoot() == path)
        return;

    m_ignoreChange = true;
    m_step->setInstallRoot(path);
    m_ignoreChange = false;
}

void BusyInstallStepConfigWidget::changeRemoveFirst(bool rf)
{
    m_step->setRemoveFirst(rf);
}

void BusyInstallStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void BusyInstallStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

// --------------------------------------------------------------------
// BusyInstallStepFactory:
// --------------------------------------------------------------------

BusyInstallStepFactory::BusyInstallStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<Core::Id> BusyInstallStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY
            && qobject_cast<ProjectExplorer::DeployConfiguration *>(parent->parent())
            && qobject_cast<BusyProject *>(parent->target()->project()))
        return QList<Core::Id>() << Core::Id(Constants::BUSY_INSTALLSTEP_ID);
    return QList<Core::Id>();
}

QString BusyInstallStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::BUSY_INSTALLSTEP_ID))
        return tr("Busy Install");
    return QString();
}

bool BusyInstallStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            || !qobject_cast<ProjectExplorer::DeployConfiguration *>(parent->parent())
            || !qobject_cast<BusyProject *>(parent->target()->project()))
        return false;
    return id == Core::Id(Constants::BUSY_INSTALLSTEP_ID);
}

ProjectExplorer::BuildStep *BusyInstallStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                          const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new BusyInstallStep(parent);
}

bool BusyInstallStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *BusyInstallStepFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                           const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    BusyInstallStep *bs = new BusyInstallStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool BusyInstallStepFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                     ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *BusyInstallStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                         ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new BusyInstallStep(parent, static_cast<BusyInstallStep *>(product));
}

} // namespace Internal
} // namespace BusyProjectManager
