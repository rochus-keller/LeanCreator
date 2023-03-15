/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhelpcollectionhandler_p.h"
#include "qhelp_global.h"
#include "qhelpdbreader_p.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include "sqlite3.h"

QT_BEGIN_NAMESPACE

class QHelpCollectionHandler::Db {
public:
    Db():db(0){}
    ~Db()
    {
        sqlite3_close(db);
    }

    sqlite3* db;

    bool exec( const QByteArray& sql )
    {
        return sqlite3_exec(db, sql.constData(), 0, 0, 0 ) == SQLITE_OK;
    }
    int lastId()
    {
        return sqlite3_last_insert_rowid(db);
    }

    struct Stm
    {
        sqlite3_stmt* s;
        Stm( Db* db, const QByteArray& sql):s(0)
        {
            sqlite3_prepare( db->db, sql.constData(), sql.size(), &s, 0 );
        }
        ~Stm()
        {
            sqlite3_finalize(s);
        }
        bool next()
        {
            return sqlite3_step(s) == SQLITE_ROW;
        }
        bool exec()
        {
            const int res = sqlite3_step(s);
            return res == SQLITE_ROW || res == SQLITE_DONE;
        }
        QString text(int col) const
        {
            const char* str = (const char*)sqlite3_column_text(s, col);
            const int len = sqlite3_column_bytes(s, col);
            return QString::fromUtf8(str,len);
        }
        int toInt(int col) const
        {
            return sqlite3_column_int(s,col);
        }
        QByteArray byteArray(int col) const
        {
            const void* data = sqlite3_column_blob(s, col);
            const int len = sqlite3_column_bytes(s, col);
            return QByteArray((const char*)data,len);
        }
        void bind(int col, const QString& str)
        {
            const QByteArray tmp = str.toUtf8();
            sqlite3_bind_text(s, col + 1, tmp.constData(), tmp.size(), SQLITE_TRANSIENT);
        }
        void bind(int col, const QByteArray& str)
        {
            sqlite3_bind_blob(s, col + 1, str.constData(), str.size(), SQLITE_TRANSIENT);
        }
        void bind(int col, int i)
        {
            sqlite3_bind_int(s, col + 1, i);
        }
   };
};

QHelpCollectionHandler::QHelpCollectionHandler(const QString &collectionFile, QObject *parent)
    : QObject(parent)
    , m_dbOpened(false)
    , m_collectionFile(collectionFile)
    , m_connectionName(QString())
    , d_db( new Db() )
{
    QFileInfo fi(m_collectionFile);
    if (!fi.isAbsolute())
        m_collectionFile = fi.absoluteFilePath();
}

QHelpCollectionHandler::~QHelpCollectionHandler()
{
    if( d_db )
        delete d_db;
}

bool QHelpCollectionHandler::isDBOpened()
{
    if (m_dbOpened)
        return true;
    emit error(tr("The collection file '%1' is not set up yet.").
               arg(m_collectionFile));
    return false;
}

QString QHelpCollectionHandler::collectionFile() const
{
    return m_collectionFile;
}

bool QHelpCollectionHandler::openCollectionFile()
{
    if (m_dbOpened)
        return m_dbOpened;

    m_connectionName = QHelpGlobal::uniquifyConnectionName(
        QLatin1String("QHelpCollectionHandler"), this);
    bool openingOk = sqlite3_open( collectionFile().toUtf8().constData(), &d_db->db) == SQLITE_OK;
    if (!openingOk) {
        sqlite3_close(d_db->db);
        emit error(tr("Cannot open collection file: %1").arg(collectionFile()));
        return false;
    }

    d_db->exec("PRAGMA synchronous=OFF");
    d_db->exec("PRAGMA cache_size=3000");

    Db::Stm s(d_db, "SELECT COUNT(*) FROM sqlite_master WHERE TYPE=\'table\'"
                               "AND Name=\'NamespaceTable\'");
    if ( s.next() && s.toInt(0) < 1) {
        if (!createTables(d_db)) {
            emit error(tr("Cannot create tables in file %1.").arg(collectionFile()));
            return false;
        }
    }
    m_dbOpened = true;
    return m_dbOpened;
}

