#include "map.h"
#include "mapsearch.h"
#include "qmath.h"

map::map(QByteArray *fileData, quint32 fileAddr, bool isDTC, QObject *parent) :
    ppdData(fileData, fileAddr, parent),
    numAxes(0), zValues(new QList<double>),
    dtc(isDTC)
{
    axes[0] = 0;
    axes[1] = 0;
    if (dtc) {
        updateFriends = false;
    }
}

map::~map()
{
    if (!scaledTriples.empty()) {
        qDeleteAll(scaledTriples.begin(), scaledTriples.end());
    }
    if (zValues) {
        delete zValues;
    }
}

bool map::addAxis(axis *axisToAdd)
{
    if (numAxes >= 2) {
        return false;
    }

    /*
    if (axisToAdd->getAddress() >= fileAddr) {
        return false;
    }
    */

    axes[numAxes++] = axisToAdd;

    return true;
}

quint8 map::getNumAxes()
{
    return numAxes;
}

QString map::infoStr()
{
    QString mapString;

    if (!dtc) {
        if (numAxes <= 0) {
            mapString = "Map contains no axes";
        }
        else if (numAxes == 1) {
            mapString = QString::number(axes[0]->getLength()) + "x1";
            mapString += " MAP 0x" + formatHex(fileAddr);
            mapString += " AXIS 0x" + formatHex(axes[0]->getAddress());
            mapString += " (" + QString::number(axes[0]->getWordSize() * 8) + " bit)";
        }
        else if (numAxes == 2) {
            mapString = QString::number(axes[0]->getLength());
            mapString += "x" + QString::number(axes[1]->getLength());
            mapString += " MAP 0x" + formatHex(fileAddr);
            mapString += " AXIS1 0x" + formatHex(axes[0]->getAddress());
            mapString += " (" + QString::number(axes[0]->getWordSize() * 8) + " bit)";
            mapString += " AXIS2 0x" + formatHex(axes[1]->getAddress());
            mapString += " (" + QString::number(axes[1]->getWordSize() * 8) + " bit)";
        }
        else {
            mapString = "Invalid map";
        }
    }
    else {
        mapString = "DTC 0x" + formatHex(fileAddr) + " " + label;
    }

    return mapString;
}

QString map::dimensionsStr()
{
    if (numAxes == 1) {
        return QString::number(axes[0]->getLength()) + "x1";
    }
    else if (numAxes == 2) {
        return QString::number(axes[0]->getLength()) + "x" + QString::number(axes[1]->getLength());
    }
    return "";
}

bool map::isValid()
{
    return (numAxes > 0);
}

bool map::loadData()
{
    if (!fileData || !isValid()) {
        return false;
    }

    if (data) {
        delete data;
    }

    int numBytes = 1;
    for (int i = 0; i < numAxes; i++) {
        numBytes *= axes[i]->getLength();
    }
    numBytes *= wordSize;

    data = new QByteArray(fileData->mid(fileAddr, numBytes));

    return (data > 0);
}

bool map::doScaling() {
    bool firstTime = false;
    bool map1d = false;

    if (scaledTriples.empty()) {
        firstTime = true;
    }

    if (data || loadData()) {
        int xMax = getXDim();
        int yMax = getYDim();

        zero = true;
        flat = true;
        double prev = 0.0;

        if (yMax == 1) {
            yMax = 2;
            map1d = true;
        }

        zValues->clear();

        bool firstPoint = true;
        for (int y = 0; y < yMax; y++) {
            if (firstTime) {
                scaledTriples.append(new tripleVector());
            }

            for (int x = 0; x < xMax; x++) {
                void* point;
                if (!map1d) {
                    point = (void*) (data->constData() + (x * yMax + y) * wordSize);
                }
                else {
                    point = (void*) (data->constData() + x * wordSize);
                }

                double raw = asDouble(point);

                // check for zero map
                if (raw != 0.0) {
                    zero = false;
                }

                // checks for flat maps
                if (firstPoint) {
                    prev = raw;
                    firstPoint = false;
                }
                if (flat) {
                    if (raw != prev) {
                        flat = false;
                    }
                    prev = raw;
                }

                Qwt3D::Triple newTriple;

                newTriple.x = axisValue(0, x);

                if (!map1d) {
                    newTriple.y = axisValue(1, y);
                }
                else {
                    newTriple.y = y;
                }

                newTriple.z = raw * scaleFactor + offset;

                // Vector containing all the z values, used to create tics on the Z axis
                if (!zValues->contains(newTriple.z)) {
                    zValues->append(newTriple.z);
                }

                if (firstTime) {
                    scaledTriples.at(y)->append(newTriple);
                }
                else {
                    scaledTriples.at(y)->replace(x, newTriple);
                }
            }
        }
        qSort(*zValues);
        dataRead = true;

        emit changed();

        if (firstTime) {
            for (int i = 0; i < numAxes; i++) {
                connect(axes[i], SIGNAL(changed()), this, SLOT(axisChanged()));
            }
        }

        return true;
    }
    else {
        return false;
    }
}

