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

#include "util.h"

bool compareDoubles(double a, double b)
{
    int aInt = a * 100;
    int bInt = b * 100;
    return (aInt == bInt);
}

QString formatHex(quint32 num, bool pad, bool ohX)
{
    QString temp;
    if (ohX) {
        temp = "0x";
    }
    if (pad) {
        return temp += QString("%1").arg(num, 8, 16, QChar('0')).toUpper();
    }
    return temp += QString::number(num, 16).toUpper();
}