bool QHelpCollectionHandler::copyCollectionFile(const QString &fileName)
{
    if (!m_dbOpened)
        return false;

    QFileInfo fi(fileName);
    if (fi.exists()) {
        emit error(tr("The collection file '%1' already exists.").
                   arg(fileName));
        return false;
    }

    if (!fi.absoluteDir().exists() && !QDir().mkpath(fi.absolutePath())) {
        emit error(tr("Cannot create directory: %1").arg(fi.absolutePath()));
        return false;
    }

    QString colFile = fi.absoluteFilePath();
    QString connectionName = QHelpGlobal::uniquifyConnectionName(
        QLatin1String("QHelpCollectionHandlerCopy"), this);
    Db db2;
    bool openingOk = sqlite3_open( colFile.toUtf8().constData(), &db2.db) == SQLITE_OK;
    if (!openingOk) {
        sqlite3_close(db2.db);
        emit error(tr("Cannot open collection file: %1").arg(colFile));
        return false;
    }

    db2.exec("PRAGMA synchronous=OFF");
    db2.exec("PRAGMA cache_size=3000");

    if (!createTables(&db2)) {
        emit error(tr("Cannot copy collection file: %1").arg(colFile));
        return false;
    }

    QString oldBaseDir = QFileInfo(collectionFile()).absolutePath();
    QString oldFilePath;
    QFileInfo newColFi(colFile);

    Db::Stm s1( d_db, "SELECT Name, FilePath FROM NamespaceTable");
    while (s1.next()) {
        Db::Stm s(&db2,"INSERT INTO NamespaceTable VALUES(NULL, ?, ?)");
        s.bind(0, s1.text(0) );
        oldFilePath = s1.text(1);
        if (!QDir::isAbsolutePath(oldFilePath))
            oldFilePath = oldBaseDir + QDir::separator() + oldFilePath;
        s.bind(1, newColFi.absoluteDir().relativeFilePath(oldFilePath));
        s.next();
    }

    Db::Stm s2( d_db, "SELECT NamespaceId, Name FROM FolderTable");
    while (s2.next()) {
        Db::Stm s(&db2,"INSERT INTO FolderTable VALUES(NULL, ?, ?)");
        s.bind(0, s2.text(0));
        s.bind(1, s2.text(1));
        s.next();
    }

    Db::Stm s3( d_db, "SELECT Name FROM FilterAttributeTable");
    while (s3.next()) {
        Db::Stm s(&db2,"INSERT INTO FilterAttributeTable VALUES(NULL, ?)");
        s.bind(0, s3.text(0));
        s.next();
    }

    Db::Stm s4( d_db, "SELECT Name FROM FilterNameTable");
    while (s4.next()) {
        Db::Stm s(&db2,"INSERT INTO FilterNameTable VALUES(NULL, ?)");
        s.bind(0, s4.text(0));
        s.next();
    }

    Db::Stm s5( d_db, "SELECT NameId, FilterAttributeId FROM FilterTable");
    while (s5.next()) {
        Db::Stm s(&db2,"INSERT INTO FilterTable VALUES(?, ?)");
        s.bind(0, s5.toInt(0));
        s.bind(1, s5.toInt(1));
        s.next();
    }

    Db::Stm s6( d_db, "SELECT Key, Value FROM SettingsTable");
    while (s6.next()) {
        const QString tmp = s6.text(0);
        if (tmp == QLatin1String("CluceneSearchNamespaces"))
            continue;
        Db::Stm s(&db2,"INSERT INTO SettingsTable VALUES(?, ?)");
        s.bind(0, tmp);
        s.bind(1, s6.byteArray(1));
        s.next();
    }

    return true;
}

