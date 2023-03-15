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

#include "qhelpdbreader_p.h"
#include "qhelp_global.h"

#include <QtCore/QVariant>
#include <QtCore/QFile>
#include <QtDebug>

#include "sqlite3.h"

QT_BEGIN_NAMESPACE

class QHelpDBReader::Db {
public:
    Db():db(0){}
    ~Db()
    {
        sqlite3_close(db);
    }

    sqlite3* db;

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
    };
};

QHelpDBReader::QHelpDBReader(const QString &dbName)
    : QObject(0),d_db(0)
{
    initObject(dbName,
        QHelpGlobal::uniquifyConnectionName(QLatin1String("QHelpDBReader"),
        this));
}

QHelpDBReader::QHelpDBReader(const QString &dbName, const QString &uniqueId,
                           QObject *parent)
    : QObject(parent),d_db(0)
{
    initObject(dbName, uniqueId);
}

void QHelpDBReader::initObject(const QString &dbName, const QString &uniqueId)
{
    m_dbName = dbName;
    m_uniqueId = uniqueId;
    m_initDone = false;
    m_useAttributesCache = false;
    d_db = new Db;
}

QHelpDBReader::~QHelpDBReader()
{
    if( d_db )
        delete d_db;
}

bool QHelpDBReader::init()
{
    if (m_initDone)
        return true;

    if (!QFile::exists(m_dbName))
        return false;

    if( sqlite3_open_v2( m_dbName.toUtf8().constData(), &d_db->db, SQLITE_OPEN_READONLY, 0) != SQLITE_OK )
    {
        /*: The placeholders are: %1 - The name of the database which cannot be opened
                                  %2 - The unique id for the connection */
        m_error = tr("Cannot open database '%1' '%2'").arg(m_dbName, m_uniqueId);
        sqlite3_close(d_db->db);
        return false;
    }
    m_initDone = true;

    return true;
}

QString QHelpDBReader::databaseName() const
{
    return m_dbName;
}

QString QHelpDBReader::errorMessage() const
{
    return m_error;
}

QString QHelpDBReader::namespaceName() const
{
    if (!m_namespace.isEmpty())
        return m_namespace;
    if (d_db->db) {
        Db::Stm s(d_db, "SELECT Name FROM NamespaceTable");
        if( s.next() )
            m_namespace = s.text(0);
    }
    return m_namespace;
}

QString QHelpDBReader::virtualFolder() const
{
    if (d_db->db) {
        Db::Stm s(d_db, "SELECT Name FROM FolderTable WHERE Id=1");
        if( s.next() )
            return s.text(0);
    }
    return QString();
}

QList<QStringList> QHelpDBReader::filterAttributeSets() const
{
    QList<QStringList> result;
    if (d_db->db) {
        Db::Stm s(d_db,"SELECT a.Id, b.Name FROM FileAttributeSetTable a, "
            "FilterAttributeTable b WHERE a.FilterAttributeId=b.Id ORDER BY a.Id");
        int oldId = -1;
        while (s.next()) {
            int id = s.toInt(0);
            if (id != oldId) {
                result.append(QStringList());
                oldId = id;
            }
            result.last().append(s.text(1));
        }
    }
    return result;
}

