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

#include "mapsearch.h"

extern QVector<mapPreset*> presets;

const quint8 idDTC_PPD[] = {0x05,0x00,0x01,0x02,0x03,0x04};
const quint8 idDTC_SID803A[] = {0x01,0x23,0x45,0x74,0x00,0x00,0x00,0x00};
const quint8 idDTC_SID803A_VOLVO[] = {0x18,0xE7,0x04,0x00};
const quint8 idDTC_SID803[] = {0x01,0x23,0x45};
const quint8 idDTC_SID803_FORD[] = {0x00,0x69,0x7F,0x60,0x35,0x00,0x00,0x00};
const quint8 idDTC_SID201[] = {0x0E,0x11,0xC6,0xA2,0x01,0x23,0x45,0x74,0x00,0x00};
const quint8 idDTC_SID206[] = {0x00,0x0D,0x00,0x0E,0x00,0x0F,0x08};

const bool enableLogging = true;

// ecuType,    dataOffset, codeOffset, codeStart, r2, dtcWidth, dtcCount, dataLower, dataUpper
const ecuDescriptor ecuDefs[] = {
    {UNKNOWN_ECU,   0x800000, 0x400000, 0,       0,       26, 6,  0x40000, 0x80000},
    {PPD,           0x800000, 0x200000, 0x80000, 0x482F0, 28, 10, 0x41100, 0x7D87F},
    {SID803,        0,        0,        0,       0x4846C, 22, 6,  0x40240, 0x657FF},
    {SID803,        0,        0,        0,       0x48230, 26, 6,  0x40240, 0x657FF}, // SID803_FORD
    {SID803A,       0x800000, 0x400000, 0x80000, 0x48494, 26, 6,  0x40240, 0x7CDBF},
    {SID803A,       0x800000, 0x400000, 0x80000, 0x48488, 28, 6,  0x40240, 0x657FF}, // SID803A_VOLVO
    {SID201,        0x800000, 0x400000, 0x80000, 0x27FF0, 22, 6,  0x40240, 0x7D0FF},
    {SID206,        0x800000, 0x400000, 0x80000, 0x4850C, 26, 6,  0x40240, 0x7D1BF},
    {SID30X,        0x800000, 0x400000, 0x80000, 0x4857C, 28, 7,  0x40240, 0x7D07F},
    {SID30X,        0x800000, 0x400000, 0x80000, 0x4857C, 32, 10, 0x40240, 0x7D07F} // SID30X_3M
};

mapSearch::mapSearch(bool discardInvalid, QObject *parent) :
    QObject(parent), bdm_file(0), data(0), stop(false), validFile(false),
    discardInvalid(discardInvalid),
    dtcPtrs(0), dtcCount_table(0), dtcCount_switch(0),
    manufacturer(UNKNOWN_MANUFACTURER),
    specificECU(UNKNOWN_ECU), ecu(ecuDefs[UNKNOWN_ECU])
{
    bdm_file = new QFile(this);
    data = new QByteArray();
    data->reserve(0x200000);
    mapList = new QList<map*>;
    axisList = new QList<axis*>;
}

mapSearch::~mapSearch()
{
    qDeleteAll(mapList->begin(), mapList->end());
    qDeleteAll(axisList->begin(), axisList->end());

    mapList->clear();
    axisList->clear();
    data->clear();

    delete data;
    delete mapList;
    delete axisList;
}

QList<map *> *mapSearch::getMapList()
{
    return mapList;
}

QString mapSearch::getInfoStr()
{
    QString temp;
    if (!ecuType.isEmpty()) {
        temp += "<b>ECU Type:</b> " + ecuType + " ";
    }
    if (!manufacturerStr.isEmpty()) {
        temp += "<b>Manufacturer:</b> " + manufacturerStr + " ";
    }
    if (!codeVersion.isEmpty()) {
        temp += "<b>Ver:</b> " + codeVersion + " ";
    }
    if (!partNumber.isEmpty()) {
        temp += "<b>ECU:</b> " + partNumber + " ";
    }
    if (!engineInfo.isEmpty()) {
        temp += "<b>Info:</b> " + engineInfo;
    }

    return temp;
}

void mapSearch::findTableStarts()
{
    int matches = 0;
    int threshold = 3;

    for (int i = ecu.codeStart; i < data->length(); i+=4) {
        quint32 address = readData<quint32>(i);
        address -= ecu.dataOffset;
        addressType currentAddrType = examineAddr(address);

        if (currentAddrType == mapAddr || currentAddrType == axisAddr) {
            matches++;
        }
        else {
            matches = 0;
        }

        if (matches == threshold) {
            int start = i - (threshold-1) * 4;
            tableStarts.append(start);
            emit log("List of maps found starting at: 0x" + formatHex(start), debugLvl);
        }
    }
}

template <class fixedSizeInt>
fixedSizeInt mapSearch::readData(quint32 addr) {
    return castToInteger<fixedSizeInt>((void*) (data->constData() + addr), true);
}

bool mapSearch::asmMatch(quint32 addr, quint32 mask[], quint32 value[], int len, int arrayIndex) {
    mask += arrayIndex;
    value += arrayIndex;

    qint64 addr64 = addr;
    if (addr64 > data->length())
        return false;

    quint32 instr = readData<quint32>(addr);
    if ((instr & *mask) == *value) {
        if (len == 1) {
            return true;
        }
        return asmMatch(addr+4, mask+1, value+1, len-1);
    }
    return false;
}

