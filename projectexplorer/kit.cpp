/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "kit.h"

#include "kitmanager.h"
#include "ioutputparser.h"
#include "osparser.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QStyle>
#include <QTextStream>
#include <QUuid>

using namespace Core;
using namespace Utils;

const char ID_KEY[] = "PE.Profile.Id";
const char DISPLAYNAME_KEY[] = "PE.Profile.Name";
const char FILESYSTEMFRIENDLYNAME_KEY[] = "PE.Profile.FileSystemFriendlyName";
const char AUTODETECTED_KEY[] = "PE.Profile.AutoDetected";
const char AUTODETECTIONSOURCE_KEY[] = "PE.Profile.AutoDetectionSource";
const char SDK_PROVIDED_KEY[] = "PE.Profile.SDK";
const char DATA_KEY[] = "PE.Profile.Data";
const char ICON_KEY[] = "PE.Profile.Icon";
const char MUTABLE_INFO_KEY[] = "PE.Profile.MutableInfo";
const char STICKY_INFO_KEY[] = "PE.Profile.StickyInfo";

namespace ProjectExplorer {
namespace Internal {

// -------------------------------------------------------------------------
// KitPrivate
// -------------------------------------------------------------------------


class KitPrivate
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Kit)

public:
    KitPrivate(Id id, Kit *kit) :
        m_id(id),
        m_nestedBlockingLevel(0),
        m_autodetected(false),
        m_sdkProvided(false),
        m_isValid(true),
        m_hasWarning(false),
        m_hasValidityInfo(false),
        m_mustNotify(false)
    {
        if (!id.isValid())
            m_id = Id::fromString(QUuid::createUuid().toString());

        m_unexpandedDisplayName = QCoreApplication::translate("ProjectExplorer::Kit", "Unnamed");
        m_iconPath = FileName::fromLatin1(":///DESKTOP///");

        m_macroExpander.setDisplayName(tr("Kit"));
        m_macroExpander.setAccumulating(true);
        m_macroExpander.registerVariable("Kit:Id", tr("Kit ID"),
            [kit] { return kit->id().toString(); });
        m_macroExpander.registerVariable("Kit:FileSystemName", tr("Kit filesystem-friendly name"),
            [kit] { return kit->fileSystemFriendlyName(); });
        foreach (KitInformation *ki, KitManager::kitInformation())
            ki->addToMacroExpander(kit, &m_macroExpander);

        // This provides the same global fall back as the global expander
        // without relying on the currentKit() discovery process there.
        m_macroExpander.registerVariable(Constants::VAR_CURRENTKIT_NAME,
            tr("The name of the currently active kit."),
            [kit] { return kit->displayName(); },
            false);
        m_macroExpander.registerVariable(Constants::VAR_CURRENTKIT_FILESYSTEMNAME,
            tr("The name of the currently active kit in a filesystem-friendly version."),
            [kit] { return kit->fileSystemFriendlyName(); },
            false);
        m_macroExpander.registerVariable(Constants::VAR_CURRENTKIT_ID,
            tr("The id of the currently active kit."),
            [kit] { return kit->id().toString(); },
            false);
    }

    QString m_unexpandedDisplayName;
    QString m_fileSystemFriendlyName;
    QString m_autoDetectionSource;
    Id m_id;
    int m_nestedBlockingLevel;
    bool m_autodetected;
    bool m_sdkProvided;
    bool m_isValid;
    bool m_hasWarning;
    bool m_hasValidityInfo;
    bool m_mustNotify;
    QIcon m_icon;
    FileName m_iconPath;

    QHash<Id, QVariant> m_data;
    QSet<Id> m_sticky;
    QSet<Id> m_mutable;
    MacroExpander m_macroExpander;
};

} // namespace Internal

// -------------------------------------------------------------------------
// Kit:
// -------------------------------------------------------------------------

