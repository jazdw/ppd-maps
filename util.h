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

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QtEndian>

enum {standardLvl = 0x1, debugLvl = 0x2, logFileLvl = 0x4};

bool compareDoubles(double a, double b);
QString formatHex(quint32 num, bool pad = false, bool ohx = false);

template <class fixedSizeInt>
fixedSizeInt castToInteger(void* ptr, bool bigEndian) {
    fixedSizeInt tmp = *((fixedSizeInt*) ptr);
    if (bigEndian && (sizeof(fixedSizeInt) > 1))
        tmp = qFromBigEndian(tmp);
    return tmp;
}

#endif // UTIL_H
