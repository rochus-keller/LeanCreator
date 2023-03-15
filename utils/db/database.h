#ifndef UTILS_DATABASE_H
#define UTILS_DATABASE_H

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

#include <QString>

extern "C" {
struct sqlite3;
struct sqlite3_stmt;
}
namespace Utils {

class Database {
public:
    Database();
    ~Database();

    bool open( const QString& path, bool readonly = false );
    bool ok() const { return db != 0; }
    bool exec( const QByteArray& sql );
    int lastId();
private:
    sqlite3* db;
    friend class Query;
};

class Query {
public:
    Query(Database*, const QByteArray& sql );
    ~Query();

    bool next();
    bool exec();
    QString text(int col) const;
    int number(int col) const;
    QByteArray bytes(int col) const;
    void bind(int col, const QString& str);
    void bind(int col, const QByteArray& str);
    void bind(int col, int i);
private:
    sqlite3_stmt* s;
};

}

#endif // UTILS_DATABASE_H
