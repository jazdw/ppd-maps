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
