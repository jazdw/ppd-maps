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
