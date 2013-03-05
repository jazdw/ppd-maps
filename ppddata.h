#ifndef PPDDATA_H
#define PPDDATA_H

#include <QObject>
#include <QVector>

#include "mappreset.h"
#include "util.h"

class ppdData : public QObject
{
    Q_OBJECT
public:
    explicit ppdData(QByteArray* fileData, quint32 fileAddr, QObject *parent = 0);
    ~ppdData();
    double getScaleFactor();
    double getOffset();
    bool getSigned();
    bool getEndian();
    quint8 getWordSize();
    virtual quint32 getAddress();
    virtual QString infoStr() { return ""; }
    virtual bool isValid() { return false; }
    QString getLabel();
    QString getUnits();
    QString getAddressStr(bool pad = false, bool ohx = false);
    void addFriend(ppdData* friendMap);
    int numFriends();
    ppdData* getFriend(int i);
    bool getUpdateFriends();
    bool isZero();
    bool isFlat();
    QString getEquation();
    void loadPreset(mapPreset* preset);
    int getNumBytes();
    int getDecimalPoints();
    double calcMax();
    double calcMin();
    double calcRange();
    int getWidth();
    static bool addrLessThan(ppdData *a, ppdData *b);
signals:
    void changed();
public slots:
    void setScaleFactor(double num, bool propagate = true);
    void setOffset(double num, bool propagate = true);
    void setScaleFactor(QString text, bool propagate = true);
    void setOffset(QString text, bool propagate = true);
    void set8bit(bool propagate = true);
    void set16bit(bool propagate = true);
    void set32bit(bool propagate = true);
    void setSigned(bool propagate = true);
    void setUnsigned(bool propagate = true);
    void setBigEndian(bool propagate = true);
    void setLittleEndian(bool propagate = true);
    void setLabel(QString text, bool propagate = true);
    void setUnits(QString text, bool propagate = true);
    void setUpdateFriends(bool value, bool propagate = true);
    void setDecimalPoints(int num, bool propagate = true);
    void setDecimalPoints(QString num, bool propagate = true);
protected:
    QByteArray* fileData;
    quint32 fileAddr;
    QByteArray* data;
    quint8 wordSize;
    double scaleFactor;
    double offset;
    bool isSigned;
    bool bigEndian;
    QString label;
    QString units;
    bool zero;
    bool flat;
    bool dataRead;
    int decimalPoints;

    QVector<ppdData*> friends;
    bool updateFriends;

    virtual bool doScaling() { return false; }
    virtual bool loadData() { return false; }

    double asDouble(void* ptr);
};

#endif // PPDDATA_H
