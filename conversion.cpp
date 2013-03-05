#include "conversion.h"

conversion::conversion(ppdData *in, int index) :
    index(index)
{
    units = in->getUnits();
    scaleFactor = in->getScaleFactor();
    offset = in->getOffset();
    decimalPoints = in->getDecimalPoints();
}

QString conversion::conversionID() const
{
    return "CNV_" + QString::number(index);
}


bool operator==(const conversion& lhs, const conversion& rhs) {
    if (lhs.decimalPoints == rhs.decimalPoints &&
            lhs.scaleFactor == rhs.scaleFactor &&
            lhs.offset == rhs.offset &&
            lhs.units == rhs.units) {
        return true;
    }

    return false;
}
