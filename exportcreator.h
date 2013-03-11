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

#ifndef XDFCREATOR_H
#define XDFCREATOR_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QList>
#include "util.h"
#include "map.h"
#include "mapsearch.h"

class exportCreator : public QObject
{
    Q_OBJECT
public:
    explicit exportCreator(QString version, QList<map*>* mapList, QObject *parent = 0);
    void setFilename(QString filename);
    void setBinSize(int binSize);
signals:
    void log(const QString &text, int level = standardLvl, bool flushBuffer = false);
public slots:
    void exportXDF(bool exportFlat = true, bool exportZero = true, bool export1D = true, bool exportDTCs = true);
    void exportA2L(bool exportFlat = true, bool exportZero = true, bool export1D = true, bool exportDTCs = true);
private:
    QString filename;
    QFile* outFile;
    QTextStream out;
    QString version;
    int binSize;
    QList<map*>* mapList;
    void printA2LHeader();
    void printA2LFooter();
    QString formatIndex(int i);
    QString formatFloat(double num);
};

#endif // XDFCREATOR_H
