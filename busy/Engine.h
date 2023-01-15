#ifndef BUSYENGINE_H
#define BUSYENGINE_H

#include <QSharedData>

namespace busy
{
class Engine : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<Engine> Ptr;

    Engine();
    ~Engine();

private:
    class Imp;
    Imp* d_imp;
};
}

#endif // BUSYENGINE_H
