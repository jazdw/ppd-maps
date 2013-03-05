#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QtEndian>

enum {standardLvl = 0x1, debugLvl = 0x2, logFileLvl = 0x4};

bool compareDoubles(double a, double b);
QString formatHex(quint32 num, bool pad = false, bool ohx = false);

template <class fixedSizeInt>
fixedSizeInt castToInteger(void* ptr, bool bigEndian) {
    fixedSizeInt tmp = *((fixedSizeInt*) ptr);
    if (bigEndian && (sizeof(fixedSizeInt) > 1))
        tmp = qFromBigEndian(tmp);
    return tmp;
}

#endif // UTIL_H
