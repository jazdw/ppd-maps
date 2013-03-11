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

#include "ppddata.h"
#include <QtEndian>
#include <qmath.h>

ppdData::ppdData(QByteArray *fileData, quint32 fileAddr, QObject *parent) :
    QObject(parent), data(0), wordSize(2),
    offset(0), isSigned(false), bigEndian(true),
    zero(true), flat(true), dataRead(false),
    decimalPoints(0), updateFriends(true)
{
    scaleFactor = 1.0; // / qPow(2, wordSize * 8) * 100;
    this->fileData = fileData;
    this->fileAddr = fileAddr;
}

ppdData::~ppdData()
{
    if (data) {
        delete data;
    }
}

double ppdData::getScaleFactor()
{
    return scaleFactor;
}

double ppdData::getOffset()
{
    return offset;
}

bool ppdData::getSigned()
{
    return isSigned;
}

bool ppdData::getEndian()
{
    return bigEndian;
}

quint8 ppdData::getWordSize()
{
    return wordSize;
}

quint32 ppdData::getAddress()
{
    return fileAddr;
}

void ppdData::setScaleFactor(double num, bool propagate)
{
    scaleFactor = num;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setScaleFactor(num, false);
        }
    }
}

void ppdData::setOffset(double num, bool propagate)
{
    offset = num;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setOffset(num, false);
        }
    }
}

void ppdData::setScaleFactor(QString text, bool propagate)
{
    this->setScaleFactor(text.toDouble(), propagate);
}

void ppdData::setOffset(QString text, bool propagate)
{
    setOffset(text.toDouble(), propagate);
}

void ppdData::set8bit(bool propagate)
{
    wordSize = 1;
    loadData();
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->set8bit(false);
        }
    }
}

void ppdData::set16bit(bool propagate)
{
    wordSize = 2;
    loadData();
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->set16bit(false);
        }
    }
}

void ppdData::set32bit(bool propagate)
{
    wordSize = 4;
    loadData();
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->set32bit(false);
        }
    }
}

void ppdData::setSigned(bool propagate)
{
    isSigned = true;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setSigned(false);
        }
    }
}

void ppdData::setUnsigned(bool propagate)
{
    isSigned = false;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setUnsigned(false);
        }
    }
}

void ppdData::setBigEndian(bool propagate)
{
    bigEndian = true;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setBigEndian(false);
        }
    }
}

void ppdData::setLittleEndian(bool propagate)
{
    bigEndian = false;
    doScaling();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setLittleEndian(false);
        }
    }
}

void ppdData::setLabel(QString text, bool propagate)
{
    label = text;
    emit changed();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setLabel(text, false);
        }
    }
}

void ppdData::setUnits(QString text, bool propagate)
{
    units = text;
    emit changed();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setUnits(text, false);
        }
    }
}

QString ppdData::getLabel()
{
    return label;
}

QString ppdData::getUnits()
{
    return units;
}

QString ppdData::getAddressStr(bool pad, bool ohx)
{
    if (fileAddr) {
        return formatHex(fileAddr, pad, ohx);
    }
    return "No Addr";
}

double ppdData::asDouble(void *ptr) {
    double raw = 0.0;

    switch (wordSize) {
    case 1:
        if (isSigned)
            raw = castToInteger<qint8>(ptr, bigEndian);
        else
            raw = castToInteger<quint8>(ptr, bigEndian);
        break;
    case 2:
        if (isSigned)
            raw = castToInteger<qint16>(ptr, bigEndian);
        else
            raw = castToInteger<quint16>(ptr, bigEndian);
        break;
    case 4:
        if (isSigned)
            raw = castToInteger<qint32>(ptr, bigEndian);
        else
            raw = castToInteger<quint32>(ptr, bigEndian);
        break;
    case 8:
        if (isSigned)
            raw = castToInteger<qint64>(ptr, bigEndian);
        else
            raw = castToInteger<quint64>(ptr, bigEndian);
            break;
    }

    return raw;
}

void ppdData::addFriend(ppdData* friendMap)
{
    if (!friends.contains(friendMap)) {
        friends.append(friendMap);
    }
}

int ppdData::numFriends()
{
    return friends.count();
}

ppdData* ppdData::getFriend(int i)
{
    return friends.at(i);
}

bool ppdData::getUpdateFriends()
{
    return updateFriends;
}

bool ppdData::isZero()
{
    if (!dataRead) {
        doScaling();
    }
    return zero;
}

bool ppdData::isFlat()
{
    if (!dataRead) {
        doScaling();
    }
    return flat;
}

QString ppdData::getEquation()
{
    QString equ = "X*" + QString::number(scaleFactor, 'f', 10);
    if (offset >= 0) {
        return equ + "+" + QString::number(offset, 'f', 10);
    }
    else {
        return equ + QString::number(offset, 'f', 10);
    }
}

void ppdData::setUpdateFriends(bool value, bool propagate)
{
    updateFriends = value;

    if (propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setUpdateFriends(value, false);
        }
    }
}

void ppdData::loadPreset(mapPreset *preset)
{
    setLabel(preset->label);
    setScaleFactor(preset->scale);
    setOffset(preset->offset);
    setUnits(preset->units);
    setDecimalPoints(preset->decimalPoints);
}

int ppdData::getNumBytes()
{
    if (data) {
        return data->length();
    }
    else return 0;
}

void ppdData::setDecimalPoints(int num, bool propagate)
{
    decimalPoints = num;
    emit changed();

    if (updateFriends && propagate) {
        for (int i = 0; i < friends.count(); i++) {
            friends.at(i)->setDecimalPoints(num, false);
        }
    }
}

void ppdData::setDecimalPoints(QString num, bool propagate)
{
    setDecimalPoints(num.toInt(), propagate);
}

int ppdData::getDecimalPoints()
{
    return decimalPoints;
}

double ppdData::calcMax()
{
    double max = pow(2, wordSize * 8);
    double min = max;

    if (isSigned) {
        min = -1.0 * (min/2);
        max = (max/2) - 1;
    }
    else {
        min = 0;
        max -= 1;
    }

    max = max * scaleFactor + offset;
    min = min * scaleFactor + offset;

    if (max > min)
        return max;
    else
        return min;
}

double ppdData::calcMin()
{
    double max = pow(2, wordSize * 8);
    double min = max;

    if (isSigned) {
        min = -1.0 * (min/2);
        max = (max/2) - 1;
    }
    else {
        min = 0;
        max -= 1;
    }

    max = max * scaleFactor + offset;
    min = min * scaleFactor + offset;

    if (min < max)
        return min;
    else
        return max;
}

double ppdData::calcRange()
{
    return (calcMax() - calcMin());
}

int ppdData::getWidth()
{
    int rightWidth = decimalPoints;
    if (rightWidth > 0) {
        rightWidth += 1; //accounts for the dot
    }
    int leftMax = qPow(2, wordSize * 8) * scaleFactor - offset;
    if (leftMax >= 10000000) {
        return rightWidth + 8;
    }
    if (leftMax >= 1000000) {
        return rightWidth + 7;
    }
    if (leftMax >= 100000) {
        return rightWidth + 6;
    }
    if (leftMax >= 10000) {
        return rightWidth + 5;
    }
    if (leftMax >= 1000) {
        return rightWidth + 4;
    }
    if (leftMax >= 100) {
        return rightWidth + 3;
    }
    if (leftMax >= 10) {
        return rightWidth + 2;
    }
    return rightWidth + 1;
}

bool ppdData::addrLessThan(ppdData *a, ppdData *b)
{
    return (a->getAddress() < b->getAddress());
}