bool QHelpCollectionHandler::createTables(Db* db)
{
    QStringList tables;
    tables << QLatin1String("CREATE TABLE NamespaceTable ("
        "Id INTEGER PRIMARY KEY, "
        "Name TEXT, "
        "FilePath TEXT )")
        << QLatin1String("CREATE TABLE FolderTable ("
        "Id INTEGER PRIMARY KEY, "
        "NamespaceId INTEGER, "
        "Name TEXT )")
        << QLatin1String("CREATE TABLE FilterAttributeTable ("
        "Id INTEGER PRIMARY KEY, "
        "Name TEXT )")
        << QLatin1String("CREATE TABLE FilterNameTable ("
        "Id INTEGER PRIMARY KEY, "
        "Name TEXT )")
        << QLatin1String("CREATE TABLE FilterTable ("
        "NameId INTEGER, "
        "FilterAttributeId INTEGER )")
        << QLatin1String("CREATE TABLE SettingsTable ("
        "Key TEXT PRIMARY KEY, "
        "Value BLOB )");

    foreach (const QString &q, tables) {
        if (!db->exec(q.toUtf8()))
            return false;
    }
    return true;
}

QStringList QHelpCollectionHandler::customFilters() const
{
    QStringList list;
    if (m_dbOpened) {
        Db::Stm s(d_db, "SELECT Name FROM FilterNameTable");
        while (s.next())
            list.append(s.text(0));
    }
    return list;
}

bool QHelpCollectionHandler::removeCustomFilter(const QString &filterName)
{
    if (!isDBOpened() || filterName.isEmpty())
        return false;

    int filterNameId = -1;
    Db::Stm s1(d_db, "SELECT Id FROM FilterNameTable WHERE Name=?");
    s1.bind(0, filterName);
    if (s1.next())
        filterNameId = s1.toInt(0);

    if (filterNameId < 0) {
        emit error(tr("Unknown filter '%1'.").arg(filterName));
        return false;
    }

    Db::Stm s2(d_db, "DELETE FROM FilterTable WHERE NameId=?");
    s2.bind(0, filterNameId);
    s2.next();

    Db::Stm s3(d_db, "DELETE FROM FilterNameTable WHERE Id=?");
    s3.bind(0, filterNameId);
    s3.next();
    return true;
}

bool QHelpCollectionHandler::addCustomFilter(const QString &filterName,
                                             const QStringList &attributes)
{
    if (!isDBOpened() || filterName.isEmpty())
        return false;

    int nameId = -1;
    Db::Stm s1(d_db, "SELECT Id FROM FilterNameTable WHERE Name=?");
    s1.bind(0, filterName);
    if (s1.next())
        nameId = s1.toInt(0);

    Db::Stm s2(d_db, "SELECT Id, Name FROM FilterAttributeTable");
    QStringList idsToInsert = attributes;
    QMap<QString, int> attributeMap;
    while (s2.next()) {
        const QString tmp = s2.text(1);
        attributeMap.insert(tmp,s2.toInt(0));
        if (idsToInsert.contains(tmp))
            idsToInsert.removeAll(tmp);
    }

    foreach (const QString &id, idsToInsert) {
        Db::Stm s3(d_db, "INSERT INTO FilterAttributeTable VALUES(NULL, ?)");
        s3.bind(0, id);
        s3.next();
        attributeMap.insert(id, d_db->lastId());
    }

    if (nameId < 0) {
        Db::Stm s4(d_db, "INSERT INTO FilterNameTable VALUES(NULL, ?)");
        s4.bind(0, filterName);
        if (s4.exec())
            nameId = d_db->lastId();
    }

    if (nameId < 0) {
        emit error(tr("Cannot register filter %1.").arg(filterName));
        return false;
    }

    Db::Stm s5(d_db, "DELETE FROM FilterTable WHERE NameId=?");
    s5.bind(0, nameId);
    s5.next();

    foreach (const QString &att, attributes) {
        Db::Stm s6(d_db, "INSERT INTO FilterTable VALUES(?, ?)");
        s6.bind(0, nameId);
        s6.bind(1, attributeMap[att]);
        if( !s6.exec() )
            return false;
    }
    return true;
}

