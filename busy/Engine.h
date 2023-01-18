#ifndef BUSYENGINE_H
#define BUSYENGINE_H

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

#include <QSharedData>
#include <QString>

namespace busy
{
class Engine : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<Engine> Ptr;

    Engine();
    ~Engine();

    bool parse( const QByteArray& dir );

    int getRootModule() const;
    QList<int> getSubModules(int) const;
    QList<int> getAllProducts(int module, bool withSourceOnly = false, bool runnableOnly = false) const;
    QStringList getAllSources(int product) const;
    bool isExecutable(int) const;
    QByteArray getString(int def, const char* field, bool inst = false) const;
    int getInteger(int def, const char* field) const;
    int getOwner(int def) const;
protected:
    bool pushInst(int ref) const;
private:
    class Imp;
    Imp* d_imp;
};
}

#endif // BUSYENGINE_H