bool QHelpDBReader::fileExists(const QString &virtualFolder,
                               const QString &filePath,
                               const QStringList &filterAttributes) const
{
    if (virtualFolder.isEmpty() || filePath.isEmpty() || !d_db->db)
        return false;

//SELECT COUNT(a.Name) FROM FileNameTable a, FolderTable b, FileFilterTable c, FilterAttributeTable d WHERE a.FolderId=b.Id AND b.Name='qtdoc' AND a.Name='qstring.html' AND a.FileId=c.FileId AND c.FilterAttributeId=d.Id AND d.Name='qtrefdoc'

    QString query;
    namespaceName();
    if (filterAttributes.isEmpty()) {
        query = QString(QLatin1String("SELECT COUNT(a.Name) FROM FileNameTable a, FolderTable b "
            "WHERE a.FolderId=b.Id AND b.Name=\'%1\' AND a.Name=\'%2\'")).arg(quote(virtualFolder)).arg(quote(filePath));
    } else {
        query = QString(QLatin1String("SELECT COUNT(a.Name) FROM FileNameTable a, FolderTable b, "
            "FileFilterTable c, FilterAttributeTable d WHERE a.FolderId=b.Id "
            "AND b.Name=\'%1\' AND a.Name=\'%2\' AND a.FileId=c.FileId AND "
            "c.FilterAttributeId=d.Id AND d.Name=\'%3\'"))
            .arg(quote(virtualFolder)).arg(quote(filePath))
            .arg(quote(filterAttributes.first()));
        for (int i=1; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT COUNT(a.Name) FROM FileNameTable a, "
                "FolderTable b, FileFilterTable c, FilterAttributeTable d WHERE a.FolderId=b.Id "
                "AND b.Name=\'%1\' AND a.Name=\'%2\' AND a.FileId=c.FileId AND "
                "c.FilterAttributeId=d.Id AND d.Name=\'%3\'"))
                .arg(quote(virtualFolder)).arg(quote(filePath))
                .arg(quote(filterAttributes.at(i))));
        }
    }
    Db::Stm s(d_db, query.toUtf8());
    if (s.next() && s.toInt(0))
        return true;
    return false;
}

QByteArray QHelpDBReader::fileData(const QString &virtualFolder,
                                   const QString &filePath) const
{
    QByteArray ba;
    if (virtualFolder.isEmpty() || filePath.isEmpty() || !d_db->db)
        return ba;

    namespaceName();
    Db::Stm s(d_db, "SELECT a.Data FROM FileDataTable a, FileNameTable b, FolderTable c, "
        "NamespaceTable d WHERE a.Id=b.FileId AND (b.Name=? OR b.Name=?) AND b.FolderId=c.Id "
        "AND c.Name=? AND c.NamespaceId=d.Id AND d.Name=?");
    s.bind(0, filePath);
    s.bind(1, QString(QLatin1String("./") + filePath));
    s.bind(2, virtualFolder);
    s.bind(3, m_namespace);
    if (s.next() )
        ba = qUncompress(s.byteArray(0));
    return ba;
}

QStringList QHelpDBReader::customFilters() const
{
    QStringList lst;
    if (d_db->db) {
        Db::Stm s(d_db,"SELECT Name FROM FilterNameTable");
        while (s.next())
            lst.append(s.text(0));
    }
    return lst;
}

QStringList QHelpDBReader::filterAttributes(const QString &filterName) const
{
    QStringList lst;
    if (d_db->db) {
        QByteArray sql;
        if (filterName.isEmpty()) {
            sql = "SELECT Name FROM FilterAttributeTable";
        } else {
            sql = "SELECT a.Name FROM FilterAttributeTable a, "
                "FilterTable b, FilterNameTable c WHERE c.Name=? "
                "AND c.Id=b.NameId AND b.FilterAttributeId=a.Id";
        }
        Db::Stm s(d_db,sql);
        if( !filterName.isEmpty() )
            s.bind(0, filterName);
        while (s.next())
            lst.append(s.text(0));
    }
    return lst;
}

QStringList QHelpDBReader::indicesForFilter(const QStringList &filterAttributes) const
{
    QStringList indices;
    if (!d_db->db)
        return indices;

    //SELECT DISTINCT a.Name FROM IndexTable a, IndexFilterTable b, FilterAttributeTable c WHERE a.Id=b.IndexId AND b.FilterAttributeId=c.Id AND c.Name in ('4.2.3', 'qt')

    QString query;
    if (filterAttributes.isEmpty()) {
        query = QLatin1String("SELECT DISTINCT Name FROM IndexTable");
    } else {
        query = QString(QLatin1String("SELECT DISTINCT a.Name FROM IndexTable a, "
        "IndexFilterTable b, FilterAttributeTable c WHERE a.Id=b.IndexId "
        "AND b.FilterAttributeId=c.Id AND c.Name='%1'")).arg(quote(filterAttributes.first()));
        for (int i=1; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT DISTINCT a.Name FROM IndexTable a, "
                "IndexFilterTable b, FilterAttributeTable c WHERE a.Id=b.IndexId "
                "AND b.FilterAttributeId=c.Id AND c.Name='%1'"))
                .arg(quote(filterAttributes.at(i))));
        }
    }

    Db::Stm s(d_db,query.toUtf8());
    while (s.next()) {
        const QString tmp = s.text(0);
        if (!tmp.isEmpty())
            indices.append(tmp);
    }
    return indices;
}

