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

#include "busyLexer.h"
extern "C"{
#include "bslex.h"
}
using namespace busy;

Lexer::Lexer():d_lex(0)
{

}

Lexer::~Lexer()
{
    if( d_lex )
        bslex_free(d_lex);
}

void Lexer::setStream(const QByteArray& code, const QString& sourcePath)
{
    if( d_lex )
        bslex_free(d_lex);
    d_lex = 0;
    d_code = code;
    d_sourcePath = sourcePath;
    d_lex = bslex_openFromString(d_code.constData(), d_code.size(), d_sourcePath.toUtf8().constData());
}

Token Lexer::peekToken(int off)
{
    Token res;
    if( d_lex )
    {
        BSToken t = bslex_peek(d_lex,off);
        res.d_lineNr = t.loc.row;
        if( t.loc.row )
            res.d_colNr = t.loc.col + 1;
        res.d_sourcePath = d_sourcePath;
        res.d_type = t.tok;
        res.d_val = QByteArray::fromRawData(t.val,t.len);
        res.d_uniLen = bslex_numOfUnichar(t.val,t.len);
    }
    return res;
}

Token Lexer::nextToken()
{
    Token res;
    if( d_lex )
    {
        BSToken t = bslex_next(d_lex);
        res.d_lineNr = t.loc.row;
        if( t.loc.row )
            res.d_colNr = t.loc.col + 1;
        res.d_sourcePath = d_sourcePath;
        res.d_type = t.tok;
        res.d_val = QByteArray::fromRawData(t.val,t.len);
        res.d_uniLen = bslex_numOfUnichar(t.val,t.len);
    }
    return res;
}

QList<Token> Lexer::tokens(const QByteArray& code, const QString& sourcePath)
{
    setStream( code, sourcePath );
    bslex_mute(d_lex);
    bslex_alltokens(d_lex);
    QList<Token> res;
    Token t = nextToken();
    while( t.isValid() )
    {
        res << t;
        t = nextToken();
    }
    return res;
}

