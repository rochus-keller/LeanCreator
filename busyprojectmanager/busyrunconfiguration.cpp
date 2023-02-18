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

#include "busyrunconfiguration.h"

#include "busydeployconfigurationfactory.h"
#include "busyinstallstep.h"
#include "busyproject.h"

#include <core/coreconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/kitinformation.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>
#include <utils/outputformatter.h>
#if 0
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#endif
#include <utils/hostosinfo.h>

//#include "api/runenvironment.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QDir>

using namespace ProjectExplorer;

namespace BusyProjectManager {
namespace Internal {

const char BUSY_RC_PREFIX[] = "Busy.RunConfiguration:";

static QString rcNameSeparator() { return QLatin1String("---Busy.RC.NameSeparator---"); }

static Core::Id idFromProduct(const BusyProject *project, const busy::Product &product)
{
    QString id = QLatin1String(BUSY_RC_PREFIX);
    id.append(BusyProject::uniqueProductName(product)).append(rcNameSeparator())
            .append(BusyProject::productDisplayName(project->busyProject(), product));
    return Core::Id::fromString(id);
}

static QString uniqueProductNameFromId(Core::Id id)
{
    const QString suffix = id.suffixAfter(BUSY_RC_PREFIX);
    return suffix.left(suffix.indexOf(rcNameSeparator()));
}

static QString productDisplayNameFromId(Core::Id id)
{
    const QString suffix = id.suffixAfter(BUSY_RC_PREFIX);
    const int sepPos = suffix.indexOf(rcNameSeparator());
    if (sepPos == -1)
        return suffix;
    return suffix.mid(sepPos + rcNameSeparator().count());
}

const busy::Product findProduct(const busy::Project &pro, const QString &uniqeName)
{
    foreach (const busy::Product &product, pro.allProducts()) {
        if (BusyProject::uniqueProductName(product) == uniqeName)
            return product;
    }
    return busy::Product();
}

// --------------------------------------------------------------------
// BusyRunConfiguration:
// --------------------------------------------------------------------

BusyRunConfiguration::BusyRunConfiguration(Target *parent, Core::Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_uniqueProductName(uniqueProductNameFromId(id)),
    m_currentInstallStep(0),
    m_currentBuildStepList(0)
{
    addExtraAspect(new LocalEnvironmentAspect(this));
    addExtraAspect(new ArgumentsAspect(this, QStringLiteral("Busy.RunConfiguration.CommandLineArguments")));
    addExtraAspect(new WorkingDirectoryAspect(this, QStringLiteral("Busy.RunConfiguration.WorkingDirectory")));

    addExtraAspect(new TerminalAspect(this,
                                      QStringLiteral("Busy.RunConfiguration.UseTerminal"),
                                      isConsoleApplication()));

    ctor();
}

BusyRunConfiguration::BusyRunConfiguration(Target *parent, BusyRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_uniqueProductName(source->m_uniqueProductName),
    m_currentInstallStep(0), // no need to copy this, we will get if from the DC anyway.
    m_currentBuildStepList(0) // ditto
{
    ctor();
}

bool BusyRunConfiguration::isEnabled() const
{
    BusyProject *project = static_cast<BusyProject *>(target()->project());
    return !project->isParsing() && project->hasParseResult();
}

QString BusyRunConfiguration::disabledReason() const
{
    BusyProject *project = static_cast<BusyProject *>(target()->project());
    if (project->isParsing())
        return tr("The BUSY files are currently being parsed.");

    if (!project->hasParseResult())
        return tr("Parsing of BUSY files has failed.");
    return QString();
}

void BusyRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    BusyProject *project = static_cast<BusyProject *>(target()->project());
    connect(project, &BusyProject::projectParsingStarted, this, &RunConfiguration::enabledChanged);
    connect(project, &BusyProject::projectParsingDone, this, [this](bool success) {
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (success && !terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
        emit enabledChanged();
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this,
            [this, project](Project *p) {
                if (p == project && !BuildManager::isBuilding(p))
                    emit enabledChanged();
            }
    );

    connect(target(), &Target::activeDeployConfigurationChanged,
            this, &BusyRunConfiguration::installStepChanged);
    installStepChanged();
}

QWidget *BusyRunConfiguration::createConfigurationWidget()
{
    return new BusyRunConfigurationWidget(this, 0);
}

void BusyRunConfiguration::installStepChanged()
{
    if (m_currentInstallStep)
        disconnect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));
    if (m_currentBuildStepList) {
        disconnect(m_currentBuildStepList, SIGNAL(stepInserted(int)), this, SLOT(installStepChanged()));
        disconnect(m_currentBuildStepList, SIGNAL(stepRemoved(int)), this, SLOT(installStepChanged()));
        disconnect(m_currentBuildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(installStepChanged()));
    }

    BusyDeployConfiguration *activeDc = qobject_cast<BusyDeployConfiguration *>(target()->activeDeployConfiguration());
    m_currentBuildStepList = activeDc ? activeDc->stepList() : 0;
    m_currentInstallStep = activeDc ? activeDc->busyInstallStep() : 0;

    if (m_currentInstallStep)
        connect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));