bool mapSearch::addSwitches(quint32 addr1, quint32 addr2, quint16 tblIndex)
{
    map* sw1 = new map(data, addr1, false, this);
    map* sw2 = new map(data, addr2, false, this);
    axis* sw_axis = findExistingAxis(new axis(1));
    sw1->set8bit();
    sw2->set8bit();
    sw1->addAxis(sw_axis);
    sw2->addAxis(sw_axis);

    QString sw1label = "Sw 1 ";
    QString sw2label = "Sw 2 ";

    if (dtcPtrs) {
        quint32 dtcAddr = readData<quint32>(dtcPtrs + 20*tblIndex + 8);
        dtcAddr -= ecu.dataOffset;

        // add DTC if its not in table
        bool found = false;
        for (int i = 0; i < mapList->length(); i++) {
            if (mapList->at(i)->getAddress() == dtcAddr) {
                found = true;
                break;
            }
        }
        if (!found) {
            emit log("DTC for switch " + formatHex(addr1, false, true) + " not found in table. Index: " +
                     QString("%1").arg(tblIndex, 3, 10, QChar('0')) + ". Will add to DTC list.", debugLvl);
            if (addDTC(dtcAddr)) {
                dtcCount_switch++;
            }
            else {
                emit log("Couldn't add DTC, not valid.", debugLvl);
            }
        }

        sw1label += "for" + getPCodes(dtcAddr) + " at " + formatHex(dtcAddr);
        sw2label += "for" + getPCodes(dtcAddr) + " at " + formatHex(dtcAddr);
    }
    else {
        sw1label += QString("%1").arg(tblIndex, 3, 10, QChar('0'));
        sw2label += QString("%1").arg(tblIndex, 3, 10, QChar('0'));
    }
    sw1->setLabel(sw1label);
    sw2->setLabel(sw2label);

    return (addMap(sw1) && addMap(sw2));
}

int mapSearch::switchSearchR2()
{
    if (ecu.r2 == 0) {
        return 0;
    }

    int switchesFound = 0;

    // search for code like this
    // lbz       r5, xxxx(r2)
    // lbz       r6, xxxx(r2)
    // li        r3, xx
    quint32 masks[] =  {0xFFFF0000, 0xFFFF0000, 0xFFFF0000};
    quint32 values[] = {0x88A20000, 0x88C20000, 0x38600000};

    for (int i = ecu.codeStart; i < data->length(); i=i+4) {
        if (asmMatch(i, masks, values, 3)) {
            if (!dtcPtrs) {
                dtcPtrs = findDTCPtrs(i);
                if (dtcPtrs) {
                    dtcPtrs -= ecu.codeOffset;
                }
            }

            quint16 index = readData<quint16>(i+10);
            quint32 sw1addr = ecu.r2 + readData<quint16>(i+2);
            quint32 sw2addr = ecu.r2 + readData<quint16>(i+6);

            emit log("Found R2 Indexed DTC switch code at " + formatHex(i, true, true) +
                     " (Index " + QString::number(index) + ")", debugLvl);
            if (addSwitches(sw1addr, sw2addr, index)) {
                switchesFound++;
            }
        }
    }

    return 2*switchesFound;
}

int mapSearch::switchSearchDirect()
{
    int switchesFound = 0;

    // TODO: GENERISISE THIS CODE

    // search for code like this
    // lis       r5, xxxx@h
    // lis       r6, xxxx@h
    // xxxx
    // lbz       r5, xxxx@l(r5)
    // lbz       r6, xxxx@l(r6)
    // xxxx
    // li        r3, xx
    quint32 masks1[] =  {0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000};
    quint32 values1[] = {0x3CA00000, 0x3CC00000, 0x88A50000, 0x88C60000, 0x38600000};
    quint32 masks2[] =  {0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0x00000000, 0xFFFF0000};
    quint32 values2[] = {0x3CA00000, 0x3CC00000, 0x88A50000, 0x88C60000, 0x00000000, 0x38600000};
    quint32 masks3[] =  {0xFFFF0000, 0xFFFF0000, 0x00000000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000};
    quint32 values3[] = {0x3CA00000, 0x3CC00000, 0x00000000, 0x88A50000, 0x88C60000, 0x38600000};
    quint32 masks4[] =  {0xFFFF0000, 0xFFFF0000, 0x00000000, 0xFFFF0000, 0xFFFF0000, 0x00000000, 0xFFFF0000};
    quint32 values4[] = {0x3CA00000, 0x3CC00000, 0x00000000, 0x88A50000, 0x88C60000, 0x00000000, 0x38600000};
    quint32 masks5[] =  {0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000};
    quint32 values5[] = {0x3CA00000, 0x88A50000, 0x3CC00000, 0x88C60000, 0x38600000};
    quint32 masks6[] =  {0xFFFF0000, 0xFFFF0000, 0x00000000, 0x00000000, 0xFFFF0000, 0xFFFF0000, 0x00000000, 0xFFFF0000};
    quint32 values6[] = {0x3CA00000, 0x3CC00000, 0x00000000, 0x00000000, 0x88A50000, 0x88C60000, 0x00000000, 0x38600000};

    for (int i = ecu.codeStart; i < data->length(); i=i+4) {
        if (asmMatch(i, masks1, values1, 2) || asmMatch(i, masks5, values5, 2)) {
            quint16 index;
            quint32 sw1addr, sw2addr;
            if (asmMatch(i, masks1, values1, 5)) {
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+10);
                sw2addr = readData<qint16>(i+6);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+14);
                index = readData<quint16>(i+18);
            }
            else if (asmMatch(i, masks2, values2, 6)){
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+10);
                sw2addr = readData<qint16>(i+6);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+14);
                index = readData<quint16>(i+22);
            }
            else if (asmMatch(i, masks3, values3, 6)){
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+14);
                sw2addr = readData<qint16>(i+6);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+18);
                index = readData<quint16>(i+22);
            }
            else if (asmMatch(i, masks4, values4, 7)){
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+14);
                sw2addr = readData<qint16>(i+6);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+18);
                index = readData<quint16>(i+26);
            }
            else if (asmMatch(i, masks5, values5, 5)) {
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+6);
                sw2addr = readData<qint16>(i+10);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+14);
                index = readData<quint16>(i+18);
            }
            else if (asmMatch(i, masks6, values6, 7)){
                sw1addr = readData<qint16>(i+2);
                sw1addr = sw1addr << 16;
                sw1addr += readData<qint16>(i+18);
                sw2addr = readData<qint16>(i+6);
                sw2addr = sw2addr << 16;
                sw2addr += readData<qint16>(i+24);
                index = readData<quint16>(i+30);
            }
            else {
                //emit log("Possible DTC switch code at: " + formatHex(i, true, true), debugLvl);
                continue;
            }

            sw1addr -= ecu.dataOffset;
            sw2addr -= ecu.dataOffset;

            if (!dtcPtrs) {
                dtcPtrs = findDTCPtrs(i);
                if (dtcPtrs) {
                    dtcPtrs -= ecu.codeOffset;
                }
            }

            emit log("Found Direct DTC switch code at " + formatHex(i, true, true) +
                     " (Index " + QString::number(index) + ")", debugLvl);
            if (addSwitches(sw1addr, sw2addr, index)) {
                switchesFound++;
            }
        }
    }

    return 2*switchesFound;
}

