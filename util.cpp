#include "util.h"

bool compareDoubles(double a, double b)
{
    int aInt = a * 100;
    int bInt = b * 100;
    return (aInt == bInt);
}

QString formatHex(quint32 num, bool pad, bool ohX)
{
    QString temp;
    if (ohX) {
        temp = "0x";
    }
    if (pad) {
        return temp += QString("%1").arg(num, 8, 16, QChar('0')).toUpper();
    }
    return temp += QString::number(num, 16).toUpper();
}
