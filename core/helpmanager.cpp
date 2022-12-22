/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of Qt Creator.
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

#include "helpmanager.h"

#include <core/icore.h>
#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <QUrl>

// TODO RK
#ifndef QT_NO_HELP
#include <QHelpEngineCore>
#endif

#ifndef QT_NO_SQL
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#endif

static const char kUserDocumentationKey[] = "Help/UserDocumentation";

namespace Core {

struct HelpManagerPrivate
{
    HelpManagerPrivate() :
       m_needsSetup(true),
   #ifndef QT_NO_HELP
       m_helpEngine(0),
   #endif
       m_collectionWatcher(0)
    {}

    QStringList documentationFromInstaller();
    void readSettings();
    void writeSettings();
    void cleanUpDocumentation();

    bool m_needsSetup;
#ifndef QT_NO_HELP
    QHelpEngineCore *m_helpEngine;
#endif
    Utils::FileSystemWatcher *m_collectionWatcher;

    // data for delayed initialization
    QSet<QString> m_filesToRegister;
    QSet<QString> m_nameSpacesToUnregister;
    QHash<QString, QVariant> m_customValues;

    QSet<QString> m_userRegisteredFiles;
};

static HelpManager *m_instance = 0;
static HelpManagerPrivate *d;

static const char linksForKeyQuery[] = "SELECT d.Title, f.Name, e.Name, "
    "d.Name, a.Anchor FROM IndexTable a, FileNameTable d, FolderTable e, "
    "NamespaceTable f WHERE a.FileId=d.FileId AND d.FolderId=e.Id AND "
    "a.NamespaceId=f.Id AND a.Name='%1'";

// -- DbCleaner

struct DbCleaner
{
    DbCleaner(const QString &dbName) : name(dbName) {}
#ifndef QT_NO_SQL
    ~DbCleaner() { QSqlDatabase::removeDatabase(name); }
#endif
    QString name;
};

// -- HelpManager

HelpManager::HelpManager(QObject *parent) :
    QObject(parent)
{
    QTC_CHECK(!m_instance);
    m_instance = this;
    d = new HelpManagerPrivate;
}

HelpManager::~HelpManager()
{
    d->writeSettings();
#ifndef QT_NO_HELP
    delete d->m_helpEngine;
    d->m_helpEngine = 0;
#endif
    m_instance = 0;
    delete d;
}

HelpManager *HelpManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

QString HelpManager::collectionFilePath()
{
    return QDir::cleanPath(ICore::userResourcePath()
        + QLatin1String("/helpcollection.qhc"));
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    if (d->m_needsSetup) {
        foreach (const QString &filePath, files)
            d->m_filesToRegister.insert(filePath);
        return;
    }

    bool docsChanged = false;
#ifndef QT_NO_HELP
    foreach (const QString &file, files) {
        const QString &nameSpace = d->m_helpEngine->namespaceName(file);
        if (nameSpace.isEmpty())
            continue;
        if (!d->m_helpEngine->registeredDocumentations().contains(nameSpace)) {
            if (d->m_helpEngine->registerDocumentation(file)) {
                docsChanged = true;
            } else {
                qWarning() << "Error registering namespace '" << nameSpace
                    << "' from file '" << file << "':" << d->m_helpEngine->error();
            }
        } else {
            const QLatin1String key("CreationDate");
            const QString &newDate = d->m_helpEngine->metaData(file, key).toString();
            const QString &oldDate = d->m_helpEngine->metaData(
                d->m_helpEngine->documentationFileName(nameSpace), key).toString();
            if (QDateTime::fromString(newDate, Qt::ISODate)
                > QDateTime::fromString(oldDate, Qt::ISODate)) {
                if (d->m_helpEngine->unregisterDocumentation(nameSpace)) {
                    docsChanged = true;
                    d->m_helpEngine->registerDocumentation(file);
                }
            }
        }
    }
#endif
    if (docsChanged)
        emit m_instance->documentationChanged();
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    if (d->m_needsSetup) {
        foreach (const QString &name, nameSpaces)
            d->m_nameSpacesToUnregister.insert(name);
        return;
    }

    bool docsChanged = false;
#ifndef QT_NO_HELP
    foreach (const QString &nameSpace, nameSpaces) {
        const QString filePath = d->m_helpEngine->documentationFileName(nameSpace);
        if (d->m_helpEngine->unregisterDocumentation(nameSpace)) {
            docsChanged = true;
            d->m_userRegisteredFiles.remove(filePath);
        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                << "' from file '" << filePath
                << "': " << d->m_helpEngine->error();
        }
    }
#endif
    if (docsChanged)
        emit m_instance->documentationChanged();
}

void HelpManager::registerUserDocumentation(const QStringList &filePaths)
{
    foreach (const QString &filePath, filePaths)
        d->m_userRegisteredFiles.insert(filePath);
    registerDocumentation(filePaths);
}

QSet<QString> HelpManager::userDocumentationPaths()
{
    return d->m_userRegisteredFiles;
}

static QUrl buildQUrl(const QString &ns, const QString &folder,
    const QString &relFileName, const QString &anchor)
{
    QUrl url;
    url.setScheme(QLatin1String("qthelp"));
    url.setAuthority(ns);
    url.setPath(QLatin1Char('/') + folder + QLatin1Char('/') + relFileName);
    url.setFragment(anchor);
    return url;
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key)
{
    QMap<QString, QUrl> links;
    QTC_ASSERT(!d->m_needsSetup, return links);

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::linksForKeyword");

    DbCleaner cleaner(name);
#if !defined QT_NO_SQL && !defined QT_NO_HELP
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(d->m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1(linksForKeyQuery).arg(key));
                while (query.next()) {
                    QString title = query.value(0).toString();
                    if (title.isEmpty()) // generate a title + corresponding path
                        title = key + QLatin1String(" : ") + query.value(3).toString();
                    links.insertMulti(title, buildQUrl(query.value(1).toString(),
                        query.value(2).toString(), query.value(3).toString(),
                        query.value(4).toString()));
                }
            }
        }
    }