quint32 mapSearch::findDTCPtrs(quint32 addr)
{
    qint32 blOffsetSigned = 0;

    //                bl,         lis,        addi
    quint32 mask[] = {0xFC000003, 0xFC1F0000, 0xFC000000};
    quint32 val[] =  {0x48000001, 0x3C000000, 0x38000000};
    quint32 subAddr = addr;

    for (int i = 4; i <= 28; i=i+4) {
        if (asmMatch(addr+i, mask, val, 1)) {
            // found bl to subroutine
            subAddr += i; // update subAddr to match location where bl was found
            quint32 blOffset = readData<quint32>(addr+i) & 0x03FFFFFC;
            blOffset = blOffset >> 2;
            if (blOffset & 0x00800000) { // sign extend
                blOffset |= 0xFF000000;
            }
            blOffsetSigned = *((qint32*) &blOffset);
            blOffsetSigned *= 4;
            break;
        }
    }

    if (blOffsetSigned == 0) {
        return 0;
    }

    subAddr += blOffsetSigned; // calculate new address
    qint64 subAddr64 = blOffsetSigned;
    if (subAddr64 > data->size()) {
        return 0;
    }

    quint32 tableLoc = 0;
    for (int i = 0; i < 80; i=i+4) {
        if (asmMatch(subAddr+i, mask+1, val+1, 1)) {
            // found lis
            quint16 upper = readData<quint16>(subAddr+i+2);
            for (int j = 0; j < 80; j=j+4) {
                if (asmMatch(subAddr+i+j, mask+2, val+2, 1)) {
                    // found addi
                    qint16 lower = readData<qint16>(subAddr+i+j+2);
                    tableLoc = upper;
                    tableLoc = tableLoc << 16;
                    tableLoc += lower;
                    break;
                }
            }
            break;
        }
    }

    return tableLoc;
}

QString mapSearch::getPCodes(quint32 addr)
{
    QString ret;

    for (int i = 0; i < ecu.dtcCount; i++) {
        quint16 pCode = readData<quint16>(addr + i*2);
        if (pCode != 0 && pCode < 61440) {
            QString pCodeTxt = QString(" P%1").arg(pCode, 4, 16, QChar('0')).toUpper();
            if (!ret.contains(pCodeTxt)) {
                ret += pCodeTxt;
            }
        }
    }
    if (ret == "")
        ret = " NULL";
    return ret;
}

bool mapSearch::fileIsValid()
{
    return validFile;
}

int mapSearch::getBdmFileSize()
{
    return bdm_file->size();
}

QString mapSearch::getBdmFilename()
{
    return bdm_file->fileName();
}

void mapSearch::cancel() {
    this->stop = true;
}

void mapSearch::setDiscardInvalid(bool discard)
{
    discardInvalid = discard;
}

