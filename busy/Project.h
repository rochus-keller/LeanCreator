#ifndef BUSYPROJECT_H
#define BUSYPROJECT_H

#include <QString>
#include <QSharedData>

namespace busy
{
class Module : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<Module> Ptr;

    Module();
};
}

#endif // BUSYPROJECT_H
