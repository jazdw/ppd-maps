#ifndef MAPTABLEMODEL_H
#define MAPTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "map.h"

class mapTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit mapTableModel(QList<map*>* data, QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    map* getMap(const QModelIndex &index);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex getIndex(map *search);
    void setHighlightedRow(int i);
    void clearHighlightedRows();
    void updateRow(int i);
signals:
    void refreshed();
public slots:
    void refresh();
private:
    QList<map*>* modelData;
    QList<int> highlighted;
};

#endif // MAPTABLEMODEL_H