void mapSearch::open_file(QString filename)
{
    validFile = false;
    specificECU = UNKNOWN_ECU;
    manufacturer = UNKNOWN_MANUFACTURER;
    dtcPtrs = 0;
    ecu = ecuDefs[UNKNOWN_ECU];
    dtcCount_table = 0;
    dtcCount_switch = 0;

    qDeleteAll(mapList->begin(), mapList->end());
    mapList->clear();
    qDeleteAll(axisList->begin(), axisList->end());
    axisList->clear();
    emit resetTableModel();

    if (bdm_file->isOpen()) {
        bdm_file->close();
    }
    bdm_file->setFileName(filename);
    bdm_file->open(QIODevice::ReadOnly);
    data->clear();
    qint64 readSize = bdm_file->size();
    if (readSize > 0x200000) {
        readSize = 0x200000;
    }
    data->append(bdm_file->read(readSize));
    bdm_file->close();

    if (readData<quint32>(0) != 0x48000002) {
        emit log("Error: File is not a full BDM read.");
        emit log("OBD reads do not contain the code and pointers to the maps.");
        emit log("Try loading a BDM read with the exact same software version.");
    }
    else {
        validFile = true;
    }

    codeVersion = QString::fromAscii(data->mid(0x40023, 13).constData());
    QString codeVersionLeft2 = codeVersion.left(2);
    if (codeVersionLeft2 == "SN") {
        manufacturerStr = "VW/Audi";
        manufacturer = VW;
    }
    else if (codeVersionLeft2 == "VO") {
        manufacturerStr = "Volvo";
        manufacturer = VOLVO;
    }
    else if (codeVersionLeft2 == "PO" || codeVersionLeft2 == "PP") {
        manufacturerStr = "Peugeot";
        manufacturer = PEUGEOT;
    }
    else if (codeVersionLeft2 == "FO" || codeVersionLeft2 == "FR") {
        manufacturerStr = "Ford";
        manufacturer = FORD;
    }
    else if (codeVersionLeft2 == "RM") {
        manufacturerStr = "Renault";
        manufacturer = RENAULT;
    }
    else if (codeVersionLeft2 == "3M") {
        manufacturerStr = "Nissan";
        manufacturer = NISSAN;
    }
    else {
        manufacturerStr = "Unknown";
        manufacturer = UNKNOWN_MANUFACTURER;
    }

    partNumber = "Unknown";
    engineInfo = "Unknown";
    ecuType = "Unknown";

    if (QString::fromAscii(data->mid(0x40000, 4).constData()) == "CASN") {
        specificECU = PPD;
        partNumber = QString::fromAscii(data->mid(0x40060, 11).constData());
        engineInfo = QString::fromAscii(data->mid(0x4006C, 14).constData());
    }
    else {
        emit log("Warning: File is not from a Siemens PPD. Preliminary support only.");
        if (data->size() == 0x200000 && QString::fromAscii(data->mid(0x6300, 2)) == "5W") {
            partNumber = QString::fromAscii(data->mid(0x6300, 11).constData());
            if (QString::fromAscii(data->mid(0x4260, 14).constData()) == "SIEMENS-SID206") {
                specificECU = SID206;
                engineInfo = QString::fromAscii(data->mid(0x4260, 23).constData());
            }
            else {
                specificECU = SID803A;
            }
        }

        if (data->size() == 0x200000 && QString::fromAscii(data->mid(0x6900, 2)) == "5W") {
            partNumber = QString::fromAscii(data->mid(0x6900, 11).constData());
            specificECU = SID201;
        }

        if (data->size() == 0x71000) {
            specificECU = SID803;
        }

        if (manufacturer == RENAULT) {
            specificECU = SID30X;
        }

        if (codeVersionLeft2 == "3M") {
            specificECU = SID30X_3M;
        }
    }

    if (specificECU == SID803 && manufacturer == FORD) {
        specificECU = SID803_FORD;
    }
    else if (specificECU == SID803A && manufacturer == VOLVO) {
        specificECU = SID803A_VOLVO;
    }

    ecu = ecuDefs[specificECU];

    switch (ecu.genericECU) {
    case PPD:
        ecuType = "PPD";
        break;
    case SID803:
        ecuType = "SID803";
        break;
    case SID803A:
        ecuType = "SID803A";
        break;
    case SID201:
        ecuType = "SID201";
        break;
    case SID206:
        ecuType = "SID206";
        break;
    case SID30X:
        ecuType = "SID30X";
        break;
    default:
        ecuType = "Unknown";
    }

    emit log("Manufacturer: " + manufacturerStr, logFileLvl | standardLvl);
    emit log("ECU Type:     " + ecuType, logFileLvl | standardLvl);
    emit log("Code Version: " + codeVersion, logFileLvl | standardLvl);
    emit log("Part Number:  " + partNumber, logFileLvl | standardLvl);
    emit log("Info String:  " + engineInfo, logFileLvl | standardLvl);

    if (validFile) {
        checkR2();
        search();
    }

    emit fileLoaded();
    emit log("Finished.", standardLvl, true);
}

mapSearch::addressType mapSearch::examineAddr(quint32 address, bool extraCheck)
{
    if (address < ecu.dataLower || address >= ecu.dataUpper) {
        return outOfRangeAddr;
    }

    axis test(data, address);
    if (!test.isValid()) {
        return mapAddr;
    }
    else if (!extraCheck) {
        return axisAddr;
    }

    int nextAddr = address + (test.getLength()+1)*test.getWordSize();
    if (nextAddr % 2) {
        nextAddr++;
    }

    axis testNext(data, nextAddr);
    if (!testNext.isValid())
        return mapAddr;
    else
        return axisAddr;
}

bool mapSearch::addMap(map *newMap)
{
    emit log(newMap->infoStr(), debugLvl);
    if (discardInvalid) {
        if (newMap->getAxis(0)->isFlat()) {
            emit log("Warning: X-Axis is flat or zero. Discarding map.", debugLvl);
            delete newMap;
            return false;
        }
        if (newMap->getNumAxes() > 1 && newMap->getAxis(1)->isFlat()) {
            emit log("Warning: Y-Axis is flat or zero. Discarding map.", debugLvl);
            delete newMap;
            return false;
        }
    }
    for (int i = 0; i < mapList->length(); i++) {
        if (mapList->at(i)->getAddress() == newMap->getAddress()) {
            emit log("Warning: Duplicate map detected. Discarding map.", debugLvl);
            delete newMap;
            return false;
        }
    }

    if (!newMap->isDTC() && newMap->getAxis(0)->getRawFileAddr() != 0) {
        // Logic to find parallel maps, i.e. maps with the same axes
        map* searchMap;
        for (int i = 0; i < mapList->length(); i++) {
            searchMap = mapList->at(i);
            if (
                    searchMap->getXDim() == newMap->getXDim() &&
                    searchMap->getAxis(0)->getAddress() == newMap->getAxis(0)->getAddress() &&
                    searchMap->getYDim() == newMap->getYDim() &&
                    (searchMap->getYDim() == 1 || searchMap->getAxis(1)->getAddress() == newMap->getAxis(1)->getAddress())
                    )
            {
                searchMap->addFriend(newMap);
                newMap->addFriend(searchMap);
            }
        }
    }

    mapList->append(newMap);
    return true;
}

axis* mapSearch::findExistingAxis(axis* newAxis)
{
    if (newAxis->isFixed()) {
        for (int j = 0; j < axisList->length(); j++) {
            if (axisList->at(j)->isFixed() &&
                axisList->at(j)->getLength() == newAxis->getLength()) {
                delete newAxis;
                return axisList->at(j);
            }
        }
    }
    else {
        for (int j = 0; j < axisList->length(); j++) {
            if (axisList->at(j)->getRawFileAddr() == newAxis->getRawFileAddr()) {
                delete newAxis;
                return axisList->at(j);
            }
        }
    }

    axisList->append(newAxis);
    return newAxis;
}