    if (m_currentBuildStepList) {
        connect(m_currentBuildStepList, SIGNAL(stepInserted(int)), this, SLOT(installStepChanged()));
        connect(m_currentBuildStepList, SIGNAL(aboutToRemoveStep(int)), this,
                SLOT(installStepToBeRemoved(int)));
        connect(m_currentBuildStepList, SIGNAL(stepRemoved(int)), this, SLOT(installStepChanged()));
        connect(m_currentBuildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(installStepChanged()));
    }

    emit targetInformationChanged();
}

void BusyRunConfiguration::installStepToBeRemoved(int pos)
{
    QTC_ASSERT(m_currentBuildStepList, return);
    // TODO: Our logic is rather broken. Users can create as many qbs install steps as they want,
    // but we ignore all but the first one.
    if (m_currentBuildStepList->steps().at(pos) != m_currentInstallStep)
        return;
    disconnect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));
    m_currentInstallStep = 0;
}

QString BusyRunConfiguration::executable() const
{
    BusyProject *pro = static_cast<BusyProject *>(target()->project());
    const busy::Product product = findProduct(pro->busyProject(), m_uniqueProductName);

    if (!product.isValid() || !pro->busyProject().isValid())
        return QString();

    return pro->busyProject().targetExecutable(product, installOptions());
}

ApplicationLauncher::Mode BusyRunConfiguration::runMode() const
{
    return extraAspect<TerminalAspect>()->runMode();
}

bool BusyRunConfiguration::isConsoleApplication() const
{
    BusyProject *pro = static_cast<BusyProject *>(target()->project());
    const busy::Product product = findProduct(pro->busyProject(), m_uniqueProductName);
    return product.properties().value(QLatin1String("consoleApplication"), false).toBool();
}

QString BusyRunConfiguration::workingDirectory() const
{
    const auto *wdAspect = extraAspect<WorkingDirectoryAspect>();
    QTC_ASSERT(wdAspect, return baseWorkingDirectory());
    return wdAspect->workingDirectory();
}

QString BusyRunConfiguration::baseWorkingDirectory() const
{
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(executable()).absolutePath();
    return QString();
}

QString BusyRunConfiguration::commandLineArguments() const
{
    return extraAspect<ArgumentsAspect>()->arguments();
}

void BusyRunConfiguration::setRunMode(ApplicationLauncher::Mode runMode)
{
    extraAspect<TerminalAspect>()->setRunMode(runMode);
}

void BusyRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    BusyProject *project = static_cast<BusyProject *>(target()->project());
    if (project) {
        const busy::Product product = findProduct(project->busyProject(), m_uniqueProductName);
        if (product.isValid()) {
            QProcessEnvironment procEnv = env.toProcessEnvironment();
            procEnv.insert(QLatin1String("BUSY_RUN_FILE_PATH"), executable());
            procEnv = project->busyProject().getRunEnvironment(product, installOptions(),
                    procEnv, BusyManager::settings());
            if (!procEnv.isEmpty()) {
                env = Utils::Environment();
                foreach (const QString &key, procEnv.keys())
                    env.set(key, procEnv.value(key));
            }
        }
    }

#if 0
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qtVersion)
        env.prependOrSetLibrarySearchPath(qtVersion->qmakeProperty("QT_INSTALL_LIBS"));
#endif
}

QString BusyRunConfiguration::uniqueProductName() const
{
    return m_uniqueProductName;
}

