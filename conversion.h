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

#ifndef CONVERSION_H
#define CONVERSION_H

#include "ppddata.h"

class conversion
{
public:
    explicit conversion(ppdData* in, int index);
    QString conversionID() const;
    double scaleFactor;
    double offset;
    int decimalPoints;
    QString units;
    int index;
private:

signals:
    
public slots:
    
};

bool operator==(const conversion& lhs, const conversion& rhs);

#endif // CONVERSION_H
