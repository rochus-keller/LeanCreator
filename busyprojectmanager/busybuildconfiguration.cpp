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

#include "busybuildconfiguration.h"

#include "busybuildconfigurationwidget.h"
#include "busybuildstep.h"
#include "busycleanstep.h"
#include "busyinstallstep.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"

#include <core/documentmanager.h>
#include <core/icore.h>
#include <utils/qtcassert.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>

#include <QCoreApplication>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace BusyProjectManager {
namespace Internal {

const char BUSY_BC_ID[] = "Busy.BusyBuildConfiguration";

// ---------------------------------------------------------------------------
// BusyBuildConfiguration:
// ---------------------------------------------------------------------------

BusyBuildConfiguration::BusyBuildConfiguration(Target *target) :
    BuildConfiguration(target, Core::Id(BUSY_BC_ID)),
    m_isParsing(true),
    m_parsingError(false)
{
    connect(project(), &BusyProject::projectParsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(project(), &BusyProject::projectParsingDone, this, &BuildConfiguration::enabledChanged);

    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    connect(bsl, &BuildStepList::stepInserted, this, &BusyBuildConfiguration::buildStepInserted);
}

BusyBuildConfiguration::BusyBuildConfiguration(Target *target, Core::Id id) :
    BuildConfiguration(target, id)
{ }

BusyBuildConfiguration::BusyBuildConfiguration(Target *target, BusyBuildConfiguration *source) :
    BuildConfiguration(target, source)
{
    cloneSteps(source);
}

bool BusyBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    // Fix up the existing build steps:
    for (int i = 0; i < bsl->count(); ++i) {
        BusyBuildStep *bs = qobject_cast<BusyBuildStep *>(bsl->at(i));
        if (bs)
            connect(bs, &BusyBuildStep::busyConfigurationChanged, this, &BusyBuildConfiguration::busyConfigurationChanged);
    }

    return true;
}

void BusyBuildConfiguration::buildStepInserted(int pos)
{
    BusyBuildStep *step = qobject_cast<BusyBuildStep *>(stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->at(pos));
    if (step) {
        connect(step, &BusyBuildStep::busyConfigurationChanged, this, &BusyBuildConfiguration::busyConfigurationChanged);
        emit busyConfigurationChanged();
    }
}

NamedWidget *BusyBuildConfiguration::createConfigWidget()
{
    return new BusyBuildConfigurationWidget(this);
}

BusyBuildStep *BusyBuildConfiguration::busyStep() const
{
    foreach (BuildStep *bs, stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->steps()) {
        if (BusyBuildStep *qbsBs = qobject_cast<BusyBuildStep *>(bs))
            return qbsBs;
    }
    return 0;
}

QVariantMap BusyBuildConfiguration::busyConfiguration() const
{
    QVariantMap config;
    BusyBuildStep *qbsBs = busyStep();
    if (qbsBs)
        config = qbsBs->busyConfiguration();
    return config;
}

Internal::BusyProject *BusyBuildConfiguration::project() const
{
    return qobject_cast<Internal::BusyProject *>(target()->project());
}

IOutputParser *BusyBuildConfiguration::createOutputParser() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    return tc ? tc->outputParser() : 0;
}

bool BusyBuildConfiguration::isEnabled() const
{
    return !project()->isParsing() && project()->hasParseResult();
}

QString BusyBuildConfiguration::disabledReason() const
{
    if (project()->isParsing())
        return tr("Parsing the Busy project.");
    if (!project()->hasParseResult())
        return tr("Parsing of Busy project has failed.");
    return QString();
}

BuildConfiguration::BuildType BusyBuildConfiguration::buildType() const
{
    QString variant;
    if (busyStep())
        variant = busyStep()->buildVariant();

    if (variant == QLatin1String(Constants::BUSY_VARIANT_DEBUG))
        return Debug;
    if (variant == QLatin1String(Constants::BUSY_VARIANT_RELEASE))
        return Release;
    return Unknown;
}

void BusyBuildConfiguration::setChangedFiles(const QStringList &files)
{
    m_changedFiles = files;
}

QStringList BusyBuildConfiguration::changedFiles() const
{
    return m_changedFiles;
}

void BusyBuildConfiguration::setActiveFileTags(const QStringList &fileTags)
{
    m_activeFileTags = fileTags;
}