Kit::Kit(Id id) :
    d(new Internal::KitPrivate(id, this))
{
    foreach (KitInformation *sti, KitManager::kitInformation())
        d->m_data.insert(sti->id(), sti->defaultValue(this));

    d->m_icon = icon(d->m_iconPath);
}

Kit::Kit(const QVariantMap &data) :
    d(new Internal::KitPrivate(Id(), this))
{
    d->m_id = Id::fromSetting(data.value(QLatin1String(ID_KEY)));

    d->m_autodetected = data.value(QLatin1String(AUTODETECTED_KEY)).toBool();
    d->m_autoDetectionSource = data.value(QLatin1String(AUTODETECTIONSOURCE_KEY)).toString();

    // if we don't have that setting assume that autodetected implies sdk
    QVariant value = data.value(QLatin1String(SDK_PROVIDED_KEY));
    if (value.isValid())
        d->m_sdkProvided = value.toBool();
    else
        d->m_sdkProvided = d->m_autodetected;

    d->m_unexpandedDisplayName = data.value(QLatin1String(DISPLAYNAME_KEY),
                                            d->m_unexpandedDisplayName).toString();
    d->m_fileSystemFriendlyName = data.value(QLatin1String(FILESYSTEMFRIENDLYNAME_KEY)).toString();
    d->m_iconPath = FileName::fromString(data.value(QLatin1String(ICON_KEY),
                                                    d->m_iconPath.toString()).toString());
    d->m_icon = icon(d->m_iconPath);

    QVariantMap extra = data.value(QLatin1String(DATA_KEY)).toMap();
    d->m_data.clear(); // remove default values
    const QVariantMap::ConstIterator cend = extra.constEnd();
    for (QVariantMap::ConstIterator it = extra.constBegin(); it != cend; ++it) {
        const QString key = it.key();
        if (!key.isEmpty())
            d->m_data.insert(Id::fromString(key), it.value());
    }

    QStringList mutableInfoList = data.value(QLatin1String(MUTABLE_INFO_KEY)).toStringList();
    foreach (const QString &mutableInfo, mutableInfoList)
        if (!mutableInfo.isEmpty())
            d->m_mutable.insert(Id::fromString(mutableInfo));

    QStringList stickyInfoList = data.value(QLatin1String(STICKY_INFO_KEY)).toStringList();
    foreach (const QString &stickyInfo, stickyInfoList)
        if (!stickyInfo.isEmpty())
            d->m_sticky.insert(Id::fromString(stickyInfo));
}

Kit::~Kit()
{
    delete d;
}

void Kit::blockNotification()
{
    ++d->m_nestedBlockingLevel;
}

void Kit::unblockNotification()
{
    --d->m_nestedBlockingLevel;
    if (d->m_nestedBlockingLevel > 0)
        return;
    if (d->m_mustNotify)
        kitUpdated();
}

Kit *Kit::clone(bool keepName) const
{
    Kit *k = new Kit;
    if (keepName)
        k->d->m_unexpandedDisplayName = d->m_unexpandedDisplayName;
    else
        k->d->m_unexpandedDisplayName = QCoreApplication::translate("ProjectExplorer::Kit", "Clone of %1")
                .arg(d->m_unexpandedDisplayName);
    k->d->m_autodetected = false;
    k->d->m_data = d->m_data;
    // Do not clone m_fileSystemFriendlyName, needs to be unique
    k->d->m_isValid = d->m_isValid;
    k->d->m_icon = d->m_icon;
    k->d->m_iconPath = d->m_iconPath;
    k->d->m_sticky = d->m_sticky;
    k->d->m_mutable = d->m_mutable;
    return k;
}

