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

#include "busyparser.h"
#include <busy/busyLexer.h>

#include <projectexplorer/task.h>

#include <utils/fileutils.h>

#include <QFileInfo>

namespace BusyProjectManager {
namespace Internal {

BusyParser::BusyParser()
{
    setObjectName(QLatin1String("BusyParser"));
}

void BusyParser::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = QDir(workingDirectory);
    IOutputParser::setWorkingDirectory(workingDirectory);
}

void BusyParser::taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skipLines)
{
    ProjectExplorer::Task editable(task);

    QString filePath = task.file.toString();

    if (!filePath.isEmpty())
        editable.file = Utils::FileName::fromUserInput(m_workingDirectory.absoluteFilePath(filePath));

    IOutputParser::taskAdded(editable, linkedLines, skipLines);
}

class BusyParamParser::Imp : public QObject // for tr()
{
public:
    bool parse( const QByteArray& code )
    {
        d_params.clear();
        d_err.clear();
        this->code = code;
        lex.setStream(code, QString());
        try
        {
            while( lex.peekToken(1).isValid() )
                assignment();
        }catch(...)
        {

        }
        return d_err.isEmpty();
    }
    QList<QPair<QByteArray,QByteArray> > d_params;
    QString d_err;
    int row, col,len;
private:
    QByteArray code;
    void assignment()
    {
        QByteArray name = desig();
        busy::Token t = lex.peekToken(1);
        QByteArray value = "true";
        if( t.d_type == Tok_Eq || t.d_type == Tok_ColonEq )
        {
            lex.nextToken();
            value = factor();
        }
        d_params << qMakePair(name,value);

        t = lex.peekToken(1);
        if( t.d_type == Tok_Semi )
            lex.nextToken();
    }
    QByteArray desig()
    {
        QByteArray res;
        busy::Token t = lex.nextToken();
        if( t.d_type != Tok_ident )
            error(tr("expecting identifier"), t.d_lineNr, t.d_colNr,t.d_val.size());
        else
            res += t.d_val;
        t = lex.peekToken(1);
        while( t.d_type == Tok_Dot )
        {
            lex.nextToken();
            res += ".";
            t = lex.nextToken();
            if( t.d_type != Tok_ident )
                error(tr("invalid designator"), t.d_lineNr, t.d_colNr,t.d_val.size());
            else
                res += t.d_val;
            t = lex.peekToken(1);
        }
        return res;
    }
    QByteArray factor()
    {
        busy::Token t = lex.nextToken();
        switch( t.d_type )
        {
        case Tok_string:
        case Tok_integer:
        case Tok_real:
        case Tok_path:
        case Tok_symbol:
            return QByteArray(t.d_val.constData(), t.d_val.size() ); // force a copy of the raw array
        case Tok_true:
            return "true";
        case Tok_false:
            return "false";
        default:
            error(tr("invalid factor"),t.d_lineNr, t.d_colNr,t.d_val.size());
        }
        return QByteArray();
    }
    void error( const QString& msg, int r, int c, int l )
    {
        d_err = msg;
        col = c;
        row = r;
        len = l;
        throw 0;
    }
    busy::Lexer lex;
};

BusyParamParser::Result BusyParamParser::parse(const QString& params)
{
    Imp imp;
    Result res;
    if( !imp.parse( params.toUtf8() ) )
    {
        res.d_err = imp.d_err;
        res.d_row = imp.row;
        res.d_col = imp.col;
        res.d_len = imp.len;
    }
    res.d_params = imp.d_params;
    return res;
}

BusyTargetParser::Result BusyTargetParser::parse(const QString& targets)
{
    Result res;
    const QByteArray code = targets.toUtf8();
    busy::Lexer lex;
    lex.setStream(code,QString());
    busy::Token t = lex.nextToken();
    while( t.isValid() )
    {
        if( t.d_type == Tok_ident )
            res.d_targets.append( QByteArray(t.d_val.constData(), t.d_val.size() ) ); // force a copy of the raw array
        else
        {
            res.d_err = tr("invalid identifier: %1").arg( code.mid(t.d_colNr).constData());
            res.d_col = t.d_colNr;
            res.d_row = t.d_lineNr;
            res.d_len = t.d_val.size();
            return res;
        }
        t = lex.nextToken();
    }
    if( t.d_type == Tok_Invalid )
    {
        res.d_err = tr("invalid syntax: %1").arg( code.mid(t.d_colNr).constData());
        res.d_col = t.d_colNr;
        res.d_row = t.d_lineNr;
        res.d_len = t.d_val.size();
    }
    return res;
}

} // namespace Internal
} // namespace BusyProjectManager

