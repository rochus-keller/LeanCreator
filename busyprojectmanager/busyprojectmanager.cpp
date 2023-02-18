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

#include "busyprojectmanager.h"

#include "defaultpropertyprovider.h"
#include "busylogsink.h"
#include "busyproject.h"
#include "busyprojectmanagerconstants.h"
#include "busyprojectmanagerplugin.h"

#include <core/icore.h>
#include <core/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#if 0
#include <qmljstools/qmljstoolsconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtprofilesetup.h>
#endif

#include <QCryptographicHash>
#include <QVariantMap>

#include <busytools/busyapi.h>

const QChar sep = QLatin1Char('.');

static QString qtcProfileGroup() { return QLatin1String("preferences.qtcreator.kit"); }
static QString qtcProfilePrefix() { return qtcProfileGroup() + sep; }

namespace BusyProjectManager {
namespace Internal {

busy::Settings *BusyManager::m_settings = 0;
Internal::BusyLogSink *BusyManager::m_logSink = 0;
BusyManager *BusyManager::m_instance = 0;

BusyManager::BusyManager() :
    m_defaultPropertyProvider(new DefaultPropertyProvider)
{
    m_settings = new busy::Settings(Core::ICore::userResourcePath());
    m_instance = this;

    setObjectName(QLatin1String("BusyProjectManager"));
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsLoaded, this,
            [this]() { m_kitsToBeSetupForBusy = ProjectExplorer::KitManager::kits(); } );
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitAdded, this,
            &BusyManager::addProfileFromKit);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitUpdated, this,
            &BusyManager::handleKitUpdate);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitRemoved, this,
            &BusyManager::handleKitRemoval);

    m_logSink = new BusyLogSink(this);
    int level = busy::LoggerWarning;
    const QString levelEnv = QString::fromLocal8Bit(qgetenv("BUSY_LOG_LEVEL"));
    if (!levelEnv.isEmpty()) {
        bool ok = false;
        int tmp = levelEnv.toInt(&ok);
        if (ok) {
            if (tmp < static_cast<int>(busy::LoggerMinLevel))
                tmp = static_cast<int>(busy::LoggerMinLevel);
            if (tmp > static_cast<int>(busy::LoggerMaxLevel))
                tmp = static_cast<int>(busy::LoggerMaxLevel);
            level = tmp;
        }
    }
    m_logSink->setLogLevel(static_cast<busy::LoggerLevel>(level));
}

BusyManager::~BusyManager()
{
    delete m_defaultPropertyProvider;
    delete m_settings;
    m_instance = 0;
}

QString BusyManager::mimeType() const
{
    return QLatin1String(BusyProjectManager::Constants::MIME_TYPE);
}

ProjectExplorer::Project *BusyManager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file.")
                .arg(fileName);
        return 0;
    }

    return new BusyProject(this, fileName);
}

QString BusyManager::profileForKit(const ProjectExplorer::Kit *k)
{
    if (!k)
        return QString();
    updateProfileIfNecessary(k);
    return m_settings->value(qtcProfilePrefix() + k->id().toString()).toString();
}

void BusyManager::setProfileForKit(const QString &name, const ProjectExplorer::Kit *k)
{
    m_settings->setValue(qtcProfilePrefix() + k->id().toString(), name);
}

void BusyManager::updateProfileIfNecessary(const ProjectExplorer::Kit *kit)
{
    // kit in list <=> profile update is necessary
    // Note that the const_cast is safe, as we do not call any non-const methods on the object.
    if (m_kitsToBeSetupForBusy.removeOne(const_cast<ProjectExplorer::Kit *>(kit)))
        addProfileFromKit(kit);
}

void BusyManager::addProfile(const QString &name, const QVariantMap &data)
{
    busy::Profile profile(name, settings());
    const QVariantMap::ConstIterator cend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != cend; ++it)
        profile.setValue(it.key(), it.value());
}

void BusyManager::addProfileFromKit(const ProjectExplorer::Kit *k)
{
    const QString name = QString::fromLatin1("qtc_%1_%2").arg(k->fileSystemFriendlyName().left(8),
            QString::fromLatin1(QCryptographicHash::hash(k->id().name(),
                                                         QCryptographicHash::Sha1).toHex().left(8)));
    busy::Profile(name, settings()).removeProfile();
    setProfileForKit(name, k);
    //addQtProfileFromKit(name, k);

    // set up properties:
    QVariantMap data = m_defaultPropertyProvider->properties(k, QVariantMap());
    QList<PropertyProvider *> providerList = ExtensionSystem::PluginManager::getObjects<PropertyProvider>();
    foreach (PropertyProvider *provider, providerList) {
        if (provider->canHandle(k))
            data = provider->properties(k, data);
    }

    addProfile(name, data);
}

void BusyManager::handleKitUpdate(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForBusy.removeOne(kit);
    addProfileFromKit(kit);
}

void BusyManager::handleKitRemoval(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForBusy.removeOne(kit);
    const QString key = qtcProfilePrefix() + kit->id().toString();
    const QString profileName = m_settings->value(key).toString();
    m_settings->remove(key);
    busy::Profile(profileName, m_settings).removeProfile();
}

} // namespace Internal
} // namespace BusyProjectManager