void Kit::copyFrom(const Kit *k)
{
    KitGuard g(this);
    d->m_data = k->d->m_data;
    d->m_iconPath = k->d->m_iconPath;
    d->m_icon = k->d->m_icon;
    d->m_autodetected = k->d->m_autodetected;
    d->m_autoDetectionSource = k->d->m_autoDetectionSource;
    d->m_unexpandedDisplayName = k->d->m_unexpandedDisplayName;
    d->m_fileSystemFriendlyName = k->d->m_fileSystemFriendlyName;
    d->m_mustNotify = true;
    d->m_sticky = k->d->m_sticky;
    d->m_mutable = k->d->m_mutable;
}

bool Kit::isValid() const
{
    if (!d->m_id.isValid())
        return false;

    if (!d->m_hasValidityInfo)
        validate();

    return d->m_isValid;
}

bool Kit::hasWarning() const
{
    if (!d->m_hasValidityInfo)
        validate();

    return d->m_hasWarning;
}

QList<Task> Kit::validate() const
{
    QList<Task> result;
    QList<KitInformation *> infoList = KitManager::kitInformation();
    d->m_isValid = true;
    d->m_hasWarning = false;
    foreach (KitInformation *i, infoList) {
        QList<Task> tmp = i->validate(this);
        foreach (const Task &t, tmp) {
            if (t.type == Task::Error)
                d->m_isValid = false;
            if (t.type == Task::Warning)
                d->m_hasWarning = true;
        }
        result.append(tmp);
    }
    Utils::sort(result);
    d->m_hasValidityInfo = true;
    return result;
}

void Kit::fix()
{
    KitGuard g(this);
    foreach (KitInformation *i, KitManager::kitInformation())
        i->fix(this);
}

void Kit::setup()
{
    KitGuard g(this);
    // Process the KitInfos in reverse order: They may only be based on other information lower in
    // the stack.
    QList<KitInformation *> info = KitManager::kitInformation();
    for (int i = info.count() - 1; i >= 0; --i)
        info.at(i)->setup(this);
}

QString Kit::unexpandedDisplayName() const
{
    return d->m_unexpandedDisplayName;
}

QString Kit::displayName() const
{
    return d->m_macroExpander.expand(d->m_unexpandedDisplayName);
}

void Kit::setUnexpandedDisplayName(const QString &name)
{
    if (d->m_unexpandedDisplayName == name)
        return;

    d->m_unexpandedDisplayName = name;
    kitUpdated();
}

void Kit::setCustomFileSystemFriendlyName(const QString &fileSystemFriendlyName)
{
    d->m_fileSystemFriendlyName = fileSystemFriendlyName;
}

QString Kit::customFileSystemFriendlyName() const
{
    return d->m_fileSystemFriendlyName;
}

QString Kit::fileSystemFriendlyName() const
{
    QString name = customFileSystemFriendlyName();
    if (name.isEmpty())
        name = FileUtils::qmakeFriendlyName(displayName());
    foreach (Kit *i, KitManager::kits()) {
        if (i == this)
            continue;
        if (name == FileUtils::qmakeFriendlyName(i->displayName())) {
            // append part of the kit id: That should be unique enough;-)
            // Leading { will be turned into _ which should be fine.
            name = FileUtils::qmakeFriendlyName(name + QLatin1Char('_') + (id().toString().left(7)));
            break;
        }
    }
    return name;
}

bool Kit::isAutoDetected() const
{
    return d->m_autodetected;
}

QString Kit::autoDetectionSource() const
{
    return d->m_autoDetectionSource;
}

bool Kit::isSdkProvided() const
{
    return d->m_sdkProvided;
}

Id Kit::id() const
{
    return d->m_id;
}

QIcon Kit::icon() const
{
    return d->m_icon;
}

QIcon Kit::icon(const FileName &path)
{
    if (path.isEmpty())
        return QIcon();
    if (path == FileName::fromLatin1(":///DESKTOP///"))
        return qApp->style()->standardIcon(QStyle::SP_ComputerIcon);

    QFileInfo fi(path.toString());
    if (fi.isFile() && fi.isReadable())
        return QIcon(path.toString());
    return QIcon();
}

FileName Kit::iconPath() const
{
    return d->m_iconPath;
}