Qwt3D::Triple** map::getScaledTriples()
{
    if (!scaledTriples.empty() || doScaling()) {
        for (int i = 0; i < scaledTriples.size(); i++) {
            tempArray[i] = scaledTriples.at(i)->data();
        }
        return tempArray;
    }
    else return 0;
}

int map::getYDim()
{
    if (isValid()) {
        return (numAxes == 1) ? 1 : axes[1]->getLength();
    }
    else {
        return 0;
    }
}

double map::axisValue(int axisNum, int i)
{
    if (axisNum >= numAxes) {
        return 0;
    }
    if (axes[axisNum]->isFlat()) {
        return i;
    }
    return axes[axisNum]->at(i);
}

axis* map::getAxis(int i)
{
    if (numAxes > i) {
        return axes[i];
    }
    return 0;
}

QList<double> *map::getZValues()
{
    if (!zValues->empty() || doScaling()) {
        return zValues;
    }
    return 0;
}

bool map::isDTC()
{
    return dtc;
}

quint32 map::getEndAddress()
{
    return fileAddr + getXDim() * getYDim() * wordSize;
}

double map::getValue(int x, int y)
{
    if (!scaledTriples.empty() || doScaling()) {
        if (x >= getXDim() || y >= getYDim() || x < 0 || y < 0) {
            return -1;
        }
        return scaledTriples.at(y)->at(x).z;
    }
    return -1;
}

const QString &map::getTableText(bool colour)
{
    QStringList tableTextList;
    double min = getZValues()->at(0);
    double max = getZValues()->at(getZValues()->length() - 1);
    double diff = max - min;

    int yAxisWidth = 0;
    if (axes[1]) {
        yAxisWidth = axes[1]->getWidth();
    }
    int xAxisWidth = axes[0]->getWidth();
    int valWidth = getWidth();
    if (xAxisWidth > valWidth) {
        valWidth = xAxisWidth;
    }
    if (isDTC()) {
        valWidth = 2 * getWordSize() + 2;
    }

    QString mapText = "<b>Map    " + formatHex(getAddress(), false, true) + ": " + label;
    if (units.length() != 0) {
        mapText += " (" + units + ")";
    }
    mapText += "</b>";
    tableTextList << mapText;

    if (!axes[0]->isFixed()) {
        QString axis1Text = "<b>X-Axis " + formatHex(axes[0]->getAddress(), false, true) + ": " + axes[0]->getLabel();
        if (axes[0]->getUnits().length() != 0) {
            axis1Text += " (" + axes[0]->getUnits() + ")";
        }
        axis1Text += "</b>";
        tableTextList << axis1Text;
    }
    if (axes[1]) {
        QString axis2Text = "<b>Y-Axis " + formatHex(axes[1]->getAddress(), false, true) + ": " + axes[1]->getLabel();
        if (axes[0]->getUnits().length() != 0) {
            axis2Text += " (" + axes[1]->getUnits() + ")";
        }
        axis2Text += "</b>";
        tableTextList << axis2Text;
    }
    tableTextList << "";

    QString xLabels;
    if (yAxisWidth > 0) {
        xLabels.fill(QChar(' '), yAxisWidth + 1);
    }
    for (int i = 0; i < getXDim(); i++) {
        xLabels += QString("%1 ").arg(axisValue(0, i), valWidth, 'f', axes[0]->getDecimalPoints(), QChar(' '));
    }
    xLabels = "<span style=\"text-decoration:underline; font-weight:bold;\">" + xLabels + "</span>";
    tableTextList << xLabels;

    for (int y = 0; y < getYDim(); y++) {
        QString row;
        if (axes[1]) {
            int dp = axes[1]->getDecimalPoints();
            row = QString("%1 ").arg(axisValue(1, y), yAxisWidth, 'f', dp, QChar(' '));
            row = "<span style=\"font-weight:bold\">" + row + "</span>";
        }

        for (int x = 0; x < getXDim(); x++) {
            int dp = getDecimalPoints();
            if (isDTC()) {
                int val = getValue(x,y);
                row += "0x" + QString("%1 ").arg(val, valWidth-2, 16, QChar('0')).toUpper();
            }
            else {
                double val = getValue(x, y);
                if (colour) {
                    double temp = val - min;
                    if (diff == 0) {
                        temp = 1.0;
                    }
                    else {
                        temp /= diff;
                    }
                    int red = 255*temp;
                    int green = 255-255*temp;
                    row += "<span style=\"background-color: rgb(" + QString::number(red) + "," + QString::number(green) + ",0);\">";
                }
                else {
                    row += "<span>";
                }

                row += QString("%1 </span>").arg(val, valWidth, 'f', dp, QChar(' '));
            }
        }
        tableTextList << row;
    }

    tableText = "<pre style=\"font-size:16px; color:black\">" + tableTextList.join("\n") + "</pre>";
    return tableText;
}

int map::getXDim()
{
    if (isValid()) {
        return axes[0]->getLength();
    }
    else {
        return 0;
    }
}

void map::axisChanged()
{
    doScaling();
}

