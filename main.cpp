#include <QtGui/QApplication>
#include "mainwindow.h"
#include "mappreset.h"

QVector<mapPreset*> presets;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("JaredWiltshire");
    QCoreApplication::setOrganizationDomain("jazdw.net");
    QCoreApplication::setApplicationName("PPD-Maps");


    presets.append(new mapPreset("","", 1, 0));
    presets.append(new mapPreset("Pressure","hPa", 12.06017666543982, 0, 1));
    presets.append(new mapPreset("Pressure","hPa", 12.06017666543982, 32768, 1));
    presets.append(new mapPreset("Flow","kg/h", 64, 0, 2));
    presets.append(new mapPreset("Speed","km/h", 1, 0, 1));
    presets.append(new mapPreset("Speed","km/h", 128, 0, 1));
    presets.append(new mapPreset("Quantity","mg/stk", 250, 0, 2));
    presets.append(new mapPreset("Quantity","mg/stk", 250, 5000, 2));
    presets.append(new mapPreset("Quantity","mg/stk", 250, 32768, 2));
    presets.append(new mapPreset("Quantity","mg/stk", 47.18214542836573074154067674586, 0, 2));
    presets.append(new mapPreset("Quantity","mg/stk", 47.18214542836573074154067674586, 32768, 2));
    presets.append(new mapPreset("Torque","Nm", 32, 0));
    presets.append(new mapPreset("Torque","Nm", 32, 32768));
    presets.append(new mapPreset("Torque","Nm", 64, 0));
    presets.append(new mapPreset("ES","rpm", 1, 0));
    presets.append(new mapPreset("Time","s", 10, 0));
    presets.append(new mapPreset("Time","s", 100, 0));
    presets.append(new mapPreset("Voltage","V", 204.8, 0, 2));
    presets.append(new mapPreset("Voltage","V", 26.5625/3, 0, 2));
    presets.append(new mapPreset("Voltage","V", 13108, 0, 2));
    presets.append(new mapPreset("Temperature",QString::fromUtf8("\u00B0C"), 1, 50, 2));
    presets.append(new mapPreset("Temperature",QString::fromUtf8("\u00B0C"), 16, 0, 2));
    presets.append(new mapPreset("Temperature",QString::fromUtf8("\u00B0C"), 64, 0, 2));
    presets.append(new mapPreset("Crank Angle",QString::fromUtf8("\u00B0CRK"), 128.0/3, 0, 2));
    presets.append(new mapPreset("Crank Angle",QString::fromUtf8("\u00B0CRK"), 128.0/3, 5120, 2));
    presets.append(new mapPreset("Crank Angle",QString::fromUtf8("\u00B0CRK"), 128.0/3, 32768, 2));
    presets.append(new mapPreset("Percent","%", 655.36, 0, 2));
    presets.append(new mapPreset("Percent","%", 655.36, 32768, 2));
    presets.append(new mapPreset("Percent","%", 327.68, 0, 2));
    presets.append(new mapPreset("Percent","%", 327.68, 32768, 2));
    presets.append(new mapPreset("Factor",":1", 1000, 0, 2));
    presets.append(new mapPreset("Factor",":1", 10000, 0, 2));
    presets.append(new mapPreset("Factor",":1", 16384, 0, 2));
    presets.append(new mapPreset("Factor",":1", 32768, 0, 2));
    presets.append(new mapPreset("Factor",":1", 65536, 0, 2));

    MainWindow w;
    w.show();
    
    int ret = a.exec();

    qDeleteAll(presets.begin(), presets.end());
    return ret;
}