bool mapSearch::createMap(quint32 addrMap, quint32 axis1, quint32 axis2)
{
    map* newMap = new map(data, addrMap, false, this);
    axis* newAxis1 = findExistingAxis(new axis(data, axis1));
    axis* newAxis2 = 0;
    if (!axisAddresses.contains(axis1)) {
        axisAddresses.append(axis1);
    }
    if (axis2) {
        newAxis2 = findExistingAxis(new axis(data, axis2));
        if (!axisAddresses.contains(axis2)) {
            axisAddresses.append(axis2);
        }
    }
    newMap->addAxis(newAxis1);
    if (newAxis2) {
        newMap->addAxis(newAxis2);
    }
    return addMap(newMap);
}

void mapSearch::idMap(map *aMap, int i)
{
    if (aMap->isDTC() || aMap->getLabel() != "") {
        return;
    }
    map* prev = 0;
    map* next = 0;
    if (i-1 >= 0)
        prev = mapList->at(i-1);
    if (i+1 < mapList->length())
        next = mapList->at(i+1);

    if (aMap->dimensionsStr() == "20x1") {
        aMap->loadPreset(presets.at(NM_2_PRESET));
        aMap->setLabel("Torque Limiter");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "6x4") {
        aMap->loadPreset(presets.at(MG_STK_PRESET));
        aMap->setLabel("IQ Limiter");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "16x19") {
        aMap->loadPreset(presets.at(NM_PRESET));
        aMap->setLabel("Smoke Limiter");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(HPA_PRESET));
        aMap->getAxis(1)->setLabel("MAP");
        return;
    }

    if (aMap->dimensionsStr() == "16x8" && aMap->getAxis(0)->at(0) == 750) {
        aMap->loadPreset(presets.at(HPA_PRESET));
        aMap->setLabel("MAP Limiter");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        if (aMap->getAxis(1)->at(0) == 50) {
            aMap->getAxis(1)->loadPreset(presets.at(DEG_C_PRESET));
        }
        else {
            aMap->getAxis(1)->loadPreset(presets.at(HPA_PRESET));
        }
        return;
    }

    if (aMap->dimensionsStr() == "16x8" && aMap->getAddress() > 0x70000) {
        aMap->loadPreset(presets.at(NM_2_PRESET));
        aMap->setLabel("Drivers Wish");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        //aMap->getAxis(1)->loadPreset(presets.at(FACTOR_PRESET));
        aMap->getAxis(1)->setScaleFactor(0.097752);
        aMap->getAxis(1)->setUnits("%");
        aMap->getAxis(1)->setLabel("Pedal Position");
        return;
    }

    /*
    if (aMap->dimensionsStr() == "15x15") {
        aMap->setUpdateFriends(false);
        if (next && next->dimensionsStr() == "15x15") {
            aMap->setScaleFactor(0.0390625);
            aMap->setLabel("Exhaust spec. heat capacity");
        }
        else {
            aMap->setScaleFactor(0.004);
            aMap->setLabel("Exhaust viscosity/thermal conductivity");
        }
        aMap->getAxis(0)->loadPreset(presets.at(DEG_C_3_PRESET));
        aMap->getAxis(1)->setScaleFactor(0.00048828125);
        aMap->getAxis(1)->setUnits("Lambda");
        return;
    }
    */

    if (aMap->dimensionsStr() == "2x1") {
        if (compareDoubles(aMap->getAxis(0)->at(0) / 13108, 0.40)) {
            aMap->loadPreset(presets.at(HPA_PRESET));
            aMap->setLabel("MAP Sensor Linearisation");
            aMap->getAxis(0)->loadPreset(presets.at(V_3_PRESET));
            aMap->getAxis(0)->setLabel("Sensor");
            return;
        }
    }

    if (aMap->dimensionsStr() == "8x2") {
        aMap->setLabel("MAP Limiter?");
        aMap->loadPreset(presets.at(HPA_PRESET));
        //aMap->getAxis(0)->loadPreset(presets.at(KG_H_PRESET));
        aMap->getAxis(1)->setLabel("Selector");
        aMap->getAxis(1)->loadPreset(presets.at(FACTOR_5_PRESET));
        aMap->getAxis(0)->setScaleFactor(0.015625);
        return;
    }

    if (aMap->dimensionsStr() == "16x1" && aMap->getAxis(0)->at(0) == 441) {
        aMap->loadPreset(presets.at(NM_2_PRESET));
        aMap->setLabel("Limp Mode Torque Limiter?");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        return;
    }

    /*
    if (aMap->dimensionsStr() == "16x16" &&
            aMap->getAxis(0)->at(0) == 850 &&
            (aMap->getAxis(1)->at(1) == 0x8280 || aMap->getAxis(1)->at(1) == 0x80a0)) {
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_2_PRESET));
        if (aMap->getValue(0,0) < 20000) {
            aMap->setLabel("Turbo Related 2");
            aMap->loadPreset(presets.at(V_PRESET)); // Not sure what this actually is
        }
        else {
            aMap->setLabel("Unknown");
            aMap->loadPreset(presets.at(PERCENT_PRESET));
        }
        return;
    }
    */

    if (aMap->dimensionsStr() == "16x16" && aMap->getAxis(0)->at(0) == 780) {
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_PRESET));
        if (aMap->getAxis(0)->at(1) == 1200) {
            aMap->loadPreset(presets.at(PERCENT_PRESET));
            aMap->setLabel("Turbo Precontrol (N75 Duty) 1");
        }
        else if ((codeVersion.left(6) == "SN000F" || codeVersion.left(7) == "SN100K3") &&
                 aMap->getAxis(1)->at(15) == 400)
        {
            aMap->loadPreset(presets.at(PERCENT_PRESET));
            aMap->setLabel("Turbo Precontrol (N75 Duty) 1");
        }
        else {
            aMap->loadPreset(presets.at(HPA_PRESET));
            aMap->setLabel("Turbo (MAP) Setpoint 1");
        }
        return;
    }

    if (aMap->dimensionsStr() == "16x16" &&
            aMap->getAxis(0)->at(0) == 850 &&
            aMap->getAxis(1)->at(1) == 320) {
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_PRESET));
        if (aMap->getValue(0,0) > 20000) {
            aMap->loadPreset(presets.at(PERCENT_3_PRESET));
            aMap->setLabel("Turbo Precontrol (N75 Duty) 2");
        }
        else {
            aMap->loadPreset(presets.at(HPA_PRESET));
            aMap->setLabel("Turbo (MAP) Setpoint 2");
        }
        return;
    }

    if (aMap->dimensionsStr() == "12x12" &&
            aMap->getAxis(0)->at(1) == 12 &&
            aMap->getAxis(1)->at(1) == 12) {
        aMap->loadPreset(presets.at(HPA_PRESET));
        aMap->setLabel("MAP Limiter 2");
        return;
    }

    if (aMap->dimensionsStr() == "8x8" &&
            aMap->getAxis(0)->at(0) == 1000 &&
            aMap->getAxis(1)->at(1) == 0x0320) {
        aMap->loadPreset(presets.at(HPA_PRESET));
        aMap->setLabel("MAP Limiter 3");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "16x16" && aMap->getAxis(0)->at(0) == 719) {
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_PRESET));
        if (aMap->getValue(0,0) == 0) {
            aMap->loadPreset(presets.at(PERCENT_PRESET));
            aMap->setLabel("EGR Duty Precontrol?");
        }
        else {
            // MAF SP
            aMap->loadPreset(presets.at(MG_STK_4_PRESET));
            aMap->setLabel("MAF Set Point");
        }
        return;
    }

    if (aMap->dimensionsStr() == "16x16" &&
            aMap->getAxis(0)->at(0) == 750) {
        if (aMap->isFlat())
            return;
        aMap->loadPreset(presets.at(MG_STK_2_PRESET));
        aMap->setLabel("Inj. Quantity");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_2_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "16x10") {
        aMap->loadPreset(presets.at(PERCENT_PRESET));
        aMap->setLabel("Pressure correction on Temp?");
        aMap->getAxis(0)->loadPreset(presets.at(DEG_C_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(HPA_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "16x16" &&
            aMap->getAxis(1)->at(0) == 0 &&
            aMap->getAxis(1)->at(1) == 1 &&
            aMap->getAxis(1)->at(2) == 2 &&
            aMap->getAxis(1)->at(3) == 3 &&
            aMap->getAxis(1)->at(4) == 4 &&
            aMap->getAxis(1)->at(5) == 5) {
        aMap->loadPreset(presets.at(NM_2_PRESET));
        aMap->setLabel("Torque limiter for monitoring error");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->setUnits("Gear?");
        return;
    }

    if (aMap->dimensionsStr() == "16x16" &&
            aMap->getAxis(0)->at(0) == 800 &&
            aMap->getAxis(0)->at(1) == 1000 &&
            aMap->getAxis(0)->at(2) == 1500 &&
            aMap->getAxis(0)->at(3) == 2000 &&
            aMap->getAxis(0)->at(4) == 2225 &&
            aMap->getAxis(0)->at(5) == 2500) {
        aMap->loadPreset(presets.at(NM_2_PRESET));
        aMap->setLabel("Torque limiter by EGT");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(DEG_C_3_PRESET));
        return;
    }

    if (aMap->dimensionsStr() == "16x16" && aMap->getAxis(0)->at(0) == 100) {
        aMap->loadPreset(presets.at(DEG_CRK_2_PRESET));
        aMap->setLabel("Start of Inj. (SOI)");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(NM_2_PRESET));
        return;
    }

    /*
    if (aMap->dimensionsStr() == "8x8" &&
            aMap->getAxis(0)->at(0) == 0 &&
            aMap->getAxis(0)->at(1) == 1 &&
            aMap->getAxis(0)->at(2) == 2 &&
            aMap->getAxis(1)->at(0) == 0 &&
            aMap->getAxis(1)->at(1) == 1 &&
            aMap->getAxis(1)->at(2) == 2) {
        aMap->setLabel("DPF Related Coefficient Matrix");
        return;
    }
    */

    if (aMap->dimensionsStr() == "5x5" &&
            aMap->getWordSize() == 1 &&
            aMap->getValue(0,0) == 0 &&
            aMap->getValue(4,4) == 255) {
        aMap->setLabel("Swirl Flap Map");
        aMap->getAxis(0)->loadPreset(presets.at(RPM_PRESET));
        aMap->getAxis(1)->loadPreset(presets.at(MG_STK_3_PRESET));
        aMap->setScaleFactor(0.003921568627451);
    }
}

