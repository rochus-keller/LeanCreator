/****************************************************************************
**
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
****************************************************************************/

#include "database.h"
#include "sqlite3.h"
using namespace Utils;

static int count = 0;

Database::Database():db(0)
{
    if( count == 0 )
        sqlite3_initialize();
    count++;
}

Database::~Database()
{
    sqlite3_close(db);
    count--;
    if( count == 0 )
        sqlite3_shutdown();
}

bool Database::open(const QString &path, bool readonly)
{
    bool openingOk = false;
    if( readonly )
        openingOk = sqlite3_open_v2( path.toUtf8().constData(), &db, SQLITE_OPEN_READONLY, 0) == SQLITE_OK;
    else
        openingOk = sqlite3_open( path.toUtf8().constData(), &db) == SQLITE_OK;
    if (!openingOk)
        sqlite3_close(db);
    return openingOk;
}

bool Database::exec( const QByteArray& sql )
{
    return sqlite3_exec(db, sql.constData(), 0, 0, 0 ) == SQLITE_OK;
}

int Database::lastId()
{
    return sqlite3_last_insert_rowid(db);
}

Query::Query(Database * db, const QByteArray &sql)
{
    sqlite3_prepare( db->db, sql.constData(), sql.size(), &s, 0 );

}

Query::~Query()
{
    sqlite3_finalize(s);
}

bool Query::next()
{
    return sqlite3_step(s) == SQLITE_ROW;
}

bool Query::exec()
{
    const int res = sqlite3_step(s);
    return res == SQLITE_ROW || res == SQLITE_DONE;
}

QString Query::text(int col) const
{
    const char* str = (const char*)sqlite3_column_text(s, col);
    const int len = sqlite3_column_bytes(s, col);
    return QString::fromUtf8(str,len);
}

int Query::number(int col) const
{
    return sqlite3_column_int(s,col);
}

QByteArray Query::bytes(int col) const
{
    const void* data = sqlite3_column_blob(s, col);
    const int len = sqlite3_column_bytes(s, col);
    return QByteArray((const char*)data,len);
}

void Query::bind(int col, const QString& str)
{
    const QByteArray tmp = str.toUtf8();
    sqlite3_bind_text(s, col + 1, tmp.constData(), tmp.size(), SQLITE_TRANSIENT);
}

void Query::bind(int col, const QByteArray& str)
{
    sqlite3_bind_blob(s, col + 1, str.constData(), str.size(), SQLITE_TRANSIENT);
}

void Query::bind(int col, int i)
{
    sqlite3_bind_int(s, col + 1, i);
}
