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

#include "mapscale.h"
#include "qmath.h"

mapScale::mapScale(axis* scaleAxis, int dp) :
    scaleAxis(scaleAxis), values(0), dp(dp)
{
}

mapScale::mapScale(QList<double> *values, int dp) :
    scaleAxis(0), values(values), dp(dp)
{
}

Qwt3D::Scale *mapScale::clone() const
{
    if (scaleAxis) {
        return new mapScale(scaleAxis, dp);
    }
    else if (values) {
        return new mapScale(values, dp);
    }
    return 0;
}

void mapScale::calculate()
{
    majors_p.clear();
    minors_p.clear();
    if (scaleAxis) {
        for (int i = 0; i < scaleAxis->getLength(); i++) {
            if (i == 0) {
                majors_p.push_back(roundDouble(scaleAxis->at(i), dp));
            }
            else if (i == (scaleAxis->getLength() - 1)) {
                majors_p.push_back(roundDouble(scaleAxis->at(i), dp));
            }
            else {
                minors_p.push_back(roundDouble(scaleAxis->at(i), dp));
            }
        }
    }
    else if (values) {
        for (int i = 0; i < values->size(); i++) {
            if (i == 0) {
                majors_p.push_back(roundDouble(values->at(i), dp));
            }
            else if (i == (values->size() - 1)) {
                majors_p.push_back(roundDouble(values->at(i), dp));
            }
            //else {
            //    minors_p.push_back(values->at(i));
            //}
        }
    }
}

double mapScale::roundDouble(double in, int dp)
{
    int temp = in * qPow(10, dp);
    return temp / qPow(10, dp);
}
