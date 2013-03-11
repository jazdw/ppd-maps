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

#include "mappreset.h"

mapPreset::mapPreset(QObject *parent) :
    QObject(parent), scale(1.0), offset(0.0), decimalPoints(0)
{
}

mapPreset::mapPreset(QString label, QString units, double scale, double offset, int dp, QObject *parent) :
    QObject(parent)
{
    this->label = label;
    this->units = units;
    this->scale = 1.0/scale;
    this->offset = -offset/scale;
    this->decimalPoints = dp;
}

QString mapPreset::infoStr()
{
    if (units == "") {
        return "";
    }
    QString result = units + " [" + QString::number(scale) + ", " + QString::number(offset) + "]";
    if (decimalPoints > 0) {
        result += " " + QString::number(decimalPoints) + "dp";
    }
    return result;
}