QString BusyRunConfiguration::defaultDisplayName()
{
    QString defaultName = productDisplayNameFromId(id());
    if (defaultName.isEmpty())
        defaultName = tr("Busy Run Configuration");
    return defaultName;
}

busy::InstallOptions BusyRunConfiguration::installOptions() const
{
    if (m_currentInstallStep)
        return m_currentInstallStep->installOptions();
    return busy::InstallOptions();
}

QString BusyRunConfiguration::installRoot() const
{
    if (m_currentInstallStep)
        return m_currentInstallStep->absoluteInstallRoot();
    return QString();
}

Utils::OutputFormatter *BusyRunConfiguration::createOutputFormatter() const
{
    // return new QtSupport::QtOutputFormatter(target()->project());
    return new Utils::OutputFormatter();
}

// --------------------------------------------------------------------
// BusyRunConfigurationWidget:
// --------------------------------------------------------------------

BusyRunConfigurationWidget::BusyRunConfigurationWidget(BusyRunConfiguration *rc, QWidget *parent)
    : QWidget(parent),
    m_rc(rc),
    m_ignoreChange(false),
    m_isShown(false)
{
    QVBoxLayout *vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    QHBoxLayout *hl = new QHBoxLayout();
    hl->addStretch();
    m_disabledIcon = new QLabel(this);
    m_disabledIcon->setPixmap(QPixmap(QLatin1String(Core::Constants::ICON_WARNING)));
    hl->addWidget(m_disabledIcon);
    m_disabledReason = new QLabel(this);
    m_disabledReason->setVisible(false);
    hl->addWidget(m_disabledReason);
    hl->addStretch();
    vboxTopLayout->addLayout(hl);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(m_detailsContainer);
    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    QFormLayout *toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    setExecutableLineText();
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    m_rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);

    m_rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

    runConfigurationEnabledChange();

    connect(m_rc, SIGNAL(targetInformationChanged()),
            this, SLOT(targetInformationHasChanged()), Qt::QueuedConnection);

    connect(m_rc, SIGNAL(enabledChanged()),
            this, SLOT(runConfigurationEnabledChange()));
}

void BusyRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_rc->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_rc->disabledReason());

    targetInformationHasChanged();
}

void BusyRunConfigurationWidget::targetInformationHasChanged()
{
    m_ignoreChange = true;
    setExecutableLineText(m_rc->executable());

    WorkingDirectoryAspect *aspect = m_rc->extraAspect<WorkingDirectoryAspect>();
    aspect->setDefaultWorkingDirectory(m_rc->baseWorkingDirectory());
    aspect->pathChooser()->setBaseFileName(m_rc->target()->project()->projectDirectory());
    m_ignoreChange = false;
}

void BusyRunConfigurationWidget::setExecutableLineText(const QString &text)
{
    const QString newText = text.isEmpty() ? tr("<unknown>") : text;
    m_executableLineLabel->setText(newText);
}

// --------------------------------------------------------------------
// BusyRunConfigurationFactory:
// --------------------------------------------------------------------

BusyRunConfigurationFactory::BusyRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("BusyRunConfigurationFactory"));
}

BusyRunConfigurationFactory::~BusyRunConfigurationFactory()
{ }

bool BusyRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    BusyProject *project = static_cast<BusyProject *>(parent->project());
    return findProduct(project->busyProject(), uniqueProductNameFromId(id)).isValid();
}

RunConfiguration *BusyRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return new BusyRunConfiguration(parent, id);
}

bool BusyRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).toString().startsWith(QLatin1String(BUSY_RC_PREFIX));
}

RunConfiguration *BusyRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    return new BusyRunConfiguration(parent, idFromMap(map));
}

bool BusyRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *BusyRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    BusyRunConfiguration *old = static_cast<BusyRunConfiguration *>(source);
    return new BusyRunConfiguration(parent, old);
}

QList<Core::Id> BusyRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    BusyProject *project = static_cast<BusyProject *>(parent->project());
    if (!project || !project->busyModule().isValid())
        return result;

    foreach (const busy::Product &product, project->busyProject().allProducts(true, true)) {
        result << idFromProduct(project, product);
    }

    return result;
}

QString BusyRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return productDisplayNameFromId(id);
}

bool BusyRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<BusyProject *>(t->project()))
        return false;
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(t->kit());
    return devType == Constants::DESKTOP_DEVICE_TYPE;
}

} // namespace Internal
} // namespace BusyProjectManager