void mapSearch::checkOverlap()
{
    int length = mapList->length() - 1;

    for (int i = 0; i < length; i++) {
        map* current = mapList->at(i);
        map* next = mapList->at(i+1);
        if (current->getEndAddress() > next->getAddress()) {
            current->set8bit();
        }
        if (current->getEndAddress() > next->getAddress()) {
            emit log("Warning: Overlapping map, removing: " + current->getAddressStr(), debugLvl);
            mapList->removeAt(i--);
            length--;
        }
    }
}

int mapSearch::durationSearch()
{
    int durMapCount = 0;

    // this code searches for 4 sequential occurrences of addi rX, r2, XXXX
    // they are 1 instruction apart
    quint32 masks[] =  {0x7C1F0000, 0, 0x7C1F0000, 0, 0x7C1F0000, 0, 0x7C1F0000};
    quint32 values[] = {0x38020000, 0, 0x38020000, 0, 0x38020000, 0, 0x38020000};

    for (int i = 0x80000; i < data->length() && durMapCount < 4; i=i+4) {
        if (asmMatch(i, masks, values, 7)){
            emit log("Found injection duration code at: " + formatHex(i, true, true), debugLvl);
            addDurationMaps(i, durMapCount);
            durMapCount++;
        }
    }

    return durMapCount;
}

