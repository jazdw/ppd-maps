#ifndef CONVERSION_H
#define CONVERSION_H

#include "ppddata.h"

class conversion
{
public:
    explicit conversion(ppdData* in, int index);
    QString conversionID() const;
    double scaleFactor;
    double offset;
    int decimalPoints;
    QString units;
    int index;
private:

signals:
    
public slots:
    
};

bool operator==(const conversion& lhs, const conversion& rhs);

#endif // CONVERSION_H