QStringList BusyBuildConfiguration::activeFileTags() const
{
    return m_activeFileTags;
}

void BusyBuildConfiguration::setProducts(const QStringList &products)
{
    m_products = products;
}

QStringList BusyBuildConfiguration::products() const
{
    return m_products;
}

void BusyBuildConfiguration::emitBuildTypeChanged()
{
    emit buildTypeChanged();
}

class StepProxy
{
public:
    StepProxy(const BuildStep *buildStep)
        : m_qbsBuildStep(qobject_cast<const BusyBuildStep *>(buildStep))
        , m_qbsCleanStep(qobject_cast<const BusyCleanStep *>(buildStep))
        , m_qbsInstallStep(qobject_cast<const BusyInstallStep *>(buildStep))
    {
    }

    QString command() const {
        if (m_qbsBuildStep)
            return QLatin1String("build");
        if (m_qbsCleanStep)
            return QLatin1String("clean");
        return QLatin1String("install");
    }

    bool dryRun() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->stopOnError();
        if (m_qbsCleanStep)
            return m_qbsCleanStep->dryRun();
        return m_qbsInstallStep->dryRun();
    }

    bool keepGoing() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->trackHeaders();
        if (m_qbsCleanStep)
            return m_qbsCleanStep->keepGoing();
        return m_qbsInstallStep->keepGoing();
    }

    bool showCommandLines() const {
        return m_qbsBuildStep ? m_qbsBuildStep->showCommandLines() : false;
    }

    bool noInstall() const {
        return m_qbsBuildStep ? !m_qbsBuildStep->install() : false;
    }

    bool cleanInstallRoot() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->cleanInstallRoot();
        if (m_qbsInstallStep)
            return m_qbsInstallStep->removeFirst();
        return false;
    }

    int jobCount() const {
        return m_qbsBuildStep ? m_qbsBuildStep->maxJobs() : 0;
    }

    bool allArtifacts() const {
        return m_qbsCleanStep ? m_qbsCleanStep->cleanAll() : false;
    }

    QString installRoot() const {
        return m_qbsInstallStep ? m_qbsInstallStep->absoluteInstallRoot() : QString();
    }

private:
    const BusyBuildStep * const m_qbsBuildStep;
    const BusyCleanStep * const m_qbsCleanStep;
    const BusyInstallStep * const m_qbsInstallStep;
};

QString BusyBuildConfiguration::equivalentCommandLine(const BuildStep *buildStep)
{
    QString commandLine;

    const QString qbsFilePath = Utils::HostOsInfo::withExecutableSuffix("lua");
    Utils::QtcProcess::addArg(&commandLine, QDir::toNativeSeparators(qbsFilePath));
    Utils::QtcProcess::addArg(&commandLine, "build.lua");
    commandLine += " ...";
#if 0
    const StepProxy stepProxy(buildStep);
    Utils::QtcProcess::addArg(&commandLine, stepProxy.command());
    const BusyBuildConfiguration * const buildConfig = qobject_cast<BusyBuildConfiguration *>(
                buildStep->project()->activeTarget()->activeBuildConfiguration());
    if (buildConfig) {
        const QString buildDir = buildConfig->buildDirectory().toUserOutput();
        Utils::QtcProcess::addArgs(&commandLine, QStringList() << QLatin1String("-d") << buildDir);
    }
    Utils::QtcProcess::addArgs(&commandLine, QStringList() << QLatin1String("-f")
                               << buildStep->project()->projectFilePath().toUserOutput());
    Utils::QtcProcess::addArgs(&commandLine, QStringList() << QLatin1String("--settings-dir")
                               << QDir::toNativeSeparators(Core::ICore::userResourcePath()));
    if (stepProxy.dryRun())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--dry-run"));
    if (stepProxy.keepGoing())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--keep-going"));
    if (stepProxy.showCommandLines())
        Utils::QtcProcess::addArgs(&commandLine, QStringList()
                                   << QLatin1String("--command-echo-mode")
                                   << QLatin1String("command-line"));
    if (stepProxy.noInstall())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--no-install"));
    if (stepProxy.cleanInstallRoot())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--clean-install-root"));
    const int jobCount = stepProxy.jobCount();
    if (jobCount > 0) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList() << QLatin1String("--jobs")
                                   << QString::number(jobCount));
    }
    if (stepProxy.allArtifacts())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--all-artifacts"));
    const QString installRoot = stepProxy.installRoot();
    if (!installRoot.isEmpty()) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList() << QLatin1String("--install-root")
                                   << installRoot);
    }
