#ifndef MAP_H
#define MAP_H

#include <QObject>
#include <QVector>
#include "ppddata.h"
#include "axis.h"
#include "qwt3d_types.h"

typedef QVector<Qwt3D::Triple> tripleVector;

class map : public ppdData
{
    Q_OBJECT
public:
    explicit map(QByteArray* fileData, quint32 fileAddr, bool isDTC = false, QObject *parent = 0);
    ~map();
    bool addAxis(axis* axisToAdd);
    quint8 getNumAxes();
    QString infoStr();
    QString dimensionsStr();
    bool isValid();
    Qwt3D::Triple** getScaledTriples();
    int getXDim();
    int getYDim();
    double axisValue(int axisNum, int i);
    axis* getAxis(int i);
    QList<double> *getZValues();
    bool isDTC();
    quint32 getEndAddress();
    double getValue(int x, int y);
    const QString& getTableText(bool colour);
public slots:
    void axisChanged();
private:
    quint8 numAxes;
    axis* axes[2];
    QVector<tripleVector*> scaledTriples;
    Qwt3D::Triple* tempArray[30];
    bool loadData();
    bool doScaling();
    QList<double>* zValues;
    bool dtc;
    QString tableText;
};

#endif // MAP_H