void mapSearch::addDurationMaps(quint32 addr, int groupNum)
{
    const QString labels[4] = {"Prev", "Main", "Post", "Regen"};
    qint16 offset = readData<qint16>(addr + 2);
    quint16 selectorLen = readData<quint16>(ecu.r2 + offset);

    map* selector = new map(data, ecu.r2 + offset + 2, false, this);
    axis* selectorAxis = findExistingAxis(new axis(selectorLen));
    selector->addAxis(selectorAxis);
    selector->loadPreset(presets.at(DEG_CRK_2_PRESET));
    selector->setLabel("Inj. Duration: " + labels[groupNum] + " selector SOI");
    addMap(selector);

    offset = readData<qint16>(addr + 10);
    axis* axis1 = findExistingAxis(new axis(data, ecu.r2 + offset));
    offset = readData<qint16>(addr + 18);
    axis* axis2 = findExistingAxis(new axis(data, ecu.r2 + offset));
    axis1->loadPreset(presets.at(RPM_PRESET));
    axis2->loadPreset(presets.at(MG_STK_PRESET));

    for (int i = 0; i < selectorLen; i++) {
        quint32 durMapAddr = readData<quint16>(addr + 30 + i*12);
        durMapAddr = durMapAddr <<  16;
        durMapAddr |= readData<quint16>(addr + 38 + i*12);
        durMapAddr -= 0x800000;
        map* durMap = new map(data, durMapAddr, false);
        durMap->addAxis(axis1);
        durMap->addAxis(axis2);
        durMap->loadPreset(presets.at(DEG_CRK_PRESET));
        durMap->setLabel("Inj. Duration: " + labels[groupNum]);
        addMap(durMap);
    }

    groupNum++;
}

void mapSearch::search()
{
    stop = false;

    if (!validFile) {
        emit log("No BDM file loaded.");
        return;
    }

    tableStarts.clear();
    axisAddresses.clear();

    findTableStarts();

    int tablePos;
    int mapNoAxis = 0;
    int axisNoMap = 0;
    int twoMaps = 0;
    int fallBackMethod = 0;
    int duplicateMaps = 0;

    // beef of the search is here
    for (int i = 0; i < tableStarts.length(); i++) {
        tablePos = tableStarts.at(i);
        if (i < tableStarts.length()) {
            stop = false;
        }
        emit  log("Info: Traversing new table at " + formatHex(tablePos, false, true), debugLvl);
        while (tablePos <= data->length() && !stop) {
            quint32 addr[5];
            addressType addrType[5];
            // calculate the actual file address
            for (int i = 0; i < 5; i++) {
                addr[i] = readData<quint32>(tablePos+i*4);
                addr[i] -= ecu.dataOffset;
                addrType[i] = examineAddr(addr[i]);
            }

            if (addrType[0] == outOfRangeAddr) {
                emit  log("Info: Address out of range at " + formatHex(tablePos, false, true) + ". End of table reached.", debugLvl);
                stop = true;
            }
            else if (addrType[0] == axisAddr) {
                addrType[0] = mapAddr;
            }

            if (addrType[0] == mapAddr) {
                // now determine how many axes this map has
                if (addrType[1] == mapAddr) { // 0 axes
                    emit log("Warning: Two maps in a row at " + formatHex(tablePos, false, true) +
                             " -> " + formatHex(addr[0], false, true), debugLvl);
                    twoMaps++;
                }
                else if (addrType[1] == outOfRangeAddr) { // 0 axes
                    emit log("Warning: Map with no axes at " + formatHex(tablePos, false, true) +
                             " -> " + formatHex(addr[0], false, true), debugLvl);
                    mapNoAxis++;
                }
                else if (addrType[2] != axisAddr) { // 1 axes
                    if (!createMap(addr[0], addr[1], 0)) {
                        duplicateMaps++;
                    }
                    tablePos += 4;
                }
                else if (addrType[3] != axisAddr) { // 2 axes
                    if (!createMap(addr[0], addr[1], addr[2])) {
                        duplicateMaps++;
                    }
                    tablePos += 8;
                }
                else if (addrType[4] != axisAddr) { // 1 axes
                    if (!createMap(addr[0], addr[1], 0)) {
                        duplicateMaps++;
                    }
                    tablePos += 4;
                }
                else if (axisAddresses.contains(addr[2])) { // 2 axes
                    if (!createMap(addr[0], addr[1], addr[2])) {
                        duplicateMaps++;
                    }
                    tablePos += 8;
                }
                else {
                    emit log("Warning: Using fall back method to determine number of axes at " + formatHex(tablePos, false, true) +
                             " -> " + formatHex(addr[0], false, true), debugLvl);
                    fallBackMethod++;
                    if (addr[1] < addr[0] && addr[2] < addr[0] &&
                        addr[1] < addr[3] && addr[2] < addr[3]) { // assume 2 axis
                        if (!createMap(addr[0], addr[1], addr[2])) {
                            duplicateMaps++;
                        }
                        tablePos += 8;
                    }
                    else if (addr[1] < addr[0] && addr[1] < addr[2]) { // assume 1 axis
                        if (!createMap(addr[0], addr[1], 0)) {
                            duplicateMaps++;
                        }
                        tablePos += 4;
                    }
                    else {
                        emit log("Warning: Two maps in a row at " + formatHex(tablePos, false, true) +
                                 " -> " + formatHex(addr[0], false, true), debugLvl);
                        twoMaps++;
                    }
                }
            }

            tablePos += 4;
        }
    }

    if (stop) {
        emit log("Search stopped.", debugLvl);
    }

    dtcCount_table = dtcSearch();
    if (dtcCount_table == 0) {
        emit log("DTC table not found.", debugLvl);
    }

    // map list must be sorted before checking for overlaps and identifying maps
    qSort(mapList->begin(), mapList->end(), ppdData::addrLessThan);
    checkOverlap();

    int durationCount;
    if (ecu.genericECU == PPD) {
        for (int i = 0; i < mapList->length(); i++) {
            idMap(mapList->at(i), i);
        }
        durationCount = durationSearch();
        emit log(QString::number(durationCount) + " PPD duration maps found.", debugLvl);
    }

    int switchDirectCount = switchSearchDirect();
    int switchR2Count = switchSearchR2();

    int totalCount = mapList->length();
    int mapsOnlyCount = totalCount - switchR2Count - switchDirectCount - dtcCount_table - dtcCount_switch;

    int problems = mapNoAxis + axisNoMap + twoMaps + fallBackMethod + duplicateMaps;
    emit log("Problems:     " + QString::number(problems), debugLvl | logFileLvl);
    emit log("              " + QString::number(mapNoAxis) + " maps with no axes (next address is out of range)", debugLvl | logFileLvl);
    emit log("              " + QString::number(twoMaps) + " maps with no axes (two maps in a row)", debugLvl | logFileLvl);
    emit log("              " + QString::number(axisNoMap) + " axes with no map", debugLvl | logFileLvl);
    emit log("              " + QString::number(fallBackMethod) + " maps determined using fall back method", debugLvl | logFileLvl);
    emit log("              " + QString::number(duplicateMaps) + " duplicate maps discarded (not including switches)", debugLvl | logFileLvl);

    emit log("Maps:         " + QString::number(mapsOnlyCount), logFileLvl | standardLvl);
    emit log("DTCs:         " + QString::number(dtcCount_table + dtcCount_switch) + " (" +
             QString::number(dtcCount_table) + " from table, " +
             QString::number(dtcCount_switch) + " from switch)", logFileLvl | standardLvl);
    emit log("Switches:     " + QString::number(switchDirectCount + switchR2Count) +
             " / " + QString::number(switchDirectCount/2 + switchR2Count/2) + " pairs (" +
             QString::number(switchDirectCount) + " direct access, " +
             QString::number(switchR2Count) + " R2 indexed)", logFileLvl | standardLvl);
    emit log("TOTAL:        " + QString::number(totalCount), logFileLvl | standardLvl);
    emit log("", logFileLvl);
    emit resetTableModel();
}

