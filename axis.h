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

#ifndef AXIS_H
#define AXIS_H

#include <QObject>
#include "ppddata.h"
#include <QList>

class axis : public ppdData
{
    Q_OBJECT
public:
    explicit axis(QByteArray* fileData, quint32 fileAddr, QObject *parent = 0);
    explicit axis(quint8 setLength, QObject *parent = 0);
    quint8 getLength();
    quint8 getWordSize();
    quint32 getAddress();
    quint32 getRawFileAddr();
    bool isValid();
    double at(int i);
    bool isFixed();
signals:
    
public slots:

private:
    quint8 length;
    qint16 calcLength();
    bool loadData();
    bool doScaling();
    QList<double> axisValues;
    bool fixedData;
};

#endif // AXIS_H
