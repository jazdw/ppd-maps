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

#ifndef MAPPRESET_H
#define MAPPRESET_H

#include <QObject>
#include <QVector>

class mapPreset : public QObject
{
    Q_OBJECT
public:
    explicit mapPreset(QObject *parent = 0);
    explicit mapPreset(QString label, QString units, double scale = 1.0, double offset = 0.0, int dp = 0, QObject *parent = 0);
    QString infoStr();
    double scale;
    double offset;
    QString label;
    QString units;
    int decimalPoints;
signals:
public slots:
private:
};

enum {
    NO_PRESET,
    HPA_PRESET,
    HPA_2_PRESET,
    KG_H_PRESET,
    KM_H_PRESET,
    KM_H_2_PRESET,
    MG_STK_PRESET,
    MG_STK_2_PRESET,
    MG_STK_3_PRESET,
    MG_STK_4_PRESET,
    MG_STK_5_PRESET,
    NM_PRESET,
    NM_2_PRESET,
    NM_3_PRESET,
    RPM_PRESET,
    S_PRESET,
    S_2_PRESET,
    V_PRESET,
    V_2_PRESET,
    V_3_PRESET,
    DEG_C_PRESET,
    DEG_C_2_PRESET,
    DEG_C_3_PRESET,
    DEG_CRK_PRESET,
    DEG_CRK_2_PRESET,
    DEG_CRK_3_PRESET,
    PERCENT_PRESET,
    PERCENT_2_PRESET,
    PERCENT_3_PRESET,
    PERCENT_4_PRESET,
    FACTOR_PRESET,
    FACTOR_2_PRESET,
    FACTOR_3_PRESET,
    FACTOR_4_PRESET,
    FACTOR_5_PRESET
};

#endif // MAPPRESET_H