void Kit::setIconPath(const FileName &path)
{
    if (d->m_iconPath == path)
        return;
    d->m_iconPath = path;
    d->m_icon = icon(path);
    kitUpdated();
}

QVariant Kit::value(Id key, const QVariant &unset) const
{
    return d->m_data.value(key, unset);
}

bool Kit::hasValue(Id key) const
{
    return d->m_data.contains(key);
}

void Kit::setValue(Id key, const QVariant &value)
{
    if (d->m_data.value(key) == value)
        return;
    d->m_data.insert(key, value);
    kitUpdated();
}

/// \internal
void Kit::setValueSilently(Id key, const QVariant &value)
{
    if (d->m_data.value(key) == value)
        return;
    d->m_data.insert(key, value);
}

/// \internal
void Kit::removeKeySilently(Id key)
{
    if (!d->m_data.contains(key))
        return;
    d->m_data.remove(key);
    d->m_sticky.remove(key);
    d->m_mutable.remove(key);
}


void Kit::removeKey(Id key)
{
    if (!d->m_data.contains(key))
        return;
    d->m_data.remove(key);
    d->m_sticky.remove(key);
    d->m_mutable.remove(key);
    kitUpdated();
}

bool Kit::isSticky(Id id) const
{
    return d->m_sticky.contains(id);
}

bool Kit::isDataEqual(const Kit *other) const
{
    return d->m_data == other->d->m_data;
}

bool Kit::isEqual(const Kit *other) const
{
    return isDataEqual(other)
            && d->m_iconPath == other->d->m_iconPath
            && d->m_unexpandedDisplayName == other->d->m_unexpandedDisplayName
            && d->m_fileSystemFriendlyName == other->d->m_fileSystemFriendlyName
            && d->m_mutable == other->d->m_mutable;

}

QVariantMap Kit::toMap() const
{
    typedef QHash<Id, QVariant>::ConstIterator IdVariantConstIt;

    QVariantMap data;
    data.insert(QLatin1String(ID_KEY), QString::fromLatin1(d->m_id.name()));
    data.insert(QLatin1String(DISPLAYNAME_KEY), d->m_unexpandedDisplayName);
    data.insert(QLatin1String(AUTODETECTED_KEY), d->m_autodetected);
    if (!d->m_fileSystemFriendlyName.isEmpty())
        data.insert(QLatin1String(FILESYSTEMFRIENDLYNAME_KEY), d->m_fileSystemFriendlyName);
    data.insert(QLatin1String(AUTODETECTIONSOURCE_KEY), d->m_autoDetectionSource);
    data.insert(QLatin1String(SDK_PROVIDED_KEY), d->m_sdkProvided);
    data.insert(QLatin1String(ICON_KEY), d->m_iconPath.toString());

    QStringList mutableInfo;
    foreach (Id id, d->m_mutable)
        mutableInfo << id.toString();
    data.insert(QLatin1String(MUTABLE_INFO_KEY), mutableInfo);

    QStringList stickyInfo;
    foreach (Id id, d->m_sticky)
        stickyInfo << id.toString();
    data.insert(QLatin1String(STICKY_INFO_KEY), stickyInfo);

    QVariantMap extra;

    const IdVariantConstIt cend = d->m_data.constEnd();
    for (IdVariantConstIt it = d->m_data.constBegin(); it != cend; ++it)
        extra.insert(QString::fromLatin1(it.key().name().constData()), it.value());
    data.insert(QLatin1String(DATA_KEY), extra);

    return data;
}

void Kit::addToEnvironment(Environment &env) const
{
    QList<KitInformation *> infoList = KitManager::kitInformation();
    foreach (KitInformation *ki, infoList)
        ki->addToEnvironment(this, env);
}

IOutputParser *Kit::createOutputParser() const
{
    IOutputParser *first = new OsParser;
    QList<KitInformation *> infoList = KitManager::kitInformation();
    foreach (KitInformation *ki, infoList)
        first->appendOutputParser(ki->createOutputParser(this));
    return first;
}

