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

#ifndef CPLUSPLUS_ASTPARENT_H
#define CPLUSPLUS_ASTPARENT_H

#include <cplusplus/ASTVisitor.h>

#include <QHash>
#include <QStack>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTParent: protected ASTVisitor
{
public:
    ASTParent(TranslationUnit *translationUnit, AST *rootNode);
    virtual ~ASTParent();

    AST *operator()(AST *ast) const;

    AST *parent(AST *ast) const;
    QList<AST *> path(AST *ast) const;

protected:
    virtual bool preVisit(AST *ast);
    virtual void postVisit(AST *ast);

    void path_helper(AST *ast, QList<AST *> *path) const;

private:
    QHash<AST *, AST *> _parentMap;
    QStack<AST *> _parentStack;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_ASTPARENT_H