int mapSearch::dtcSearch()
{
    QByteArray idDTCBA;
    int bytesAfter = 0;
    if (specificECU == PPD) {
        idDTCBA.append((const char*) idDTC_PPD, sizeof(idDTC_PPD));
    }
    else if (specificECU == SID803A ){
        idDTCBA.append((const char*) idDTC_SID803A, sizeof(idDTC_SID803A));
    }
    else if (specificECU == SID803) {
        idDTCBA.append((const char*) idDTC_SID803, sizeof(idDTC_SID803));
        bytesAfter = 5;
    }
    else if (specificECU == SID201) {
        idDTCBA.append((const char*) idDTC_SID201, sizeof(idDTC_SID201));
    }
    else if (specificECU == SID206) {
        idDTCBA.append((const char*) idDTC_SID206, sizeof(idDTC_SID206));
        bytesAfter = 9;
    }
    else if (specificECU == SID803A_VOLVO) {
        idDTCBA.append((const char*) idDTC_SID803A_VOLVO, sizeof(idDTC_SID803A_VOLVO));
    }
    else if (specificECU == SID803_FORD) {
        idDTCBA.append((const char*) idDTC_SID803_FORD, sizeof(idDTC_SID803_FORD));
    }
    else if (specificECU == SID30X || specificECU == UNKNOWN_ECU) {
        return 0;
    }

    int dtcStart = data->indexOf(idDTCBA, 0);
    if (dtcStart < 0) {
        return 0;
    }
    dtcStart += idDTCBA.length() + bytesAfter;

    emit log("DTC table starts at " + formatHex(dtcStart, false, true), debugLvl);

    int count = 0;
    int pos = dtcStart;
    while (addDTC(pos)) {
        pos += ecu.dtcWidth;
        count++;
    }

    // hack for SID206 where some are 26 wide some are 28
    if (count < 30) {
        emit log("Trying second pass at finding DTCs from table.", debugLvl);
        for (int i = 0; i < count; i++) {
            mapList->removeLast();
        }
        count = 0;
        ecu.dtcWidth += 2;
        pos = dtcStart;
        while (addDTC(pos)) {
            pos += ecu.dtcWidth;
            count++;
        }
    }

    return count;
}

bool mapSearch::addDTC(quint32 addr) {
    if (specificECU == SID803A_VOLVO) {
        if (readData<quint8>(addr + ecu.dtcWidth - 2) != 0)
            return false;
    }
    else {
        if (readData<quint8>(addr + ecu.dtcWidth - 1) != 0)
            return false;
    }

    map* dtc;

    dtc = new map(data, addr, true, this);
    axis* fixedAxis = findExistingAxis(new axis(ecu.dtcWidth/2));
    dtc->addAxis(fixedAxis);

    // set label to P-Codes
    dtc->setLabel("DTC" + getPCodes(addr));

    addMap(dtc);
    return true;
}

void mapSearch::checkR2()
{
    quint32 masks[] =  {0xFFFF0000, 0xFFFF0000};
    quint32 values[] = {0x3C400000, 0x60420000};

    quint32 r2 = 0;
    // keep going after a match so it finds the last one
    for (int i = ecu.codeStart; i < data->length(); i=i+4) {
        if (asmMatch(i, masks, values, 2)) {
            r2 = readData<quint16>(i+2);
            r2 = r2 << 16;
            r2 |= readData<quint16>(i+6);
            r2 -= ecu.dataOffset;
        }
    }

    if (r2 == ecu.r2) {
        emit log("R2 value OK: " + formatHex(r2, false, true), debugLvl);
    }
    else if (r2 != 0) {
        emit log("R2 value corrected: " + formatHex(r2, false, true), debugLvl);
        ecu.r2 = r2;
    }
    else {
        emit log("R2 could not be verified: " + formatHex(r2, false, true), debugLvl);
    }
}
