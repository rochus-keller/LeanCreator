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
    ~EditorOutline();

    void setFileName(const QString&);
    bool getRowCol( const QModelIndex &, int& row, int& col ) const;
    QModelIndex findByPosition( int row, int col) const;

    // overrides
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
protected slots:
    void fileUpdated();
private:
    void fill();
    class Imp;
    Imp* d_imp;
};
}

#endif // BUSYEDITOROUTLINEMODEL_H
