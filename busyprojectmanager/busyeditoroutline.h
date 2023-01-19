#ifndef BUSYEDITOROUTLINEMODEL_H
#define BUSYEDITOROUTLINEMODEL_H

#include <QAbstractItemModel>

namespace busy
{
class EditorOutline : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit EditorOutline(QObject *parent = 0);

    // overrides
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
private:
    struct Slot
    {
        //CrossRefModel::SymRef d_sym;
        QByteArray d_name;
        bool operator<( const Slot& rhs ) const { return qstricmp( d_name, rhs.d_name ) < 0; }
    };
    QList<Slot> d_rows;
};
}

#endif // BUSYEDITOROUTLINEMODEL_H