void QHelpDBReader::linksForKeyword(const QString &keyword, const QStringList &filterAttributes,
                                    QMap<QString, QUrl> &linkMap) const
{
    if (!d_db->db)
        return;

    QString query;
    if (filterAttributes.isEmpty()) {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor "
            "FROM IndexTable a, FileNameTable d, "
            "FolderTable e, NamespaceTable f WHERE "
            "a.FileId=d.FileId AND d.FolderId=e.Id AND a.NamespaceId=f.Id "
            "AND a.Name='%1'")).arg(quote(keyword));
    } else if (m_useAttributesCache) {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor, a.Id "
            "FROM IndexTable a, "
            "FileNameTable d, FolderTable e, NamespaceTable f WHERE "
            "a.FileId=d.FileId AND d.FolderId=e.Id "
            "AND a.NamespaceId=f.Id AND a.Name='%1'"))
            .arg(quote(keyword));
        Db::Stm s(d_db, query.toUtf8());
        while (s.next()) {
            const int tmp = s.toInt(5);
            if (m_indicesCache.contains(tmp)) {
                linkMap.insertMulti(s.text(0), buildQUrl(s.text(1),
                    s.text(2), s.text(3), s.text(4)));
            }
        }
        return;
    } else {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor "
            "FROM IndexTable a, IndexFilterTable b, FilterAttributeTable c, "
            "FileNameTable d, FolderTable e, NamespaceTable f "
            "WHERE a.FileId=d.FileId AND d.FolderId=e.Id "
            "AND a.NamespaceId=f.Id AND b.IndexId=a.Id AND b.FilterAttributeId=c.Id "
            "AND a.Name='%1' AND c.Name='%2'")).arg(quote(keyword))
            .arg(quote(filterAttributes.first()));
        for (int i=1; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor "
                "FROM IndexTable a, IndexFilterTable b, FilterAttributeTable c, "
                "FileNameTable d, FolderTable e, NamespaceTable f "
                "WHERE a.FileId=d.FileId AND d.FolderId=e.Id "
                "AND a.NamespaceId=f.Id AND b.IndexId=a.Id AND b.FilterAttributeId=c.Id "
                "AND a.Name='%1' AND c.Name='%2'")).arg(quote(keyword))
                .arg(quote(filterAttributes.at(i))));
        }
    }

    QString title;
    Db::Stm s(d_db, query.toUtf8());
    while (s.next()) {
        title = s.text(0);
        if (title.isEmpty()) // generate a title + corresponding path
            title = keyword + QLatin1String(" : ") + s.text(3);
        linkMap.insertMulti(title, buildQUrl(s.text(1),
            s.text(2), s.text(3), s.text(4)));
    }
}