#endif
    return links;
}

QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id)
{
    QMap<QString, QUrl> empty;
    QTC_ASSERT(!d->m_needsSetup, return empty);
#ifndef QT_NO_HELP
    return d->m_helpEngine->linksForIdentifier(id);
#else
    return QMap<QString, QUrl>();
#endif
}

QUrl HelpManager::findFile(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return QUrl());
#ifndef QT_NO_HELP
    return d->m_helpEngine->findFile(url);
#else
    return QUrl();
#endif
}

QByteArray HelpManager::fileData(const QUrl &url)
{
    QTC_ASSERT(!d->m_needsSetup, return QByteArray());
#ifndef QT_NO_HELP
    return d->m_helpEngine->fileData(url);
#else
    return QByteArray();
#endif
}

void HelpManager::handleHelpRequest(const QUrl &url, HelpManager::HelpViewerLocation location)
{
    emit m_instance->helpRequested(url, location);
}

void HelpManager::handleHelpRequest(const QString &url, HelpViewerLocation location)
{
    handleHelpRequest(QUrl(url), location);
}

QStringList HelpManager::registeredNamespaces()
{
    QTC_ASSERT(!d->m_needsSetup, return QStringList());
#ifndef QT_NO_HELP
    return d->m_helpEngine->registeredDocumentations();
#else
    return QStringList();
#endif
}

QString HelpManager::namespaceFromFile(const QString &file)
{
    QTC_ASSERT(!d->m_needsSetup, return QString());
#ifndef QT_NO_HELP
    return d->m_helpEngine->namespaceName(file);
#else
    return QString();
#endif
}

QString HelpManager::fileFromNamespace(const QString &nameSpace)
{
    QTC_ASSERT(!d->m_needsSetup, return QString());
#ifndef QT_NO_HELP
    return d->m_helpEngine->documentationFileName(nameSpace);
#else
    return QString();
#endif
}

void HelpManager::setCustomValue(const QString &key, const QVariant &value)
{
    if (d->m_needsSetup) {
        d->m_customValues.insert(key, value);
        return;
    }
#ifndef QT_NO_HELP
    if (d->m_helpEngine->setCustomValue(key, value))
        emit m_instance->collectionFileChanged();
#endif
}

QVariant HelpManager::customValue(const QString &key, const QVariant &value)
{
    QTC_ASSERT(!d->m_needsSetup, return QVariant());
#ifndef QT_NO_HELP
    return d->m_helpEngine->customValue(key, value);
#else
    return QVariant();
#endif
}

HelpManager::Filters HelpManager::filters()
{
    QTC_ASSERT(!d->m_needsSetup, return Filters());

    Filters filters;
#ifndef QT_NO_HELP
    const QStringList &customFilters = d->m_helpEngine->customFilters();
    foreach (const QString &filter, customFilters)
        filters.insert(filter, d->m_helpEngine->filterAttributes(filter));
#endif
    return filters;
}

