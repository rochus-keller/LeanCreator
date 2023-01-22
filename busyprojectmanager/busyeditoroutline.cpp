#include "busyeditoroutline.h"
#include "busyproject.h"
#include "busy/Engine.h"
#include <projectexplorer/session.h>
extern "C" {
#include <bsparser.h>
}
using namespace busy;

class EditorOutline::Imp
{
public:
    struct Slot
    {
        //CrossRefModel::SymRef d_sym;
        int d_id;
        QByteArray d_name;
        bool operator<( const Slot& rhs ) const { return qstricmp( d_name, rhs.d_name ) < 0; }
    };
    QList<Slot> d_rows;
    Engine::Ptr d_eng;
    QString d_path;
    int d_module;

    Imp():d_module(0){}
};

EditorOutline::EditorOutline(QObject *parent) : QAbstractItemModel(parent)
{
    d_imp = new Imp();
}

EditorOutline::~EditorOutline()
{
    delete d_imp;
}

void EditorOutline::setFileName(const QString& path)
{
    BusyProjectManager::Internal::BusyProject* p =
            qobject_cast<BusyProjectManager::Internal::BusyProject *>(
                ProjectExplorer::SessionManager::instance()->projectForFile(
                    Utils::FileName::fromString(path)));
    if( p == 0 )
        return;

    connect( p, SIGNAL(projectParsingDone(bool)), this, SLOT(fileUpdated()));
    beginResetModel();
    d_imp->d_rows.clear();
    d_imp->d_eng = p->busyProject().getEngine();
    d_imp->d_module = d_imp->d_eng->findModule(path);
    d_imp->d_path = path;
    fill();
    endResetModel();
}

const QString&EditorOutline::getFileName() const
{
    return d_imp->d_path;
}

bool EditorOutline::getRowCol(const QModelIndex& index, int& row, int& col) const
{
    if( !index.isValid() )
        return false;
    const int id = index.internalId();
    if( id == 0 )
        return false;
    Q_ASSERT( id <= d_imp->d_rows.size() );
    row = d_imp->d_eng->getInteger(d_imp->d_rows[id-1].d_id,"#row");
    col = d_imp->d_eng->getInteger(d_imp->d_rows[id-1].d_id,"#col") - 1;
    return true;

}

QModelIndex EditorOutline::findByPosition(int row, int col) const
{
    if( d_imp->d_eng.constData() == 0 )
        return QModelIndex();
    QList<int> found = d_imp->d_eng->findDeclByPos(d_imp->d_path,row,col);
    for(int i = 0; i < d_imp->d_rows.size(); i++ )
    {
        if( found.contains(d_imp->d_rows[i].d_id) )
            return index(i+1,0);
    }
    return QModelIndex();
}

int EditorOutline::getDecl(const QModelIndex& index) const
{
    if( !index.isValid() )
        return 0;
    const int id = index.internalId();
    if( id == 0 )
        return false;
    Q_ASSERT( id <= d_imp->d_rows.size() );
    return d_imp->d_rows[id-1].d_id;
}

Engine*EditorOutline::getEngine() const
{
    return d_imp->d_eng.data();
}

QModelIndex EditorOutline::index(int row, int column, const QModelIndex& parent) const
{
    if( parent.isValid() || row < 0 )
        return QModelIndex();

    return createIndex( row, column, (quint32)row );
}

QModelIndex EditorOutline::parent(const QModelIndex& child) const
{
    return QModelIndex();
}

int EditorOutline::rowCount(const QModelIndex& parent) const
{
    if( parent.isValid() )
        return 0;
    else
        return d_imp->d_rows.size() + 1; // +1 because of <no symbol>
}

int EditorOutline::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant EditorOutline::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() )
        return QVariant();

    const int id = index.internalId();
    if( id == 0 )
    {
        switch (role)
        {
        case Qt::DisplayRole:
            if( !d_imp->d_rows.isEmpty() )
                return tr("<Select Symbol>");
            else
                return tr("<No Symbols>");
        default:
            return QVariant();
        }
    }
    Q_ASSERT( id <= d_imp->d_rows.size() );
    switch( role )
    {
    case Qt::DisplayRole:
        return d_imp->d_rows[id-1].d_name.data();
    case Qt::ToolTipRole:
        return QVariant();
    case Qt::DecorationRole:
        {
            const int k = d_imp->d_eng->getInteger(d_imp->d_rows[id-1].d_id,"#kind");
            switch( k )
            {
            case BS_ModuleDef:
                return QPixmap(":/busyprojectmanager/images/block.png");
            case BS_ClassDecl:
                return QPixmap(":/busyprojectmanager/images/class.png");
            case BS_EnumDecl:
                return QPixmap(":/busyprojectmanager/images/category.png");
            case BS_VarDecl:
                // TODO: different icons for var/let/param and visibility
                return QPixmap(":/busyprojectmanager/images/var.png");
            case BS_MacroDef:
                return QPixmap(":/busyprojectmanager/images/func.png");
            }
        }
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags EditorOutline::flags(const QModelIndex& index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void EditorOutline::fileUpdated()
{
    beginResetModel();
    d_imp->d_rows.clear();
    fill();
    endResetModel();
}

void EditorOutline::fill()
{
    QList<int> decls = d_imp->d_eng->getAllDecls(d_imp->d_module);
    for( int i = 0; i < decls.size(); i++ )
    {
        Imp::Slot s;
        s.d_id = decls[i];
        s.d_name = d_imp->d_eng->getString(decls[i],"#name");
        d_imp->d_rows.append( s );
    }
    std::sort( d_imp->d_rows.begin(), d_imp->d_rows.end() );
}

