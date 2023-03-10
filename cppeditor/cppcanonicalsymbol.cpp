/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppcanonicalsymbol.h"

#include <cpptools/cpptoolsreuse.h>
#include <texteditor/convenience.h>

#include <cplusplus/ExpressionUnderCursor.h>

#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace CppEditor {
namespace Internal {

CanonicalSymbol::CanonicalSymbol(const Document::Ptr &document,
                                 const Snapshot &snapshot)
    : m_document(document),
      m_snapshot(snapshot)
{
    m_typeOfExpression.init(document, snapshot);
    m_typeOfExpression.setExpandTemplates(true);
}

const LookupContext &CanonicalSymbol::context() const
{
    return m_typeOfExpression.context();
}

Scope *CanonicalSymbol::getScopeAndExpression(const QTextCursor &cursor, QString *code)
{
    if (!m_document)
        return 0;

    QTextCursor tc = cursor;
    int line, column;
    TextEditor::Convenience::convertPosition(cursor.document(), tc.position(), &line, &column);
    ++column; // 1-based line and 1-based column

    int pos = tc.position();
    QTextDocument *textDocument = cursor.document();
    if (!CppTools::isValidIdentifierChar(textDocument->characterAt(pos)))
        if (!(pos > 0 && CppTools::isValidIdentifierChar(textDocument->characterAt(pos - 1))))
            return 0;

    while (CppTools::isValidIdentifierChar(textDocument->characterAt(pos)))
        ++pos;
    tc.setPosition(pos);

    ExpressionUnderCursor expressionUnderCursor(m_document->languageFeatures());
    *code = expressionUnderCursor(tc);
    return m_document->scopeAt(line, column);
}

Symbol *CanonicalSymbol::operator()(const QTextCursor &cursor)
{
    QString code;

    if (Scope *scope = getScopeAndExpression(cursor, &code))
        return operator()(scope, code);

    return 0;
}

Symbol *CanonicalSymbol::operator()(Scope *scope, const QString &code)
{
    return canonicalSymbol(scope, code, m_typeOfExpression);
}

Symbol *CanonicalSymbol::canonicalSymbol(Scope *scope, const QString &code,
                                         TypeOfExpression &typeOfExpression)
{
    const QList<LookupItem> results =
            typeOfExpression(code.toUtf8(), scope, TypeOfExpression::Preprocess);

    for (int i = results.size() - 1; i != -1; --i) {
        const LookupItem &r = results.at(i);
        Symbol *decl = r.declaration();

        if (!(decl && decl->enclosingScope()))
            break;

        if (Class *classScope = r.declaration()->enclosingScope()->asClass()) {
            const Identifier *declId = decl->identifier();
            const Identifier *classId = classScope->identifier();

            if (classId && classId->match(declId))
                continue; // skip it, it's a ctor or a dtor.

            if (Function *funTy = r.declaration()->type()->asFunctionType()) {
                if (funTy->isVirtual())
                    return r.declaration();
            }
        }
    }

    for (int i = 0; i < results.size(); ++i) {
        const LookupItem &r = results.at(i);

        if (r.declaration())
            return r.declaration();
    }

    return 0;
}

} // namespace Internal
} // namespace CppEditor