QHelpCollectionHandler::DocInfoList QHelpCollectionHandler::registeredDocumentations() const
{
    DocInfoList list;
    if (m_dbOpened) {
        Db::Stm s(d_db, "SELECT a.Name, a.FilePath, b.Name "
            "FROM NamespaceTable a, FolderTable b WHERE a.Id=b.NamespaceId");

        while (s.next()) {
            DocInfo info;
            info.fileName = s.text(1);
            info.folderName = s.text(2);
            info.namespaceName = s.text(0);
            list.append(info);
        }
    }
    return list;
}

bool QHelpCollectionHandler::registerDocumentation(const QString &fileName)
{
    if (!isDBOpened())
        return false;

    QHelpDBReader reader(fileName, QHelpGlobal::uniquifyConnectionName(
        QLatin1String("QHelpCollectionHandler"), this), 0);
    if (!reader.init()) {
        emit error(tr("Cannot open documentation file %1.").arg(fileName));
        return false;
    }

    QString ns = reader.namespaceName();
    if (ns.isEmpty()) {
        emit error(tr("Invalid documentation file '%1'.").arg(fileName));
        return false;
    }

    int nsId = registerNamespace(ns, fileName);
    if (nsId < 1)
        return false;

    if (!registerVirtualFolder(reader.virtualFolder(), nsId))
        return false;

    addFilterAttributes(reader.filterAttributes());
    foreach (const QString &filterName, reader.customFilters())
        addCustomFilter(filterName, reader.filterAttributes(filterName));

    optimizeDatabase(fileName);

    return true;
}

bool QHelpCollectionHandler::unregisterDocumentation(const QString &namespaceName)
{
    if (!isDBOpened())
        return false;

    Db::Stm s1(d_db, "SELECT Id FROM NamespaceTable WHERE Name=?");
    s1.bind(0, namespaceName);

    int nsId = -1;
    if (s1.next())
        nsId = s1.toInt(0);

    if (nsId < 0) {
        emit error(tr("The namespace %1 was not registered.").arg(namespaceName));
        return false;
    }

    Db::Stm s2(d_db, "DELETE FROM NamespaceTable WHERE Id=?");
    s2.bind(0, nsId);
    s2.exec();

    Db::Stm s3(d_db, "DELETE FROM FolderTable WHERE NamespaceId=?");
    s3.bind(0, nsId);
    return s3.exec();
}

bool QHelpCollectionHandler::removeCustomValue(const QString &key)
{
    if (!isDBOpened())
        return false;

    Db::Stm s1(d_db, "DELETE FROM SettingsTable WHERE Key=?");
    s1.bind(0, key);
    return s1.exec();
}

QVariant QHelpCollectionHandler::customValue(const QString &key,
                                             const QVariant &defaultValue) const
{
    QVariant value = defaultValue;
    if (m_dbOpened) {
        Db::Stm s1(d_db, "SELECT COUNT(Key) FROM SettingsTable WHERE Key=?");
        s1.bind(0, key);
        if (!s1.next() || !s1.toInt(0)) {
            return defaultValue;
        }

        Db::Stm s2(d_db, "SELECT Value FROM SettingsTable WHERE Key=?");
        s2.bind(0, key);
        if (s2.next())
            value = s2.byteArray(0);
    }
    return value;
}

bool QHelpCollectionHandler::setCustomValue(const QString &key,
                                            const QVariant &value)
{
    if (!isDBOpened())
        return false;

    Db::Stm s1(d_db, "SELECT Value FROM SettingsTable WHERE Key=?");
    s1.bind(0, key);
    if (s1.next()) {
        Db::Stm s2(d_db, "UPDATE SettingsTable SET Value=? where Key=?");
        s2.bind(0, value.toByteArray());
        s2.bind(1, key);
        return s2.exec();
    }
    else {
        Db::Stm s3(d_db, "INSERT INTO SettingsTable VALUES(?, ?)");
        s3.bind(0, key);
        s3.bind(1, value.toByteArray());
        return s3.exec();
    }
    return false;
}

