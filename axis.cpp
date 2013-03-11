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

#include "axis.h"

axis::axis(QByteArray *fileData, quint32 fileAddr, QObject *parent) :
    ppdData(fileData, fileAddr, parent), length(0), fixedData(false)
{
    calcLength();
}

axis::axis(quint8 setLength, QObject *parent) :
    ppdData(0, 0, parent), length(setLength), fixedData(true)
{
    wordSize = 1;
    decimalPoints = 0;
    updateFriends = false;
}

quint8 axis::getLength()
{
    if (length > 0) {
        return length;
    }
    else {
        calcLength();
        return length;
    }
}

quint32 axis::getAddress()
{
    if (fileAddr == 0)
        return 0;
    return fileAddr + wordSize;
}

quint32 axis::getRawFileAddr()
{
    return fileAddr;
}

qint16 axis::calcLength()
{
    if (fileAddr <= 0) {
        return -1;
    }

    quint8 dim[2];
    memcpy(dim, fileData->mid(fileAddr, 2).constData(), 2);

    // 16 bit axis, assume dimension is never > 2^8
    if (dim[0] == 0) {
        wordSize = 2;
        length = dim[1];
    }
    else { // 8 bit axis
        wordSize = 1;
        length = dim[0];
    }

    return length;
}

bool axis::loadData()
{
    if (fixedData)
        return true;

    if (!fileData || length == 0 || length > 32) {
        return false;
    }

    if (data) {
        delete data;
    }

    int numBytes = length * wordSize;

    data = new QByteArray(fileData->mid(getAddress(), numBytes));

    return (data > 0);
}

bool axis::doScaling()
{
    axisValues.clear();

    if (data || loadData()) {
        zero = true;
        flat = true;
        bool firstPoint = true;
        double prev = 0.0;

        for (int i = 0; i < length; i++) {
            double raw;
            if (fixedData) {
                raw = i;
            }
            else {
                void* point = (void*) (data->constData() + i * wordSize);
                raw = asDouble(point) * scaleFactor + offset;
            }
            if (firstPoint) {
                prev = raw;
                firstPoint = false;
            }
            if (flat) {
                if (raw != prev) {
                    flat = false;
                }
                prev = raw;
            }

            if (raw != 0.0) {
                zero = false;
            }
            axisValues.append(raw);
        }
        dataRead = true;
        emit changed();
        return true;
    }

    return false;
}

quint8 axis::getWordSize()
{
    return wordSize;
}

bool axis::isValid()
{
    if (isFixed()) {
        return true;
    }

    if (length == 0 || length > 32) {
        return false;
    }

    int last = at(0);
    for (int i = 0; i < length; i++) {
        if (at(i) < last) {
            return false;
        }
        last = at(i);
    }

    return true;
}

double axis::at(int i)
{
    if (axisValues.empty()) {
        doScaling();
    }
    return axisValues.at(i);
}

bool axis::isFixed()
{
    return fixedData;
}
