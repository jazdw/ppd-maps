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

#include "maptablemodel.h"

mapTableModel::mapTableModel(QList<map*>* data, QObject *parent) :
    QAbstractTableModel(parent), modelData(data)
{
}

QVariant mapTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= modelData->size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch(index.column()) {
        case 0:
            return modelData->at(index.row())->getAddressStr();
        case 1:
            return modelData->at(index.row())->dimensionsStr();
        case 2:
            return modelData->at(index.row())->getLabel();
        }
    }

    if (role == Qt::BackgroundRole) {
        if (highlighted.contains(index.row())) {
            return QBrush(Qt::yellow);
        }
    }

    // UserRole is used for sorting
    if (role == Qt::UserRole) {
        int elements = 0;
        switch(index.column()) {
        case 0:
            return modelData->at(index.row())->getAddressStr();
        case 1:
            elements = modelData->at(index.row())->getXDim() * modelData->at(index.row())->getYDim();
            return  QString("%1").arg(elements, 3, 10, QChar('0')) + modelData->at(index.row())->getAddressStr();
        case 2:
            return modelData->at(index.row())->getLabel() + modelData->at(index.row())->getAddressStr();
        }
    }

    // UserRole+1 is used for filtering
    if (role == Qt::UserRole+1) {
        return modelData->at(index.row())->getAddressStr() + " " +
                modelData->at(index.row())->dimensionsStr() + " " +
                modelData->at(index.row())->getLabel();
    }

    return QVariant();
}

map* mapTableModel::getMap(const QModelIndex &index)
{
    return modelData->at(index.row());
}

QModelIndex mapTableModel::getIndex(map* search) {
    for (int i = 0; i < modelData->length(); i++) {
        if (modelData->at(i) == search) {
            return createIndex(i , 0);
        }
    }
    return QModelIndex();
}

void mapTableModel::setHighlightedRow(int i)
{
    highlighted.append(i);

    updateRow(i);
}

QVariant mapTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return "Addr";
            case 1:
                return "Dim";
            case 2:
                return "Label";
            }
        }
    }
    return QVariant();
}


int mapTableModel::rowCount(const QModelIndex &parent) const
{
    return modelData->size();
}

int mapTableModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

void mapTableModel::refresh()
{
    highlighted.clear();
    reset();
    emit refreshed();
}

void mapTableModel::clearHighlightedRows()
{
    if (highlighted.empty()) {
        return;
    }

    int max = 0, min = 0;

    for (int i = 0; i < highlighted.count(); i++) {
        if (highlighted.at(i) > max) {
            max = highlighted.at(i);
        }
        if (highlighted.at(i) <= min) {
            min = highlighted.at(i);
        }
    }

    highlighted.clear();

    QModelIndex first = createIndex(min, 0);
    QModelIndex last = createIndex(max, 2);
    emit dataChanged(first, last);
}

void mapTableModel::updateRow(int i)
{
    QModelIndex first = createIndex(i, 0);
    QModelIndex last = createIndex(i, 2);
    emit dataChanged(first, last);
}
