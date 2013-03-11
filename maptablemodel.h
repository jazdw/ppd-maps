/*
Copyright 2013 Jared Wiltshire

This file is part of PPD Maps.

PPD Maps is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PPD Maps is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PPD Maps.  If not, see <http://www.gnu.org/licenses/>.
*/

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
