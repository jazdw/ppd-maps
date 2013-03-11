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

#include "conversion.h"

conversion::conversion(ppdData *in, int index) :
    index(index)
{
    units = in->getUnits();
    scaleFactor = in->getScaleFactor();
    offset = in->getOffset();
    decimalPoints = in->getDecimalPoints();
}

QString conversion::conversionID() const
{
    return "CNV_" + QString::number(index);
}


bool operator==(const conversion& lhs, const conversion& rhs) {
    if (lhs.decimalPoints == rhs.decimalPoints &&
            lhs.scaleFactor == rhs.scaleFactor &&
            lhs.offset == rhs.offset &&
            lhs.units == rhs.units) {
        return true;
    }

    return false;
}
