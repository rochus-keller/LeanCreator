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

#include "busybuildstep.h"

#include "busybuildconfiguration.h"
#include "busyparser.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"
#include "busyhighlighter.h"

#include "ui_busybuildstepconfigwidget.h"

#include <QSyntaxHighlighter>
#include <core/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
//#include <qtsupport/debugginghelperbuildtask.h>
//#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <busy/busyapi.h>

static const char BUSY_CONFIG[] = "Busy.Configuration";
static const char BUSY_DRY_RUN[] = "Busy.DryRun";
static const char BUSY_KEEP_GOING[] = "Busy.DryKeepGoing";
static const char BUSY_MAXJOBCOUNT[] = "Busy.MaxJobs";
static const char BUSY_SHOWCOMMANDLINES[] = "Busy.ShowCommandLines";
static const char BUSY_INSTALL[] = "Busy.Install";
static const char BUSY_CLEAN_INSTALL_ROOT[] = "Busy.CleanInstallRoot";

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BusyBuildStep:
// --------------------------------------------------------------------

BusyBuildStep::BusyBuildStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_BUILDSTEP_ID)),
    m_job(0), m_parser(0), m_parsingProject(false)
{
    setDisplayName(tr("Busy Build"));
    setBusyConfiguration(QVariantMap());
}

BusyBuildStep::BusyBuildStep(ProjectExplorer::BuildStepList *bsl, const BusyBuildStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::BUSY_BUILDSTEP_ID)),
    m_qbsBuildOptions(other->m_qbsBuildOptions),  m_job(0), m_parser(0), m_parsingProject(false)
{
    setBusyConfiguration(other->busyConfiguration());
}

BusyBuildStep::~BusyBuildStep()
{
    cancel();
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }
    delete m_parser;
}

bool BusyBuildStep::init()
{
    if (static_cast<BusyProject *>(project())->isParsing() || m_job)
        return false;

    ProjectExplorer::Kit *kit = target()->kit();

    BusyBuildConfiguration *bc = static_cast<BusyBuildConfiguration *>(buildConfiguration());
    if (!bc)
        bc = static_cast<BusyBuildConfiguration *>(target()->activeBuildConfiguration());

    if (!bc)
        return false;

    delete m_parser;
    m_parser = new Internal::BusyParser;
    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        m_parser->appendOutputParser(parser);

    m_changedFiles = bc->changedFiles();
    m_activeFileTags = bc->activeFileTags();
    m_products = bc->products();

    connect(m_parser, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
            this, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)));
    connect(m_parser, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SIGNAL(addTask(ProjectExplorer::Task)));

    return true;
}

void BusyBuildStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    // We need a pre-build parsing step in order not to lose project file changes done
    // right before building (but before the delay has elapsed).
    parseProject();
}

ProjectExplorer::BuildStepConfigWidget *BusyBuildStep::createConfigWidget()
{
    return new BusyBuildStepConfigWidget(this);
}

bool BusyBuildStep::runInGuiThread() const
{
    return true;
}

void BusyBuildStep::cancel()
{
    if (m_parsingProject)
        busyProject()->cancelParsing();
    else if (m_job)
        m_job->cancel();
}

QVariantMap BusyBuildStep::busyConfiguration() const
{
    return m_qbsConfiguration;
}

void BusyBuildStep::setBusyConfiguration(const QVariantMap &config)
{
    BusyProject *pro = static_cast<BusyProject *>(project());

    QVariantMap tmp = config;
    tmp.insert(QLatin1String(Constants::BUSY_CONFIG_PROFILE_KEY), pro->projectManager()->profileForKit(target()->kit()));
    if (!tmp.contains(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY)))
        tmp.insert(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY),
                   QString::fromLatin1(Constants::BUSY_VARIANT_DEBUG));

    if (tmp == m_qbsConfiguration)
        return;
    m_qbsConfiguration = tmp;
    BusyBuildConfiguration *bc = static_cast<BusyBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
    emit busyConfigurationChanged();
}

bool BusyBuildStep::dryRun() const
{
    return m_qbsBuildOptions.dryRun();
}

bool BusyBuildStep::keepGoing() const
{
    return m_qbsBuildOptions.keepGoing();
}

bool BusyBuildStep::showCommandLines() const
{
    return m_qbsBuildOptions.echoMode() == busy::CommandEchoModeCommandLine;
}

bool BusyBuildStep::install() const
{
    return m_qbsBuildOptions.install();
}

bool BusyBuildStep::cleanInstallRoot() const
{
    return m_qbsBuildOptions.removeExistingInstallation();
}

int BusyBuildStep::maxJobs() const
{
    if (m_qbsBuildOptions.maxJobCount() > 0)
        return m_qbsBuildOptions.maxJobCount();
    return busy::BuildOptions::defaultMaxJobCount();
}