void QHelpDBReader::linksForIdentifier(const QString &id,
                                       const QStringList &filterAttributes,
                                       QMap<QString, QUrl> &linkMap) const
{
    if (!d_db->db)
        return;

    QString query;
    if (filterAttributes.isEmpty()) {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor "
        "FROM IndexTable a, FileNameTable d, FolderTable e, "
        "NamespaceTable f WHERE a.FileId=d.FileId AND "
        "d.FolderId=e.Id AND a.NamespaceId=f.Id AND a.Identifier='%1'"))
        .arg(quote(id));
    } else if (m_useAttributesCache) {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor, a.Id "
            "FROM IndexTable a,"
            "FileNameTable d, FolderTable e, NamespaceTable f WHERE "
            "a.FileId=d.FileId AND d.FolderId=e.Id "
            "AND a.NamespaceId=f.Id AND a.Identifier='%1'"))
            .arg(quote(id));
        Db::Stm s(d_db,query.toUtf8());
        while (s.next()) {
            if (m_indicesCache.contains(s.toInt(5))) {
                linkMap.insertMulti(s.text(0), buildQUrl(s.text(1),
                    s.text(2), s.text(3),s.text(4)));
            }
        }
        return;
    } else {
        query = QString(QLatin1String("SELECT d.Title, f.Name, e.Name, d.Name, a.Anchor "
            "FROM IndexTable a, IndexFilterTable b, FilterAttributeTable c, "
            "FileNameTable d, FolderTable e, NamespaceTable f "
            "WHERE a.FileId=d.FileId AND d.FolderId=e.Id "
            "AND a.NamespaceId=f.Id AND b.IndexId=a.Id AND b.FilterAttributeId=c.Id "
            "AND a.Identifier='%1' AND c.Name='%2'")).arg(quote(id))
            .arg(quote(filterAttributes.first()));
        for (int i=0; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT d.Title, f.Name, e.Name, "
                "d.Name, a.Anchor FROM IndexTable a, IndexFilterTable b, "
                "FilterAttributeTable c, FileNameTable d, "
                "FolderTable e, NamespaceTable f WHERE "
                "a.FileId=d.FileId AND d.FolderId=e.Id AND a.NamespaceId=f.Id "
                "AND b.IndexId=a.Id AND b.FilterAttributeId=c.Id AND "
                "a.Identifier='%1' AND c.Name='%2'")).arg(quote(id))
                .arg(quote(filterAttributes.at(i))));
        }
    }

    Db::Stm s(d_db,query.toUtf8());
    while (s.next()) {
        linkMap.insertMulti(s.text(0), buildQUrl(s.text(1),
            s.text(2), s.text(3),s.text(4)));
    }
}

QUrl QHelpDBReader::buildQUrl(const QString &ns, const QString &folder,
                              const QString &relFileName, const QString &anchor) const
{
    QUrl url;
    url.setScheme(QLatin1String("qthelp"));
    url.setAuthority(ns);
    url.setPath(QLatin1Char('/') + folder + QLatin1Char('/') + relFileName);
    url.setFragment(anchor);
    return url;
}

QList<QByteArray> QHelpDBReader::contentsForFilter(const QStringList &filterAttributes) const
{
    QList<QByteArray> contents;
    if (!d_db->db)
        return contents;

    //SELECT DISTINCT a.Data FROM ContentsTable a, ContentsFilterTable b, FilterAttributeTable c WHERE a.Id=b.ContentsId AND b.FilterAttributeId=c.Id AND c.Name='qt' INTERSECT SELECT DISTINCT a.Data FROM ContentsTable a, ContentsFilterTable b, FilterAttributeTable c WHERE a.Id=b.ContentsId AND b.FilterAttributeId=c.Id AND c.Name='3.3.8';

    QString query;
    if (filterAttributes.isEmpty()) {
        query = QLatin1String("SELECT Data from ContentsTable");
    } else {
        query = QString(QLatin1String("SELECT a.Data FROM ContentsTable a, "
            "ContentsFilterTable b, FilterAttributeTable c "
            "WHERE a.Id=b.ContentsId AND b.FilterAttributeId=c.Id "
            "AND c.Name='%1'")).arg(quote(filterAttributes.first()));
        for (int i=1; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT a.Data FROM ContentsTable a, "
            "ContentsFilterTable b, FilterAttributeTable c "
            "WHERE a.Id=b.ContentsId AND b.FilterAttributeId=c.Id "
            "AND c.Name='%1'")).arg(quote(filterAttributes.at(i))));
        }
    }

    Db::Stm s(d_db,query.toUtf8());
    while (s.next()) {
        contents.append(s.byteArray(0));
    }
    return contents;
}

QUrl QHelpDBReader::urlOfPath(const QString &relativePath) const
{
    QUrl url;
    if (!d_db->db)
        return url;

    Db::Stm s(d_db, "SELECT a.Name, b.Name FROM NamespaceTable a, "
        "FolderTable b WHERE a.id=b.NamespaceId and a.Id=1");
    if (s.next()) {
        QString rp = relativePath;
        QString anchor;
        int i = rp.indexOf(QLatin1Char('#'));
        if (i > -1) {
            rp = relativePath.left(i);
            anchor = relativePath.mid(i+1);
        }
        url = buildQUrl(s.text(0),s.text(1), rp, anchor);
    }
    return url;
}