QString Kit::toHtml(const QList<Task> &additional) const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body>";
    str << "<h3>" << displayName() << "</h3>";
    str << "<table>";

    if (!isValid() || hasWarning() || !additional.isEmpty()) {
        QList<Task> issues = additional;
        issues.append(validate());
        str << "<p>";
        foreach (const Task &t, issues) {
            str << "<b>";
            switch (t.type) {
            case Task::Error:
                str << QCoreApplication::translate("ProjectExplorer::Kit", "Error:") << " ";
                break;
            case Task::Warning:
                str << QCoreApplication::translate("ProjectExplorer::Kit", "Warning:") << " ";
                break;
            case Task::Unknown:
            default:
                break;
            }
            str << "</b>" << t.description << "<br>";
        }
        str << "</p>";
    }

    QList<KitInformation *> infoList = KitManager::kitInformation();
    foreach (KitInformation *ki, infoList) {
        KitInformation::ItemList list = ki->toUserOutput(this);
        foreach (const KitInformation::Item &j, list)
            str << "<tr><td><b>" << j.first << ":</b></td><td>" << j.second << "</td></tr>";
    }
    str << "</table></body></html>";
    return rc;
}

void Kit::setAutoDetected(bool detected)
{
    d->m_autodetected = detected;
    kitUpdated();
}

void Kit::setAutoDetectionSource(const QString &autoDetectionSource)
{
    d->m_autoDetectionSource = autoDetectionSource;
    kitUpdated();
}

void Kit::setSdkProvided(bool sdkProvided)
{
    d->m_sdkProvided = sdkProvided;
    kitUpdated();
}

void Kit::makeSticky()
{
    foreach (KitInformation *ki, KitManager::kitInformation()) {
        if (hasValue(ki->id()))
            setSticky(ki->id(), true);
    }
}

void Kit::setSticky(Id id, bool b)
{
    if (b)
        d->m_sticky.insert(id);
    else
        d->m_sticky.remove(id);
    kitUpdated();
}

void Kit::makeUnSticky()
{
    d->m_sticky.clear();
    kitUpdated();
}

void Kit::setMutable(Id id, bool b)
{
    if (b)
        d->m_mutable.insert(id);
    else
        d->m_mutable.remove(id);
    kitUpdated();
}

bool Kit::isMutable(Id id) const
{
    return d->m_mutable.contains(id);
}

QSet<QString> Kit::availablePlatforms() const
{
    QSet<QString> platforms;
    foreach (const KitInformation *ki, KitManager::kitInformation())
        platforms.unite(ki->availablePlatforms(this));
    return platforms;
}

bool Kit::hasPlatform(const QString &platform) const
{
    if (platform.isEmpty())
        return true;
    return availablePlatforms().contains(platform);
}

QString Kit::displayNameForPlatform(const QString &platform) const
{
    foreach (const KitInformation *ki, KitManager::kitInformation()) {
        const QString displayName = ki->displayNameForPlatform(this, platform);
        if (!displayName.isEmpty())
            return displayName;
    }
    return QString();

}

FeatureSet Kit::availableFeatures() const
{
    FeatureSet features;
    foreach (const KitInformation *ki, KitManager::kitInformation())
        features |= ki->availableFeatures(this);
    return features;
}

bool Kit::hasFeatures(const FeatureSet &features) const
{
    return availableFeatures().contains(features);
}

MacroExpander *Kit::macroExpander() const
{
    return &d->m_macroExpander;
}

void Kit::kitUpdated()
{
    if (d->m_nestedBlockingLevel > 0) {
        d->m_mustNotify = true;
        return;
    }
    d->m_hasValidityInfo = false;
    KitManager::notifyAboutUpdate(this);
    d->m_mustNotify = false;
}

} // namespace ProjectExplorer
