/*
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
*/

#include "busyhighlighter.h"
#include <busytools/busyLexer.h>
#include <texteditor/textdocumentlayout.h>
#include <QtDebug>
using namespace TextEditor;
using namespace BusyProjectManager;
using namespace busy;

Highlighter::Highlighter(QTextDocument* parent) :
    SyntaxHighlighter(parent)
{
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        d_format[i].setForeground(Qt::black);
        d_format[i].setBackground(Qt::transparent);
    }

    // d_format[C_Ident].setForeground(QColor(143, 31, 31));

    d_format[C_Num].setForeground(Qt::cyan); // QColor(10, 10, 133));

    d_format[C_Str].setForeground(QColor(35, 145, 35)); // QColor(208, 16, 64));

    d_format[C_Cmt].setForeground(QColor(153, 153, 136));

    d_format[C_Kw].setForeground(QColor(147, 147, 39));
    // d_format[C_Kw].setFontWeight(QFont::Bold);

    d_format[C_Op].setForeground(Qt::darkBlue);
    // d_format[C_Op].setFontWeight(QFont::Bold);

    d_format[C_Sym].setForeground(QColor(131, 7, 131));
    //d_format[C_Sym].setFontWeight(QFont::Bold);

    d_format[C_Path].setForeground(QColor(143, 31, 31)); //QColor(35, 145, 35)); // QColor(0, 128, 0)
    //d_format[C_Path].setFontWeight(QFont::Bold);    
}

QTextCharFormat Highlighter::formatForCategory(int c) const
{
    return d_format[c];
}

static int endOfComment( const QString& str, int& pos, int& level )
{
    enum State { Idle, Slash, Star } state = Idle;
    int found = 0;
    while( pos < str.size() )
    {
        const QChar c = str[pos++];
        switch( state )
        {
        case Idle:
            if( c == '/')
                state = Slash;
            else if( c == '*' )
                state = Star;
            break;
        case Slash:
            if( c == '*' )
            {
                found = 1;
                level++;
            }else if( c != '/' )
                state = Idle; // otherwise //* is not recognized
            break;
        case Star:
            if( c == '/')
            {
                found = 1;
                level--;
            }else if( c != '*' ) // otherwise **/ is not recognized
                state = Idle;
            break;
        }
        if( found && level == 0 )
            break;
    }
    return found;
}

static int endOfString( const QString& str )
{
    int i = 0;
    bool found = false;
    while( i < str.size() )
    {
        QChar ch = str[i++];
        if( ch == '\\' && i < str.size() && ( str[i] == '"' || str[i] == '\\' ) )
            i++; // valid escapes are \\ and \"
        else if( ch == '"' )
        {
            found = true;
            break;
        }
    }
    if( found )
        return i;
    else
        return -1;
}

#define SAVE_STATE( commentLevel, stringLevel, braceDepth ) \
    setCurrentBlockState((stringLevel << 16) | (braceDepth << 8) | commentLevel);

