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

#include "busyprojectparser.h"

#include "busylogsink.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"

#include <core/icore.h>
#include <utils/qtcassert.h>

#include <busy/busyapi.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

using namespace Utils;

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BusyProjectParser:
// --------------------------------------------------------------------

BusyProjectParser::BusyProjectParser(BusyProject *project, QFutureInterface<bool> *fi) :
    m_qbsSetupProjectJob(0),
    m_fi(fi),
    m_currentProgressBase(0)
{
    m_project = project->busyProject();
    m_projectFilePath = project->projectFilePath().toString();
}

BusyProjectParser::~BusyProjectParser()
{
    if (m_qbsSetupProjectJob) {
        m_qbsSetupProjectJob->disconnect(this);
        m_qbsSetupProjectJob->cancel();
        m_qbsSetupProjectJob->deleteLater();
        m_qbsSetupProjectJob = 0;
    }
    m_fi = 0; // we do not own m_fi, do not delete
}

void BusyProjectParser::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    QTC_ASSERT(!m_qbsSetupProjectJob, return);
    QTC_ASSERT(!dir.isEmpty(), return);

    m_currentProgressBase = 0;

    busy::SetupProjectParameters params;
    QVariantMap userConfig = config;
    QString specialKey = QLatin1String(Constants::BUSY_CONFIG_PROFILE_KEY);
    const QString profileName = userConfig.take(specialKey).toString();
    params.setTopLevelProfile(profileName);
    specialKey = QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY);
    params.setBuildVariant(userConfig.take(specialKey).toString());
    params.setSettingsDirectory(BusyManager::settings()->baseDirectoy());
    params.setOverriddenValues(userConfig);

    // Some people don't like it when files are created as a side effect of opening a project,
    // so do not store the build graph if the build directory does not exist yet.
    params.setDryRun(!QFileInfo::exists(dir));

    params.setBuildRoot(dir);
    params.setProjectFilePath(m_projectFilePath);
    params.setIgnoreDifferentProjectFilePath(false);
    params.setEnvironment(env.toProcessEnvironment());
    const busy::Preferences prefs(BusyManager::settings(), profileName);
    params.setSearchPaths(prefs.searchPaths(resourcesBaseDirectory()));
    params.setPluginPaths(prefs.pluginPaths(pluginsBaseDirectory()));
    params.setLibexecPath(libExecDirectory());

    m_qbsSetupProjectJob = m_project.setupProject(params, BusyManager::logSink(), 0);

    connect(m_qbsSetupProjectJob, SIGNAL(finished(bool,busy::AbstractJob*)),
            this, SLOT(handleBusyParsingDone(bool)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskStarted(QString,int,busy::AbstractJob*)),
            this, SLOT(handleBusyParsingTaskSetup(QString,int)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskProgress(int,busy::AbstractJob*)),
            this, SLOT(handleBusyParsingProgress(int)));
}

void BusyProjectParser::cancel()
{
    QTC_ASSERT(m_qbsSetupProjectJob, return);
    m_qbsSetupProjectJob->cancel();
}

busy::Project BusyProjectParser::busyProject() const
{
    return m_project;
}

busy::ErrorInfo BusyProjectParser::error()
{
    return m_error;
}

void BusyProjectParser::handleBusyParsingDone(bool success)
{
    QTC_ASSERT(m_qbsSetupProjectJob, return);

    m_project = m_qbsSetupProjectJob->project();
    m_error = m_qbsSetupProjectJob->error();

    // Do not report the operation as canceled here, as we might want to make overlapping
    // parses appear atomic to the user.

    emit done(success);
}

void BusyProjectParser::handleBusyParsingProgress(int progress)
{
    if (m_fi)
        m_fi->setProgressValue(m_currentProgressBase + progress);
}

void BusyProjectParser::handleBusyParsingTaskSetup(const QString &description, int maximumProgressValue)
{
    Q_UNUSED(description);
    if (m_fi) {
        m_currentProgressBase = m_fi->progressValue();
        m_fi->setProgressRange(0, m_currentProgressBase + maximumProgressValue);
    }
}

QString BusyProjectParser::resourcesBaseDirectory() const
{
    const QString qbsInstallDir = QLatin1String(BUSY_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir;
    return Core::ICore::resourcePath() + QLatin1String("/qbs");
}

QString BusyProjectParser::libExecDirectory() const
{
    const QString qbsInstallDir = QLatin1String(BUSY_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir + QLatin1String("/libexec");
    return Core::ICore::libexecPath();
}

#define IDE_LIBRARY_BASENAME "NOTUSED"

QString BusyProjectParser::pluginsBaseDirectory() const
{
    const QString qbsInstallDir = QLatin1String(BUSY_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir + QLatin1String("/lib/");
    if (HostOsInfo::isMacHost())
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../PlugIns"));
    else
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator/plugins"));
}

} // namespace Internal
} // namespace BusyProjectManager