bool BusyBuildStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setBusyConfiguration(map.value(QLatin1String(BUSY_CONFIG)).toMap());
    m_qbsBuildOptions.setDryRun(map.value(QLatin1String(BUSY_DRY_RUN)).toBool());
    m_qbsBuildOptions.setKeepGoing(map.value(QLatin1String(BUSY_KEEP_GOING)).toBool());
    m_qbsBuildOptions.setMaxJobCount(map.value(QLatin1String(BUSY_MAXJOBCOUNT)).toInt());
    const bool showCommandLines = map.value(QLatin1String(BUSY_SHOWCOMMANDLINES)).toBool();
    m_qbsBuildOptions.setEchoMode(showCommandLines ? busy::CommandEchoModeCommandLine
                                                   : busy::CommandEchoModeSummary);
    m_qbsBuildOptions.setInstall(map.value(QLatin1String(BUSY_INSTALL), true).toBool());
    m_qbsBuildOptions.setRemoveExistingInstallation(map.value(QLatin1String(BUSY_CLEAN_INSTALL_ROOT))
                                                    .toBool());
    return true;
}

QVariantMap BusyBuildStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(BUSY_CONFIG), m_qbsConfiguration);
    map.insert(QLatin1String(BUSY_DRY_RUN), m_qbsBuildOptions.dryRun());
    map.insert(QLatin1String(BUSY_KEEP_GOING), m_qbsBuildOptions.keepGoing());
    map.insert(QLatin1String(BUSY_MAXJOBCOUNT), m_qbsBuildOptions.maxJobCount());
    map.insert(QLatin1String(BUSY_SHOWCOMMANDLINES),
               m_qbsBuildOptions.echoMode() == busy::CommandEchoModeCommandLine);
    map.insert(QLatin1String(BUSY_INSTALL), m_qbsBuildOptions.install());
    map.insert(QLatin1String(BUSY_CLEAN_INSTALL_ROOT),
               m_qbsBuildOptions.removeExistingInstallation());
    return map;
}

void BusyBuildStep::buildingDone(bool success)
{
    m_lastWasSuccess = success;
    // Report errors:
    foreach (const busy::ErrorItem &item, m_job->error().items())
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());

    BusyProject *pro = static_cast<BusyProject *>(project());

    // Building can uncover additional target artifacts.
    pro->updateAfterBuild();

    // The reparsing, if it is necessary, has to be done before finished() is emitted, as
    // otherwise a potential additional build step could conflict with the parsing step.
    if (pro->parsingScheduled())
        parseProject();
    else
        finish();
}

void BusyBuildStep::reparsingDone(bool success)
{
    disconnect(busyProject(), SIGNAL(projectParsingDone(bool)), this, SLOT(reparsingDone(bool)));
    m_parsingProject = false;
    if (m_job) { // This was a scheduled reparsing after building.
        finish();
    } else if (!success) {
        m_lastWasSuccess = false;
        finish();
    } else {
        build();
    }
}

void BusyBuildStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);

    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void BusyBuildStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void BusyBuildStep::handleCommandDescriptionReport(const QString &highlight, const QString &message)
{
    Q_UNUSED(highlight);
    emit addOutput(message, NormalOutput);
}

void BusyBuildStep::handleProcessResultReport(const busy::ProcessResult &result)
{
    bool hasOutput = !result.stdOut().isEmpty() || !result.stdErr().isEmpty();

    if (result.success() && !hasOutput)
        return;

    m_parser->setWorkingDirectory(result.workingDirectory());

    QString commandline = result.executableFilePath() + QLatin1Char(' ')
            + Utils::QtcProcess::joinArgs(result.arguments());
    addOutput(commandline, NormalOutput);

    foreach (const QString &line, result.stdErr()) {
        m_parser->stdError(line);
        addOutput(line, ErrorOutput);
    }
    foreach (const QString &line, result.stdOut()) {
        m_parser->stdOutput(line);
        addOutput(line, NormalOutput);
    }
    m_parser->flush();
}

void BusyBuildStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message,
                                       const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, NormalOutput);
}

QString BusyBuildStep::buildVariant() const
{
    return busyConfiguration().value(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY)).toString();
}

void BusyBuildStep::setBuildVariant(const QString &variant)
{
    if (m_qbsConfiguration.value(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY)).toString() == variant)
        return;
    m_qbsConfiguration.insert(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY), variant);
    emit busyConfigurationChanged();
    BusyBuildConfiguration *bc = static_cast<BusyBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
}

QString BusyBuildStep::profile() const
{
    return busyConfiguration().value(QLatin1String(Constants::BUSY_CONFIG_PROFILE_KEY)).toString();
}