void Highlighter::highlightBlock(const QString& text)
{
    const int previousBlockState_ = previousBlockState();
    int commentLevel = 0, initialBraceDepth = 0, stringLevel = 0;
    if (previousBlockState_ != -1) {
        commentLevel = previousBlockState_ & 0xff;
        stringLevel = 0xff & ( previousBlockState_ >> 16 );
        initialBraceDepth = 0xff & ( previousBlockState_ >> 8 );
    }

    int braceDepth = initialBraceDepth;
    int foldingIndent = initialBraceDepth;

    if (TextBlockUserData *userData = TextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    int start = 0;
    if( commentLevel )
    {
        // we're in a multi-line comment
        // look for the end
        QTextCharFormat f = formatForCategory(C_Cmt);
        int pos = 0;
        endOfComment( text, pos, commentLevel );
        if( commentLevel > 0 )
        {
            // the whole block ist part of the comment
            setFormat( start, text.size(), f );
            SAVE_STATE(commentLevel, stringLevel, braceDepth);
            return;
        }else
        {
            // End of Comment found
            setFormat( start, pos , f );
            start = pos;
            if( commentLevel < 0 )
                qWarning() << "Highlighter::highlightBlock: illegal comment level" << commentLevel;
            commentLevel = 0;
        }
    }else if( stringLevel )
    {
        // we're in a multi-line string
        QTextCharFormat f = formatForCategory(C_Str);
        const int pos = endOfString(text);
        if( pos == -1 )
        {
            // the whole block ist part of the string
            setFormat( start, text.size(), f );
            SAVE_STATE(commentLevel, stringLevel, braceDepth);
            return;
        }else
        {
            // End of string found
            setFormat( start, pos , f );
            stringLevel = 0;
            start = pos;
        }
    }

    Parentheses parentheses;
    parentheses.reserve(20);

    Lexer lex;

    QList<Token> tokens = lex.tokens(text.mid(start).toUtf8());
    int i = 0;
    while( i < tokens.size() )
    {
        Token& t = tokens[i];
        t.d_colNr += start;

        QTextCharFormat f;
        if( t.d_type == Tok_Lcmt )
        {
            f = formatForCategory(C_Cmt);
            int pos = 0;
            endOfComment( QString::fromUtf8(t.d_val), pos, commentLevel );
        }else if( t.d_type == Tok_Hash )
        {
            // Line comment
            f = formatForCategory(C_Cmt);
        }else if( t.d_type == Tok_string )
        {
            f = formatForCategory(C_Str);
            if( !t.d_val.endsWith('"') )
            {
                // start of a multi-line string
                stringLevel = 1;
            }
        }else if( t.d_type == Tok_integer || t.d_type == Tok_real )
            f = formatForCategory(C_Num);
        else if( t.d_type == Tok_symbol )
            f = formatForCategory(C_Sym);
        else if( t.d_type == Tok_path )
            f = formatForCategory(C_Path);
        else if( t.d_type > TT_Literals && t.d_type < TT_Keywords )
        {
            switch( t.d_type )
            {
            case Tok_Lpar:
            case Tok_Lbrack:
            case Tok_Lbrace:
                parentheses.append(Parenthesis(Parenthesis::Opened, QChar(t.d_val[0]), t.d_colNr-1 ));
                break;
            case Tok_Rpar:
            case Tok_Rbrack:
            case Tok_Rbrace:
                parentheses.append(Parenthesis(Parenthesis::Closed, QChar(t.d_val[0]), t.d_colNr-1 ));
                break;
            }
            switch( t.d_type )
            {
            case Tok_Lbrace:
                ++braceDepth;
                break;
            case Tok_Rbrace:
                --braceDepth;
                break;
            }
            f = formatForCategory(C_Op);
        }else if( t.d_type > TT_Keywords && t.d_type < TT_Specials )
        {
#if 0
            if( tokenIsBlockBegin(t.d_type) ) // then, else if not followed by {
            {
                parentheses.append(Parenthesis(Parenthesis::Opened, text[t.d_colNr-1], t.d_colNr-1 ));
                ++braceDepth;
                // if a folding block opens at the beginning of a line, treat the entire line
                // as if it were inside the folding block
                if ( i == 0 && t.d_type == Tok_begin )
                {
                    ++foldingIndent;
                    TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                }
            }else if( tokenIsBlockEnd(t.d_type) )
            {
                const int pos = t.d_colNr-1 + ::strlen(tokenToString(t.d_type))-1;
                parentheses.append(Parenthesis(Parenthesis::Closed, text[pos], pos ));
                --braceDepth;
                if (braceDepth < foldingIndent) {
                    // unless we are at the end of the block, we reduce the folding indent
                    if (i == tokens.size()-1 || tokens.at(i+1).d_type == Tok_Semi )
                        TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                    else
                        foldingIndent = qMin(braceDepth, foldingIndent);
                }
            }
#endif
            f = formatForCategory(C_Kw);
        }else if( t.d_type == Tok_ident )
            f = formatForCategory(C_Ident);

        if( f.isValid() )
        {
            setFormat( t.d_colNr-1, t.d_uniLen, f );
        }
        i++;
    }

    TextDocumentLayout::setParentheses(currentBlock(), parentheses);
    // if the block is ifdefed out, we only store the parentheses, but

    // do not adjust the brace depth.
    if (TextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
    SAVE_STATE(commentLevel, stringLevel, braceDepth);
}