bool QHelpCollectionHandler::addFilterAttributes(const QStringList &attributes)
{
    if (!isDBOpened())
        return false;

    Db::Stm s1(d_db, "SELECT Name FROM FilterAttributeTable");
    QSet<QString> atts;
    while (s1.next())
        atts.insert(s1.text(0));

    foreach (const QString &s, attributes) {
        if (!atts.contains(s)) {
            Db::Stm s2(d_db, "INSERT INTO FilterAttributeTable VALUES(NULL, ?)");
            s2.bind(0, s);
            s2.exec();
        }
    }
    return true;
}

QStringList QHelpCollectionHandler::filterAttributes() const
{
    QStringList list;
    if (m_dbOpened) {
        Db::Stm s1(d_db, "SELECT Name FROM FilterAttributeTable");
        while (s1.next())
            list.append(s1.text(0));
    }
    return list;
}

QStringList QHelpCollectionHandler::filterAttributes(const QString &filterName) const
{
    QStringList list;
    if (m_dbOpened) {
        Db::Stm s1(d_db, "SELECT a.Name FROM FilterAttributeTable a, "
            "FilterTable b, FilterNameTable c WHERE a.Id=b.FilterAttributeId "
            "AND b.NameId=c.Id AND c.Name=?");
        s1.bind(0, filterName);
        while (s1.next())
            list.append(s1.text(0));
    }
    return list;
}

int QHelpCollectionHandler::registerNamespace(const QString &nspace, const QString &fileName)
{
    Db::Stm s1(d_db, "SELECT COUNT(Id) FROM NamespaceTable WHERE Name=?");
    s1.bind(0, nspace);
    while (s1.next()) {
        if (s1.toInt(0) > 0) {
            emit error(tr("Namespace %1 already exists.").arg(nspace));
            return -1;
        }
    }

    QFileInfo fi(m_collectionFile);
    Db::Stm s2(d_db, "INSERT INTO NamespaceTable VALUES(NULL, ?, ?)");
    s2.bind(0, nspace);
    s2.bind(1, fi.absoluteDir().relativeFilePath(fileName));
    int namespaceId = -1;
    if (s2.exec())
        namespaceId = d_db->lastId();
    if (namespaceId < 1) {
        emit error(tr("Cannot register namespace '%1'.").arg(nspace));
        return -1;
    }
    return namespaceId;
}

bool QHelpCollectionHandler::registerVirtualFolder(const QString &folderName, int namespaceId)
{
    Db::Stm s1(d_db, "INSERT INTO FolderTable VALUES(NULL, ?, ?)");
    s1.bind(0, namespaceId);
    s1.bind(1, folderName);
    return s1.exec();
}

void QHelpCollectionHandler::optimizeDatabase(const QString &fileName)
{
    if (!QFile::exists(fileName))
        return;

    Db db2;
    bool openingOk = sqlite3_open( fileName.toUtf8(), &db2.db) == SQLITE_OK;
    if (!openingOk) {
        sqlite3_close(db2.db);
        emit error(tr("Cannot open database '%1' to optimize.").arg(fileName));
        return;
    }

    db2.exec("PRAGMA synchronous=OFF");
    db2.exec("PRAGMA cache_size=3000");
    db2.exec("CREATE INDEX IF NOT EXISTS NameIndex ON IndexTable(Name)");
    db2.exec("CREATE INDEX IF NOT EXISTS FileNameIndex ON FileNameTable(Name)");
    db2.exec("CREATE INDEX IF NOT EXISTS FileIdIndex ON FileNameTable(FileId)");
}

QT_END_NAMESPACE