void BusyBuildStep::setDryRun(bool dr)
{
    if (m_qbsBuildOptions.dryRun() == dr)
        return;
    m_qbsBuildOptions.setDryRun(dr);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::setKeepGoing(bool kg)
{
    if (m_qbsBuildOptions.keepGoing() == kg)
        return;
    m_qbsBuildOptions.setKeepGoing(kg);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::setMaxJobs(int jobcount)
{
    if (m_qbsBuildOptions.maxJobCount() == jobcount)
        return;
    m_qbsBuildOptions.setMaxJobCount(jobcount);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::setShowCommandLines(bool show)
{
    if (showCommandLines() == show)
        return;
    m_qbsBuildOptions.setEchoMode(show ? busy::CommandEchoModeCommandLine
                                       : busy::CommandEchoModeSummary);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::setInstall(bool install)
{
    if (m_qbsBuildOptions.install() == install)
        return;
    m_qbsBuildOptions.setInstall(install);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::setCleanInstallRoot(bool clean)
{
    if (m_qbsBuildOptions.removeExistingInstallation() == clean)
        return;
    m_qbsBuildOptions.setRemoveExistingInstallation(clean);
    emit busyBuildOptionsChanged();
}

void BusyBuildStep::parseProject()
{
    m_parsingProject = true;
    connect(busyProject(), SIGNAL(projectParsingDone(bool)), SLOT(reparsingDone(bool)));
    busyProject()->parseCurrentBuildConfiguration();
}

void BusyBuildStep::build()
{
    busy::BuildOptions options(m_qbsBuildOptions);
    options.setChangedFiles(m_changedFiles);
    options.setFilesToConsider(m_changedFiles);
    options.setActiveFileTags(m_activeFileTags);

    QString error;
    m_job = busyProject()->build(options, m_products, error);
    if (!m_job) {
        emit addOutput(error, ErrorMessageOutput);
        m_fi->reportResult(false);
        emit finished();
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,busy::AbstractJob*)), this, SLOT(buildingDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,busy::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,busy::AbstractJob*)),
            this, SLOT(handleProgress(int)));
    connect(m_job, SIGNAL(reportCommandDescription(QString,QString)),
            this, SLOT(handleCommandDescriptionReport(QString,QString)));
    connect(m_job, SIGNAL(reportProcessResult(busy::ProcessResult)),
            this, SLOT(handleProcessResultReport(busy::ProcessResult)));

}

void BusyBuildStep::finish()
{
    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(m_lastWasSuccess);
    m_fi = 0; // do not delete, it is not ours
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }

    emit finished();
}

BusyProject *BusyBuildStep::busyProject() const
{
    return static_cast<BusyProject *>(project());
}

// --------------------------------------------------------------------
// BusyBuildStepConfigWidget:
// --------------------------------------------------------------------

BusyBuildStepConfigWidget::BusyBuildStepConfigWidget(BusyBuildStep *step) :
    m_step(step),
    m_ignoreChange(false)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(busyConfigurationChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(busyBuildOptionsChanged()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::BusyBuildStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->targetsEdit->setValidationFunction(
                [this](Utils::FancyLineEdit *edit, QString *errorMessage) {
                    return validateTargets(edit, errorMessage);
                });

    m_ui->parametersTextEdit->setWordWrapMode(QTextOption::WordWrap);
    new Highlighter(m_ui->parametersTextEdit->document());

    connect(m_ui->parametersTextEdit, SIGNAL(textChanged()), this, SLOT(changedParams()));
    connect(m_ui->buildVariantComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeBuildVariant(int)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));
    connect(m_ui->jobSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeJobCount(int)));
    connect(m_ui->showCommandLinesCheckBox, &QCheckBox::toggled, this,
            &BusyBuildStepConfigWidget::changeShowCommandLines);
    connect(m_ui->installCheckBox, &QCheckBox::toggled, this,
            &BusyBuildStepConfigWidget::changeInstall);
    connect(m_ui->cleanInstallRootCheckBox, &QCheckBox::toggled, this,
            &BusyBuildStepConfigWidget::changeCleanInstallRoot);
    updateState();
}

BusyBuildStepConfigWidget::~BusyBuildStepConfigWidget()
{
    delete m_ui;
}

