#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QThread>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QSettings>
#include "util.h"
#include "mapsearch.h"
#include "maptablemodel.h"
#include "about.h"
#include "exportcreator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void set_filename();
    void log(const QString &text, int level = standardLvl, bool flushBuffer = false);
    void newMapSelected(QModelIndex index);
    void refreshPlot();
    void refreshUiValues();
    void newFileLoaded();
    void presetSelected(int item);
    void sortMapList();
signals:
    void send_filename(QString filename);
private slots:
    void on_actionReset_Docks_triggered();
    void on_actionAbout_triggered();
    void on_actionExport_to_XDF_triggered();
    void on_actionExport_to_A2L_triggered();
    void flushLogBuffer();
    void on_checkBox_colourise_toggled(bool checked);
    void on_actionMore_Verbose_triggered(bool checked);
private:
    Ui::MainWindow *ui;
    mapSearch* searcher;
    exportCreator* exporter;
    QThread* workThread;
    int debugLevel;
    mapTableModel* tableModel;
    QSortFilterProxyModel* proxyModel;
    map* selectedMap;
    QLabel* statusWidget;
    about* aboutDialog;
    void connectMap(map* target);
    void disconnectMap(map* target);
    Qwt3D::Triple emptyPlot;
    Qwt3D::Triple* emptyPlot_1;
    Qwt3D::Triple** emptyPlot_2;
    QStringList logBuffer;
    QString svnRev;
    QString appVer;
    QSettings settings;
    bool showMapDock;
    bool showLogDock;
    bool showParamDock;

    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
    void changeEvent(QEvent *event);
    void saveSettings();
    void restoreSettings();
    void saveDocks();
    void restoreDocks();
};

#endif // MAINWINDOW_H