QStringList QHelpDBReader::files(const QStringList &filterAttributes,
                                 const QString &extensionFilter) const
{
    QStringList lst;
    if (!d_db->db)
        return lst;

    QString query;
    QString extension;
    if (!extensionFilter.isEmpty())
        extension = QString(QLatin1String("AND b.Name like \'%.%1\'")).arg(extensionFilter);

    if (filterAttributes.isEmpty()) {
        query = QString(QLatin1String("SELECT a.Name, b.Name FROM FolderTable a, "
            "FileNameTable b WHERE b.FolderId=a.Id %1"))
            .arg(extension);
    } else {
        query = QString(QLatin1String("SELECT a.Name, b.Name FROM FolderTable a, "
            "FileNameTable b, FileFilterTable c, FilterAttributeTable d "
            "WHERE b.FolderId=a.Id AND b.FileId=c.FileId "
            "AND c.FilterAttributeId=d.Id AND d.Name=\'%1\' %2"))
            .arg(quote(filterAttributes.first())).arg(extension);
        for (int i=1; i<filterAttributes.count(); ++i) {
            query.append(QString(QLatin1String(" INTERSECT SELECT a.Name, b.Name FROM "
                "FolderTable a, FileNameTable b, FileFilterTable c, "
                "FilterAttributeTable d WHERE b.FolderId=a.Id AND "
                "b.FileId=c.FileId AND c.FilterAttributeId=d.Id AND "
                "d.Name=\'%1\' %2")).arg(quote(filterAttributes.at(i)))
                .arg(extension));
        }
    }
    Db::Stm s(d_db,query.toUtf8());
    while (s.next()) {
        lst.append(s.text(0) + QLatin1Char('/')
            + s.text(1));
    }
    return lst;
}

QVariant QHelpDBReader::metaData(const QString &name) const
{
    QVariant v;
    if (!d_db->db)
        return v;

    Db::Stm s(d_db, "SELECT COUNT(Value), Value FROM MetaDataTable "
        "WHERE Name=?");
    s.bind(0, name);
    if (s.next()
        && s.toInt(0) == 1)
        v = s.text(1);
    return v;
}

QString QHelpDBReader::mergeList(const QStringList &list) const
{
    QString str;
    foreach (const QString &s, list)
        str.append(QLatin1Char('\'') + quote(s) + QLatin1String("\', "));
    if (str.endsWith(QLatin1String(", ")))
        str = str.left(str.length()-2);
    return str;
}

QString QHelpDBReader::quote(const QString &string) const
{
    QString s = string;
    s.replace(QLatin1Char('\''), QLatin1String("\'\'"));
    return s;
}

QSet<int> QHelpDBReader::indexIds(const QStringList &attributes) const
{
    QSet<int> ids;

    if (attributes.isEmpty())
        return ids;

    QString query = QString(QLatin1String("SELECT a.IndexId FROM IndexFilterTable a, "
        "FilterAttributeTable b WHERE a.FilterAttributeId=b.Id "
        "AND b.Name='%1'")).arg(attributes.first());
    for (int i=0; i<attributes.count(); ++i) {
        query.append(QString(QLatin1String(" INTERSECT SELECT a.IndexId FROM "
            "IndexFilterTable a, FilterAttributeTable b WHERE "
            "a.FilterAttributeId=b.Id AND b.Name='%1'"))
            .arg(attributes.at(i)));
    }

    Db::Stm s(d_db, query.toUtf8());

    while (s.next())
        ids.insert(s.toInt(0));

    return ids;
}

bool QHelpDBReader::createAttributesCache(const QStringList &attributes,
                                          const QSet<int> &indexIds)
{
    m_useAttributesCache = false;

    if (attributes.count() < 2) {
        m_viewAttributes.clear();
        return true;
    }

    bool needUpdate = !m_viewAttributes.count();

    foreach (const QString &s, attributes)
        m_viewAttributes.remove(s);

    if (m_viewAttributes.count() || needUpdate) {
        m_viewAttributes.clear();
        m_indicesCache = indexIds;
    }
    foreach (const QString &s, attributes)
        m_viewAttributes.insert(s);
    m_useAttributesCache = true;
    return true;
}

QT_END_NAMESPACE