QString BusyBuildStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString BusyBuildStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void BusyBuildStepConfigWidget::updateState()
{
    if (!m_ignoreChange) {
        m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
        m_ui->jobSpinBox->setValue(m_step->maxJobs());
        m_ui->showCommandLinesCheckBox->setChecked(m_step->showCommandLines());
        m_ui->installCheckBox->setChecked(m_step->install());
        m_ui->cleanInstallRootCheckBox->setChecked(m_step->cleanInstallRoot());
        updateTargetEdit(m_step->busyConfiguration());
    }


    const QString buildVariant = m_step->buildVariant();
    const int idx = (buildVariant == QLatin1String(Constants::BUSY_VARIANT_DEBUG)) ? 0 : 1;
    m_ui->buildVariantComboBox->setCurrentIndex(idx);
    QString command = BusyBuildConfiguration::equivalentCommandLine(m_step);


    QString summary = tr("<b>Busy:</b> %1").arg(command);
    if (m_summary != summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void BusyBuildStepConfigWidget::updateTargetEdit(const QVariantMap &data)
{
    m_ui->targetsEdit->setText(data.value(Constants::BUSY_CONFIG_TARGETS_KEY).toString());
    m_ui->parametersTextEdit->setPlainText(data[Constants::BUSY_CONFIG_PARAMS_KEY].toString());
}

void BusyBuildStepConfigWidget::changeBuildVariant(int idx)
{
    QString variant;
    if (idx == 1)
        variant = QLatin1String(Constants::BUSY_VARIANT_RELEASE);
    else
        variant = QLatin1String(Constants::BUSY_VARIANT_DEBUG);
    m_ignoreChange = true;
    m_step->setBuildVariant(variant);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeDryRun(bool dr)
{
    m_ignoreChange = true;
    m_step->setDryRun(dr);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeShowCommandLines(bool show)
{
    m_ignoreChange = true;
    m_step->setShowCommandLines(show);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeKeepGoing(bool kg)
{
    m_ignoreChange = true;
    m_step->setKeepGoing(kg);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeJobCount(int count)
{
    m_ignoreChange = true;
    m_step->setMaxJobs(count);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeInstall(bool install)
{
    m_ignoreChange = true;
    m_step->setInstall(install);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changeCleanInstallRoot(bool clean)
{
    m_ignoreChange = true;
    m_step->setCleanInstallRoot(clean);
    m_ignoreChange = false;
}

void BusyBuildStepConfigWidget::changedParams()
{
    BusyParamParser::Result res = BusyParamParser::parse( m_ui->parametersTextEdit->toPlainText() );
    if( !res.hasErrors() )
    {
        m_ui->parametersTextEdit->setToolTip(QString());
        m_ui->parametersTextEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    }else
    {
        QTextEdit::ExtraSelection e;
        QTextCharFormat error;
        error.setBackground(Qt::red);

        QTextCursor c( m_ui->parametersTextEdit->document()->findBlockByNumber( res.d_row - 1 ) );
        c.setPosition( c.position() + res.d_col - 2 );
        c.setPosition( c.position() + res.d_len + 1, QTextCursor::KeepAnchor);

        e.cursor = c;
        e.format = error;
        m_ui->parametersTextEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>() << e);
        m_ui->parametersTextEdit->setToolTip(res.d_err);
    }
    QVariantMap data = m_step->busyConfiguration();
    data[Constants::BUSY_CONFIG_PARAMS_KEY] = m_ui->parametersTextEdit->toPlainText();
    m_ignoreChange = true;
    m_step->setBusyConfiguration(data);
    m_ignoreChange = false;
}

bool BusyBuildStepConfigWidget::validateTargets(Utils::FancyLineEdit *edit, QString *errorMessage)
{
    BusyTargetParser::Result res = BusyTargetParser::parse(edit->text());
    if( errorMessage && res.hasErrors() )
        *errorMessage = res.d_err;

    QVariantMap data = m_step->busyConfiguration();
    data[Constants::BUSY_CONFIG_TARGETS_KEY] = edit->text();
    m_ignoreChange = true;
    m_step->setBusyConfiguration(data);
    m_ignoreChange = false;

    return !res.hasErrors();
}

// --------------------------------------------------------------------
// BusyBuildStepFactory:
// --------------------------------------------------------------------

BusyBuildStepFactory::BusyBuildStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<Core::Id> BusyBuildStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && qobject_cast<BusyBuildConfiguration *>(parent->parent())
            && qobject_cast<BusyProject *>(parent->target()->project()))
        return QList<Core::Id>() << Core::Id(Constants::BUSY_BUILDSTEP_ID);
    return QList<Core::Id>();
}

QString BusyBuildStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::BUSY_BUILDSTEP_ID))
        return tr("Busy Build");
    return QString();
}

bool BusyBuildStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            || !qobject_cast<BusyBuildConfiguration *>(parent->parent())
            || !qobject_cast<BusyProject *>(parent->target()->project()))
        return false;
    return id == Core::Id(Constants::BUSY_BUILDSTEP_ID);
}

ProjectExplorer::BuildStep *BusyBuildStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new BusyBuildStep(parent);
}

bool BusyBuildStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *BusyBuildStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    BusyBuildStep *bs = new BusyBuildStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool BusyBuildStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *BusyBuildStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new BusyBuildStep(parent, static_cast<BusyBuildStep *>(product));
}

} // namespace Internal
} // namespace BusyProjectManager
