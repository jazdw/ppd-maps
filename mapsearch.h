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

#ifndef MAPSEARCH_H
#define MAPSEARCH_H

#include <QObject>
#include <QFile>
#include <QByteArray>
#include <QList>
#include <QVector>
#include "util.h"
#include "axis.h"
#include "map.h"
#include "mappreset.h"

typedef struct {
    int genericECU;
    quint32 dataOffset;
    quint32 codeOffset;
    quint32 codeStart;
    quint32 r2;
    int dtcWidth;
    int dtcCount;
    quint32 dataLower;
    quint32 dataUpper;
} ecuDescriptor;

enum {
    UNKNOWN_ECU,
    PPD,
    SID803,
    SID803_FORD,
    SID803A,
    SID803A_VOLVO,
    SID201,
    SID206,
    SID30X,
    SID30X_3M
};

enum {
    UNKNOWN_MANUFACTURER,
    VW,
    PEUGEOT,
    FORD,
    VOLVO,
    RENAULT,
    NISSAN
};

class mapSearch : public QObject
{
    Q_OBJECT
public:
    explicit mapSearch(bool discardInvalid = true, QObject *parent = 0);
    ~mapSearch();
    enum addressType {mapAddr, axisAddr, invalidAddr, outOfRangeAddr};
    QList<map*>* getMapList();
    QString getInfoStr();
    int getBdmFileSize();
    QString getBdmFilename();
    bool fileIsValid();
public slots:
    void open_file(QString filename);
    void search();
    void cancel();
    void setDiscardInvalid(bool discard);
private:
    QFile* bdm_file;
    QByteArray* data;
    bool stop;
    bool validFile;
    addressType examineAddr(quint32 address, bool extraCheck = false);
    QList<map*>* mapList;
    QList<axis*>* axisList;
    QList<int> tableStarts;
    bool discardInvalid;
    int dtcPtrs;
    int dtcCount_table;
    int dtcCount_switch;
    int manufacturer;
    int specificECU;
    ecuDescriptor ecu;
    QString codeVersion;
    QString partNumber;
    QString engineInfo;
    QString manufacturerStr;
    QString ecuType;
    QList<quint32> axisAddresses;

    bool addMap(map* newMap);
    axis* findExistingAxis(axis* newAxis);
    bool createMap(quint32 addrMap, quint32 axis1, quint32 axis2);
    void idMap(map* aMap, int i);
    void checkOverlap();
    int durationSearch();
    void addDurationMaps(quint32 addr, int groupNum);
    int dtcSearch();
    void findTableStarts();
    template <class fixedSizeInt>
    fixedSizeInt readData(quint32 addr);
    int switchSearchR2();
    int switchSearchDirect();
    quint32 findDTCPtrs(quint32 addr);
    QString getPCodes(quint32 addr);
    bool asmMatch(quint32 addr, quint32 mask[], quint32 value[], int len, int arrayIndex = 0);
    bool addSwitches(quint32 addr1, quint32 addr2, quint16 tblIndex);
    bool addDTC(quint32 addr);
    int getDTCWidth();
    void checkR2();

signals:
    void log(const QString &text, int level = standardLvl, bool flushBuffer = false);
    void resetTableModel();
    void fileLoaded();
};

#endif // MAPSEARCH_H