#endif

    return commandLine;
}

BusyBuildConfiguration *BusyBuildConfiguration::setup(Target *t,
                                                    const QString &defaultDisplayName,
                                                    const QString &displayName,
                                                    const QVariantMap &buildData,
                                                    const Utils::FileName &directory)
{
    // Add the build configuration.
    BusyBuildConfiguration *bc = new BusyBuildConfiguration(t);
    bc->setDefaultDisplayName(defaultDisplayName);
    bc->setDisplayName(displayName);
    bc->setBuildDirectory(directory);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    BusyBuildStep *bs = new BusyBuildStep(buildSteps);
    bs->setBusyConfiguration(buildData);
    buildSteps->insertStep(0, bs);

    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    BusyCleanStep *cs = new BusyCleanStep(cleanSteps);
    cleanSteps->insertStep(0, cs);

    return bc;
}

// ---------------------------------------------------------------------------
// BusyBuildConfigurationFactory:
// ---------------------------------------------------------------------------

BusyBuildConfigurationFactory::BusyBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{ }

BusyBuildConfigurationFactory::~BusyBuildConfigurationFactory()
{ }

bool BusyBuildConfigurationFactory::canHandle(const Target *t) const
{
    return qobject_cast<Internal::BusyProject *>(t->project());
}

BuildInfo *BusyBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                         BuildConfiguration::BuildType type) const
{
    auto info = new ProjectExplorer::BuildInfo(this);
    info->typeName = tr("Build");
    info->kitId = k->id();
    info->buildType = type;
    return info;
}

int BusyBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> BusyBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;

    BuildInfo *info = createBuildInfo(parent->kit(), BuildConfiguration::Debug);
    result << info;

    return result;
}

int BusyBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::PROJECT_MIME_TYPE)))
        return 0;
    return -1;
}

static Utils::FileName defaultBuildDirectory(const QString &projectFilePath, const Kit *k,
                                             const QString &bcName,
                                             BuildConfiguration::BuildType buildType)
{
    QString projectName = QFileInfo(projectFilePath).absoluteDir().dirName();
    if( projectName.isEmpty() )
        projectName = "LeanCreatorBuild";
    ProjectMacroExpander expander(projectName, k, bcName, buildType);
    QString projectDir = Project::projectDirectory(Utils::FileName::fromString(projectFilePath)).toString();
    QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return Utils::FileName::fromString(Utils::FileUtils::resolvePath(projectDir, buildPath));
}

QList<BuildInfo *> BusyBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;

    BuildInfo *info = createBuildInfo(k, BuildConfiguration::Debug);
    //: The name of the debug build configuration created by default for a qbs project.
    info->displayName = tr("Debug");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Debug", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    info = createBuildInfo(k, BuildConfiguration::Release);
    //: The name of the release build configuration created by default for a qbs project.
    info->displayName = tr("Release");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Release", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    return result;
}

BuildConfiguration *BusyBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    QVariantMap configData;
    configData.insert(QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY),
                      (info->buildType == BuildConfiguration::Debug)
                          ? QLatin1String(Constants::BUSY_VARIANT_DEBUG)
                          : QLatin1String(Constants::BUSY_VARIANT_RELEASE));

    Utils::FileName buildDir = info->buildDirectory;
    if (buildDir.isEmpty())
        buildDir = defaultBuildDirectory(parent->project()->projectDirectory().toString(),
                                         parent->kit(), info->displayName, info->buildType);

    BuildConfiguration *bc
            = BusyBuildConfiguration::setup(parent, info->displayName, info->displayName,
                                           configData, buildDir);

    return bc;
}

bool BusyBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    return canHandle(parent) && qobject_cast<BusyBuildConfiguration *>(source);
}

BuildConfiguration *BusyBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    BusyBuildConfiguration *oldbc(static_cast<BusyBuildConfiguration *>(source));
    return new BusyBuildConfiguration(parent, oldbc);
}

bool BusyBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Core::Id(BUSY_BC_ID);
}

BuildConfiguration *BusyBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    BusyBuildConfiguration *bc = new BusyBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

} // namespace Internal
} // namespace BusyProjectManager