HelpManager::Filters HelpManager::fixedFilters()
{
    Filters fixedFilters;
    QTC_ASSERT(!d->m_needsSetup, return fixedFilters);

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::fixedCustomFilters");

    DbCleaner cleaner(name);
#if !defined QT_NO_SQL && !defined QT_NO_HELP
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = d->m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(d->m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QLatin1String("SELECT Name FROM FilterNameTable"));
                while (query.next()) {
                    const QString &filter = query.value(0).toString();
                    fixedFilters.insert(filter, d->m_helpEngine->filterAttributes(filter));
                }
            }
        }
    }
#endif
    return fixedFilters;
}

HelpManager::Filters HelpManager::userDefinedFilters()
{
    QTC_ASSERT(!d->m_needsSetup, return Filters());

    Filters all = filters();
    const Filters &fixed = fixedFilters();
    for (Filters::const_iterator it = fixed.constBegin(); it != fixed.constEnd(); ++it)
        all.remove(it.key());
    return all;
}

void HelpManager::removeUserDefinedFilter(const QString &filter)
{
    QTC_ASSERT(!d->m_needsSetup, return);

#ifndef QT_NO_HELP
    if (d->m_helpEngine->removeCustomFilter(filter))
        emit m_instance->collectionFileChanged();
#endif
}

void HelpManager::addUserDefinedFilter(const QString &filter, const QStringList &attr)
{
    QTC_ASSERT(!d->m_needsSetup, return);

#ifndef QT_NO_HELP
    if (d->m_helpEngine->addCustomFilter(filter, attr))
        emit m_instance->collectionFileChanged();
#endif
}

// -- private slots

void HelpManager::setupHelpManager()
{
    if (!d->m_needsSetup)
        return;
    d->m_needsSetup = false;

    d->readSettings();

    // create the help engine
#ifndef QT_NO_HELP
    d->m_helpEngine = new QHelpEngineCore(collectionFilePath(), m_instance);
    d->m_helpEngine->setAutoSaveFilter(false);
    d->m_helpEngine->setCurrentFilter(tr("Unfiltered"));
    d->m_helpEngine->setupData();
#endif

    foreach (const QString &filePath, d->documentationFromInstaller())
        d->m_filesToRegister.insert(filePath);

    d->cleanUpDocumentation();

    if (!d->m_nameSpacesToUnregister.isEmpty()) {
        unregisterDocumentation(d->m_nameSpacesToUnregister.toList());
        d->m_nameSpacesToUnregister.clear();
    }

    if (!d->m_filesToRegister.isEmpty()) {
        registerDocumentation(d->m_filesToRegister.toList());
        d->m_filesToRegister.clear();
    }

    QHash<QString, QVariant>::const_iterator it;
    for (it = d->m_customValues.constBegin(); it != d->m_customValues.constEnd(); ++it)
        setCustomValue(it.key(), it.value());

    emit m_instance->setupFinished();
}

// -- private

void HelpManagerPrivate::cleanUpDocumentation()
{
    // mark documentation for removal for which there is no documentation file anymore
    // mark documentation for removal that is neither user registered, nor marked for registration
#ifndef QT_NO_HELP
    const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
    foreach (const QString &nameSpace, registeredDocs) {
        const QString filePath = m_helpEngine->documentationFileName(nameSpace);
        if (!QFileInfo::exists(filePath)
                || (!m_filesToRegister.contains(filePath)
                    && !m_userRegisteredFiles.contains(filePath))) {
            m_nameSpacesToUnregister.insert(nameSpace);
        }
    }
#endif
}

QStringList HelpManagerPrivate::documentationFromInstaller()
{
    QSettings *installSettings = ICore::settings();
    QStringList documentationPaths = installSettings->value(QLatin1String("Help/InstalledDocumentation"))
            .toStringList();
    QStringList documentationFiles;
    foreach (const QString &path, documentationPaths) {
        QFileInfo pathInfo(path);
        if (pathInfo.isFile() && pathInfo.isReadable()) {
            documentationFiles << pathInfo.absoluteFilePath();
        } else if (pathInfo.isDir()) {
            QDir dir(path);
            foreach (const QFileInfo &fileInfo, dir.entryInfoList(QStringList(QLatin1String("*.qch")),
                                                              QDir::Files | QDir::Readable)) {
                documentationFiles << fileInfo.absoluteFilePath();
            }
        }
    }
    return documentationFiles;
}

void HelpManagerPrivate::readSettings()
{
    m_userRegisteredFiles = ICore::settings()->value(QLatin1String(kUserDocumentationKey))
            .toStringList().toSet();
}

void HelpManagerPrivate::writeSettings()
{
    const QStringList list = m_userRegisteredFiles.toList();
    ICore::settings()->setValue(QLatin1String(kUserDocumentationKey), list);
}

}   // Core
