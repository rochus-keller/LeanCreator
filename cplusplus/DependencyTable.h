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

#ifndef CPLUSPLUS_DEPENDENCYTABLE_H
#define CPLUSPLUS_DEPENDENCYTABLE_H

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <utils/fileutils.h>

#include <QBitArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace CPlusPlus {

class Snapshot;

class CPLUSPLUS_EXPORT DependencyTable
{
private:
    struct File {
        Utils::FileName name;
        uint modified;
        File(const Utils::FileName& _name = Utils::FileName(), uint _mod = 0):name(_name),modified(_mod){}
    };

    friend class Snapshot;
    void build(const Snapshot &snapshot);

    QVector<File> files;
    QHash<Utils::FileName, int> fileIndex;
    QHash<int, QList<int> > includes;
    QVector<QBitArray> includeMap;

public:
    Utils::FileNameList filesDependingOn(const Utils::FileName &fileName) const;
    Utils::FileNameList allFilesDependingOnModifieds() const;
    bool anyNewerDeps(const QString& path, uint ref, QString* reason = 0) const;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_DEPENDENCYTABLE_H
