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
