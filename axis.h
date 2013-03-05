#ifndef AXIS_H
#define AXIS_H

#include <QObject>
#include "ppddata.h"
#include <QList>

class axis : public ppdData
{
    Q_OBJECT
public:
    explicit axis(QByteArray* fileData, quint32 fileAddr, QObject *parent = 0);
    explicit axis(quint8 setLength, QObject *parent = 0);
    quint8 getLength();
    quint8 getWordSize();
    quint32 getAddress();
    quint32 getRawFileAddr();
    bool isValid();
    double at(int i);
    bool isFixed();
signals:
    
public slots:

private:
    quint8 length;
    qint16 calcLength();
    bool loadData();
    bool doScaling();
    QList<double> axisValues;
    bool fixedData;
};

#endif // AXIS_H
