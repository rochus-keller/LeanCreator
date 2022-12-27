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

#include "settingsdatabase.h"

#include <QDir>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#ifndef QT_NO_SQL
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#endif
#include <QDebug>

/*!
    \class Core::SettingsDatabase
    \brief The SettingsDatabase class offers an alternative to the
    application-wide QSettings that is more
    suitable for storing large amounts of data.

    The settings database is SQLite based, and lazily retrieves data when it
    is asked for. It also does incremental updates of the database rather than
    rewriting the whole file each time one of the settings change.

    The SettingsDatabase API mimics that of QSettings.
*/

using namespace Core;
using namespace Core::Internal;

enum { debug_settings = 0 };

namespace Core {
namespace Internal {

typedef QMap<QString, QVariant> SettingsMap;

class SettingsDatabasePrivate
{
public:
    QString effectiveGroup() const
    {
        return m_groups.join(QString(QLatin1Char('/')));
    }

    QString effectiveKey(const QString &key) const
    {
        QString g = effectiveGroup();
        if (!g.isEmpty() && !key.isEmpty())
            g += QLatin1Char('/');
        g += key;
        return g;
    }

    SettingsMap m_settings;

    QStringList m_groups;
    QStringList m_dirtyKeys;

#ifndef QT_NO_SQL
    QSqlDatabase m_db; // TODO RK: replace by something without SQL
#endif
};

} // namespace Internal
} // namespace Core

SettingsDatabase::SettingsDatabase(const QString &path,
                                   const QString &application,
                                   QObject *parent)
    : QObject(parent)
    , d(new SettingsDatabasePrivate)
{
    const QLatin1Char slash('/');

    // TODO: Don't rely on a path, but determine automatically
    QDir pathDir(path);
    if (!pathDir.exists())
        pathDir.mkpath(pathDir.absolutePath());

    QString fileName = path;
    if (!fileName.endsWith(slash))
        fileName += slash;
    fileName += application;
    fileName += QLatin1String(".db");

#ifndef QT_NO_SQL
    d->m_db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("settings"));
    d->m_db.setDatabaseName(fileName);
    if (!d->m_db.open()) {
        qWarning().nospace() << "Warning: Failed to open settings database at " << fileName << " ("
                             << d->m_db.lastError().driverText() << ")";
    } else {
        // Create the settings table if it doesn't exist yet
        QSqlQuery query(d->m_db);
        query.prepare(QLatin1String("CREATE TABLE IF NOT EXISTS settings ("
                                    "key PRIMARY KEY ON CONFLICT REPLACE, "
                                    "value)"));
        if (!query.exec())
            qWarning().nospace() << "Warning: Failed to prepare settings database! ("
                                 << query.lastError().driverText() << ")";

        // Retrieve all available keys (values are retrieved lazily)
        if (query.exec(QLatin1String("SELECT key FROM settings"))) {
            while (query.next()) {
                d->m_settings.insert(query.value(0).toString(), QVariant());
            }
        }

        // syncing can be slow, especially on Linux and Windows
        d->m_db.exec(QLatin1String("PRAGMA synchronous = OFF;"));
    }
#endif
}

SettingsDatabase::~SettingsDatabase()
{
    sync();

    delete d;
#ifndef QT_NO_SQL
    QSqlDatabase::removeDatabase(QLatin1String("settings"));
#endif
}

void SettingsDatabase::setValue(const QString &key, const QVariant &value)
{
    const QString effectiveKey = d->effectiveKey(key);

    // Add to cache
    d->m_settings.insert(effectiveKey, value);

#ifndef QT_NO_SQL
    if (!d->m_db.isOpen())
        return;

    // Instant apply (TODO: Delay writing out settings)
    QSqlQuery query(d->m_db);
    query.prepare(QLatin1String("INSERT INTO settings VALUES (?, ?)"));
    query.addBindValue(effectiveKey);
    query.addBindValue(value);
    query.exec();

    if (debug_settings)
        qDebug() << "Stored:" << effectiveKey << "=" << value;
#endif
}

QVariant SettingsDatabase::value(const QString &key, const QVariant &defaultValue) const
{
    const QString effectiveKey = d->effectiveKey(key);
    QVariant value = defaultValue;

    SettingsMap::const_iterator i = d->m_settings.constFind(effectiveKey);
    if (i != d->m_settings.constEnd() && i.value().isValid()) {
        value = i.value();
    }
#ifndef QT_NO_SQL
    else if (d->m_db.isOpen()) {
        // Try to read the value from the database
        QSqlQuery query(d->m_db);
        query.prepare(QLatin1String("SELECT value FROM settings WHERE key = ?"));
        query.addBindValue(effectiveKey);
        query.exec();
        if (query.next()) {
            value = query.value(0);

            if (debug_settings)
                qDebug() << "Retrieved:" << effectiveKey << "=" << value;
        }

        // Cache the result
        d->m_settings.insert(effectiveKey, value);
    }
#endif

    return value;
}

bool SettingsDatabase::contains(const QString &key) const
{
    return d->m_settings.contains(d->effectiveKey(key));
}

void SettingsDatabase::remove(const QString &key)
{
    const QString effectiveKey = d->effectiveKey(key);

    // Remove keys from the cache
    foreach (const QString &k, d->m_settings.keys()) {
        // Either it's an exact match, or it matches up to a /
        if (k.startsWith(effectiveKey)
            && (k.length() == effectiveKey.length()
                || k.at(effectiveKey.length()) == QLatin1Char('/')))
        {
            d->m_settings.remove(k);
        }
    }

#ifndef QT_NO_SQL
    if (!d->m_db.isOpen())
        return;

    // Delete keys from the database
    QSqlQuery query(d->m_db);
    query.prepare(QLatin1String("DELETE FROM settings WHERE key = ? OR key LIKE ?"));
    query.addBindValue(effectiveKey);
    query.addBindValue(QString(effectiveKey + QLatin1String("/%")));
    query.exec();
#endif
}

void SettingsDatabase::beginGroup(const QString &prefix)
{
    d->m_groups.append(prefix);
}

void SettingsDatabase::endGroup()
{
    d->m_groups.removeLast();
}

QString SettingsDatabase::group() const
{
    return d->effectiveGroup();
}

QStringList SettingsDatabase::childKeys() const
{
    QStringList children;

    const QString g = group();
    QMapIterator<QString, QVariant> i(d->m_settings);
    while (i.hasNext()) {
        const QString &key = i.next().key();
        if (key.startsWith(g) && key.indexOf(QLatin1Char('/'), g.length() + 1) == -1)
            children.append(key.mid(g.length() + 1));
    }

    return children;
}

void SettingsDatabase::beginTransaction()
{
#ifndef QT_NO_SQL
    if (!d->m_db.isOpen())
        return;
    d->m_db.exec(QLatin1String("BEGIN TRANSACTION;"));
#endif
}

void SettingsDatabase::endTransaction()
{
#ifndef QT_NO_SQL
    if (!d->m_db.isOpen())
        return;
    d->m_db.exec(QLatin1String("END TRANSACTION;"));
#endif
}

void SettingsDatabase::sync()
{
    // TODO: Delay writing of dirty keys and save them here
}
