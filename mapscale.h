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

#ifndef MAPSCALE_H
#define MAPSCALE_H

#include "qwt3d_scale.h"
#include "axis.h"
#include <QList>

class mapScale : public Qwt3D::Scale
{
    friend class Axis;
    //friend class qwt3d_ptr<Scale>;
public:
    mapScale(axis* scaleAxis = 0, int dp = 2);
    mapScale(QList<double>* values = 0, int dp = 2);
protected:
    Scale* clone() const;
    void calculate();
    axis* scaleAxis;
    QList<double>* values;
    static double roundDouble(double in, int dp = 2);
    int dp;
};

#endif // MAPSCALE_H
