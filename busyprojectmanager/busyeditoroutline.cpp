#include "busyeditoroutline.h"
using namespace busy;

EditorOutline::EditorOutline(QObject *parent) : QAbstractItemModel(parent)
{

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
        return d_rows.size() + 1; // +1 because of <no symbol>
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
            if( !d_rows.isEmpty() )
                return tr("<Select Symbol>");
            else
                return tr("<No Symbols>");
        default:
            return QVariant();
        }
    }
    Q_ASSERT( id <= d_rows.size() );
    //const CrossRefModel::Symbol* s = d_rows[id-1].d_sym.data();
    switch( role )
    {
    case Qt::DisplayRole:
        return d_rows[id-1].d_name.data();
    case Qt::ToolTipRole:
        return QVariant();
    case Qt::DecorationRole:
#if 0 // TODO
        switch( s->tok().d_type )
        {
        case SynTree::R_module_declaration:
        case SynTree::R_udp_declaration:
            return QPixmap(":/verilogcreator/images/block.png");
        case SynTree::R_module_or_udp_instance_:
            return QPixmap(":/verilogcreator/images/var.png");
        case SynTree::R_task_declaration:
        case SynTree::R_function_declaration:
            return QPixmap(":/verilogcreator/images/func.png");
        case Tok_Section:
            return QPixmap(":/verilogcreator/images/category.png");
        }
#endif
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags EditorOutline::flags(const QModelIndex& index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

