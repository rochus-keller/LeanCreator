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

#include "busycleanstep.h"

#include "busybuildconfiguration.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"

#include "ui_busycleanstepconfigwidget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

static const char BUSY_CLEAN_ALL[] = "Busy.CleanAll";
static const char BUSY_DRY_RUN[] = "Busy.DryRun";
static const char BUSY_KEEP_GOING[] = "Busy.DryKeepGoing";

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BusyCleanStep:
// --------------------------------------------------------------------

BusyCleanStep::BusyCleanStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_CLEANSTEP_ID)),
    m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("Busy Clean"));
}

BusyCleanStep::BusyCleanStep(ProjectExplorer::BuildStepList *bsl, const BusyCleanStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_CLEANSTEP_ID)),
    m_qbsCleanOptions(other->m_qbsCleanOptions), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

BusyCleanStep::~BusyCleanStep()
{
    cancel();
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }
}

bool BusyCleanStep::init()
{
    if (static_cast<BusyProject *>(project())->isParsing() || m_job)
        return false;

    BusyBuildConfiguration *bc = static_cast<BusyBuildConfiguration *>(buildConfiguration());
    if (!bc)
        bc = static_cast<BusyBuildConfiguration *>(target()->activeBuildConfiguration());

    if (!bc)
        return false;

    return true;
}

void BusyCleanStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    BusyProject *pro = static_cast<BusyProject *>(project());
    busy::CleanOptions options(m_qbsCleanOptions);

    m_job = pro->clean(options);

    if (!m_job) {
        m_fi->reportResult(false);
        emit finished();
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(taskFinished(bool)), this, SLOT(cleaningDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int)),this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int)), this, SLOT(handleProgress(int)));

    m_job->start();
}

ProjectExplorer::BuildStepConfigWidget *BusyCleanStep::createConfigWidget()
{
    return new BusyCleanStepConfigWidget(this);
}

bool BusyCleanStep::runInGuiThread() const
{
    return true;
}

void BusyCleanStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

bool BusyCleanStep::dryRun() const
{
    return m_qbsCleanOptions.dryRun();
}

bool BusyCleanStep::keepGoing() const
{
    return m_qbsCleanOptions.keepGoing();
}

int BusyCleanStep::maxJobs() const
{
    return 1;
}

bool BusyCleanStep::cleanAll() const
{
    return m_qbsCleanOptions.cleanType() == busy::CleanOptions::CleanupAll;
}

bool BusyCleanStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_qbsCleanOptions.setDryRun(map.value(QLatin1String(BUSY_DRY_RUN)).toBool());
    m_qbsCleanOptions.setKeepGoing(map.value(QLatin1String(BUSY_KEEP_GOING)).toBool());
    m_qbsCleanOptions.setCleanType(map.value(QLatin1String(BUSY_CLEAN_ALL)).toBool()
            ? busy::CleanOptions::CleanupAll : busy::CleanOptions::CleanupTemporaries);

    return true;
}

QVariantMap BusyCleanStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(BUSY_DRY_RUN), m_qbsCleanOptions.dryRun());
    map.insert(QLatin1String(BUSY_KEEP_GOING), m_qbsCleanOptions.keepGoing());
    map.insert(QLatin1String(BUSY_CLEAN_ALL),
               m_qbsCleanOptions.cleanType() == busy::CleanOptions::CleanupAll);

    return map;
}

void BusyCleanStep::cleaningDone(bool success)
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

void BusyCleanStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);
    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void BusyCleanStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void BusyCleanStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message, const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, NormalOutput);
}

void BusyCleanStep::setDryRun(bool dr)
{
    if (m_qbsCleanOptions.dryRun() == dr)
        return;
    m_qbsCleanOptions.setDryRun(dr);
    emit changed();
}

void BusyCleanStep::setKeepGoing(bool kg)
{
    if (m_qbsCleanOptions.keepGoing() == kg)
        return;
    m_qbsCleanOptions.setKeepGoing(kg);
    emit changed();
}

void BusyCleanStep::setMaxJobs(int jobcount)
{
    Q_UNUSED(jobcount); // TODO: Remove all job count-related stuff.
    emit changed();
}

void BusyCleanStep::setCleanAll(bool ca)
{
    busy::CleanOptions::CleanType newType = ca
            ? busy::CleanOptions::CleanupAll : busy::CleanOptions::CleanupTemporaries;
    if (m_qbsCleanOptions.cleanType() == newType)
        return;
    m_qbsCleanOptions.setCleanType(newType);
    emit changed();
}

// --------------------------------------------------------------------
// BusyCleanStepConfigWidget:
// --------------------------------------------------------------------

BusyCleanStepConfigWidget::BusyCleanStepConfigWidget(BusyCleanStep *step) :
    m_step(step)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(changed()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::BusyCleanStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->flagsLabel->hide();
    m_ui->cleanAllCheckBox->hide();
    m_ui->dryRunCheckBox->hide();
    m_ui->keepGoingCheckBox->hide();
    m_ui->commandLineKeyLabel->hide();
    m_ui->commandLineTextEdit->hide();

    connect(m_ui->cleanAllCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(changeCleanAll(bool)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));

    updateState();
}

BusyCleanStepConfigWidget::~BusyCleanStepConfigWidget()
{
    delete m_ui;
}

QString BusyCleanStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString BusyCleanStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void BusyCleanStepConfigWidget::updateState()
{
    m_ui->cleanAllCheckBox->setChecked(m_step->cleanAll());
    m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
    m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());

    QString command = BusyBuildConfiguration::equivalentCommandLine(m_step);
    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>BUSY:</b> %1").arg(command);
    if (m_summary !=  summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void BusyCleanStepConfigWidget::changeCleanAll(bool ca)
{
    m_step->setCleanAll(ca);
}

void BusyCleanStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void BusyCleanStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

void BusyCleanStepConfigWidget::changeJobCount(int count)
{
    m_step->setMaxJobs(count);
}

// --------------------------------------------------------------------
// BusyCleanStepFactory:
// --------------------------------------------------------------------

BusyCleanStepFactory::BusyCleanStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<Core::Id> BusyCleanStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && qobject_cast<BusyBuildConfiguration *>(parent->parent()))
        return QList<Core::Id>() << Core::Id(Constants::BUSY_CLEANSTEP_ID);
    return QList<Core::Id>();
}

QString BusyCleanStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::BUSY_CLEANSTEP_ID))
        return tr("Busy Clean");
    return QString();
}

bool BusyCleanStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
            || !qobject_cast<BusyBuildConfiguration *>(parent->parent()))
        return false;
    return id == Core::Id(Constants::BUSY_CLEANSTEP_ID);
}

ProjectExplorer::BuildStep *BusyCleanStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new BusyCleanStep(parent);
}

bool BusyCleanStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *BusyCleanStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    BusyCleanStep *bs = new BusyCleanStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool BusyCleanStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *BusyCleanStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new BusyCleanStep(parent, static_cast<BusyCleanStep *>(product));
}

} // namespace Internal
} // namespace BusyProjectManager
