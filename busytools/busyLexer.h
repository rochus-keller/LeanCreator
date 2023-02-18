#ifndef LC_BUSY_LEXER
#define LC_BUSY_LEXER

/*
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
*/

#include <busytools/busyToken.h>
#include <QList>

struct BSLexer;

namespace busy
{

class Lexer
{
public:
    Lexer();
    ~Lexer();

    void setStream( const QByteArray& code, const QString& sourcePath );

    Token peekToken(int off);
	Token nextToken();
    QList<Token> tokens( const QByteArray& code, const QString& sourcePath = QString() );
private:
    QByteArray d_code;
    QString d_sourcePath;
    BSLexer* d_lex;
};

}

#endif
