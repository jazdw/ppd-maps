#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qwt3d_axis.h"
#include "qwt3d_scale.h"

#include "mappreset.h"
#include "mapscale.h"

extern QVector<mapPreset*> presets;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    debugLevel(standardLvl),
    selectedMap(0),
    emptyPlot(0,0,0),
    emptyPlot_1(&emptyPlot),
    emptyPlot_2(&emptyPlot_1),
    settings("PPD-Maps.ini", QSettings::IniFormat)
{
    ui->setupUi(this);
    restoreSettings();

    svnRev = APP_SVN_REV;
    int colonAt = svnRev.indexOf(":");
    if (colonAt > 0) {
        svnRev = svnRev.mid(colonAt+1);
    }
    appVer = APP_VERSION;

    aboutDialog = new about(appVer, svnRev, this);
    QFont mono("");
    mono.setStyleHint(QFont::TypeWriter);
    mono.setFixedPitch(true);
    mono.setPixelSize(12);
    ui->plainTextEdit_output->setFont(mono);

    for (int i = 0; i < presets.size(); i++) {
        ui->presetMap->addItem(presets.at(i)->infoStr());
        ui->presetAxis1->addItem(presets.at(i)->infoStr());
        ui->presetAxis2->addItem(presets.at(i)->infoStr());
    }

    statusWidget = new QLabel("No file loaded.");
    ui->statusBar->addWidget(statusWidget, 1);
    statusWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    workThread = new QThread(this);

    searcher = new mapSearch(ui->actionDiscard_Invalid_Maps->isChecked());
    exporter = new exportCreator(appVer, searcher->getMapList());
    searcher->moveToThread(workThread);
    exporter->moveToThread(workThread);

    tableModel = new mapTableModel(searcher->getMapList(), this);
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortRole(Qt::UserRole);
    proxyModel->setFilterRole(Qt::UserRole+1);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setSourceModel(tableModel);
    connect(ui->lineEdit_filter, SIGNAL(textChanged(QString)), proxyModel, SLOT(setFilterWildcard(QString)));
    connect(tableModel, SIGNAL(refreshed()), this, SLOT(sortMapList()));
    ui->tableView_maps->setModel(proxyModel);
    //ui->tableView_maps->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->tableView_maps->horizontalHeader()->resizeSection(0, 55);
    ui->tableView_maps->horizontalHeader()->resizeSection(1, 50);
    ui->tableView_maps->horizontalHeader()->resizeSection(2, 160);

    QDoubleValidator* dblValidate = new QDoubleValidator(this);
    QIntValidator* intValidate = new QIntValidator(this);
    intValidate->setRange(0,9);

    ui->edit_scaleMap->setValidator(dblValidate);
    ui->edit_offsetMap->setValidator(dblValidate);
    ui->edit_scaleAxis1->setValidator(dblValidate);
    ui->edit_scaleAxis1->setValidator(dblValidate);
    ui->edit_scaleAxis2->setValidator(dblValidate);
    ui->edit_scaleAxis2->setValidator(dblValidate);
    ui->edit_dpMap->setValidator(intValidate);
    ui->edit_dpAxis1->setValidator(intValidate);
    ui->edit_dpAxis2->setValidator(intValidate);

    ui->mapPlot->setCoordinateStyle(Qwt3D::FRAME);
    //ui->mapPlot->showColorLegend(true);
    //ui->mapPlot->setOrtho(false);
    //ui->mapPlot->setTitleFont("Arial", 10);
    //ui->mapPlot->coordinates()->setNumberFont("Arial", 10);
    ui->mapPlot->coordinates()->setAutoScale(true);
    ui->mapPlot->coordinates()->setAutoDecoration(true);
    ui->mapPlot->coordinates()->setLineSmooth(true);
    ui->mapPlot->enableLighting(true);

    connect(ui->actionOpen_BDM_File, SIGNAL(triggered()), this, SLOT(set_filename()));
    connect(this, SIGNAL(send_filename(QString)), searcher, SLOT(open_file(QString)));
    connect(searcher, SIGNAL(log(const QString &, int, bool)), this, SLOT(log(const QString &, int, bool)));
    connect(exporter, SIGNAL(log(const QString &, int, bool)), this, SLOT(log(const QString &, int, bool)));
    connect(searcher, SIGNAL(fileLoaded()), this, SLOT(newFileLoaded()));
    connect(searcher, SIGNAL(resetTableModel()), tableModel, SLOT(refresh()));
    connect(searcher, SIGNAL(resetTableModel()), this, SLOT(flushLogBuffer()));
    connect(ui->tableView_maps, SIGNAL(activated(QModelIndex)), this, SLOT(newMapSelected(QModelIndex)));
    connect(ui->tableView_maps, SIGNAL(clicked(QModelIndex)), this, SLOT(newMapSelected(QModelIndex)));
    connect(ui->presetMap, SIGNAL(activated(int)), this, SLOT(presetSelected(int)));
    connect(ui->presetAxis1, SIGNAL(activated(int)), this, SLOT(presetSelected(int)));
    connect(ui->presetAxis2, SIGNAL(activated(int)), this, SLOT(presetSelected(int)));
    connect(ui->actionDiscard_Invalid_Maps, SIGNAL(toggled(bool)), searcher, SLOT(setDiscardInvalid(bool)));
    connect(ui->actionOrthogonal, SIGNAL(triggered(bool)), ui->mapPlot, SLOT(setOrtho(bool)));

    workThread->start();
}

MainWindow::~MainWindow()
{
    ui->mapPlot->loadFromData(emptyPlot_2, 1, 1);
    ui->mapPlot->updateData();
    ui->mapPlot->updateGL();

    delete ui;

    workThread->exit(0);
    searcher->cancel();

    // wait 5 seconds before terminating
    workThread->wait(5000);
    if (workThread->isRunning()) {
        workThread->terminate();
    }

    //delete searcher;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
}

void MainWindow::hideEvent(QHideEvent *event)
{
    saveDocks();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* eventWindowChange = static_cast<QWindowStateChangeEvent*>(event);
        if (eventWindowChange->oldState() == Qt::WindowMinimized) {
            restoreDocks();
        }
    }
}

void MainWindow::saveDocks()
{
    showMapDock = ui->mapsDock->isVisible();
    showLogDock = ui->logDock->isVisible();
    showParamDock = ui->paramDock->isVisible();
}

void MainWindow::restoreDocks()
{
    ui->mapsDock->setVisible(showMapDock);
    ui->logDock->setVisible(showLogDock);
    ui->paramDock->setVisible(showParamDock);
}

void MainWindow::saveSettings()
{
    settings.setValue("mainWindow/state", saveState());
    settings.setValue("mainWindow/geometry", saveGeometry());
}

void MainWindow::restoreSettings()
{
    restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    restoreState(settings.value("mainWindow/state").toByteArray());
}

void MainWindow::set_filename()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open BDM File", "", "Binary Files (*.bin *.ori *.org *.mpc);;All Files (*.*)");
    if (filename == "") {
        return;
    }
    else {
        if (ui->actionClear_Log_On_Open->isChecked()) {
            ui->plainTextEdit_output->clear();
        }
        log("Opening new file: " + filename, standardLvl, true);
    }

    if (selectedMap) {
        disconnectMap(selectedMap);
        selectedMap = 0;
    }

    ui->mapPlot->loadFromData(emptyPlot_2, 1, 1);
    ui->mapPlot->updateData();
    ui->mapPlot->updateGL();

    emit send_filename(filename);
}

void MainWindow::log(const QString &text, int level, bool flushBuffer)
{
    if (level & debugLevel) {
        //ui->plainTextEdit_output->appendPlainText(text);
        logBuffer << text;
        if (flushBuffer || logBuffer.length() >= 50) {
            flushLogBuffer();
        }
    }

    if (ui->actionWrite_Summary->isChecked() && (level & logFileLvl)) {
        QFile logFile("VER" + appVer + "-SVN" + svnRev + ".log");
        logFile.open(QIODevice::Append);
        QTextStream logFileOut;
        logFileOut.setDevice(&logFile);
        logFileOut << text << endl;
    }
}

void MainWindow::newMapSelected(QModelIndex index)
{
    static double rotation[3] = {30.0, 0.0, 45.0};

    // disconnect previous map from controls
    if (selectedMap) {
        tableModel->clearHighlightedRows();
        disconnectMap(selectedMap);
        // remember rotation parameters
        if (selectedMap->getNumAxes() != 1) {
            rotation[0] = ui->mapPlot->xRotation();
            rotation[1] = ui->mapPlot->yRotation();
            rotation[2] = ui->mapPlot->zRotation();
        }
    }
    selectedMap = 0;

    // get newly selected map and connect it to controls
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    if (!sourceIndex.isValid()) {
        log("Whoa, error looking up map in source data model.", debugLvl, true);
        return;
    }
    selectedMap = tableModel->getMap(sourceIndex);
    refreshUiValues();
    connectMap(selectedMap);
    tableModel->setHighlightedRow(tableModel->getIndex(selectedMap).row());

    for (int i = 0; i < selectedMap->numFriends(); i++) {
        map* parallelMap = (map*) selectedMap->getFriend(i);
        QModelIndex parallelIndex = tableModel->getIndex(parallelMap);
        //parallelIndex = proxyModel->mapFromSource(parallelIndex);
        tableModel->setHighlightedRow(parallelIndex.row());
    }

    refreshPlot();

    connect(selectedMap, SIGNAL(changed()), this, SLOT(refreshPlot()));

    if (selectedMap->getNumAxes() == 1) {
        ui->mapPlot->setRotation(0,0,0);
    }
    else {
        ui->mapPlot->setRotation(rotation[0], rotation[1], rotation[2]);
    }
}

void MainWindow::refreshPlot()
{
    if (!selectedMap) {
        return;
    }

    int row = tableModel->getIndex(selectedMap).row();
    tableModel->updateRow(row);

    ui->mapPlot->setTitle(selectedMap->getLabel());
    //ui->mapPlot->setTitle(selectedMap->infoStr());
    int yDim = selectedMap->getYDim();
    if (yDim == 1) {
        yDim = 2;
    }
    ui->mapPlot->loadFromData(selectedMap->getScaledTriples(), yDim, selectedMap->getXDim());

    Qwt3D::CoordinateSystem* coords = ui->mapPlot->coordinates();
    int axis1dp = selectedMap->getAxis(0)->getDecimalPoints();
    coords->axes[Qwt3D::X1].setScale(new mapScale(selectedMap->getAxis(0), axis1dp));
    coords->axes[Qwt3D::X2].setScale(new mapScale(selectedMap->getAxis(0), axis1dp));
    coords->axes[Qwt3D::X3].setScale(new mapScale(selectedMap->getAxis(0), axis1dp));
    coords->axes[Qwt3D::X4].setScale(new mapScale(selectedMap->getAxis(0), axis1dp));
    if (selectedMap->getNumAxes() > 1) {
        int axis2dp = selectedMap->getAxis(1)->getDecimalPoints();
        coords->axes[Qwt3D::Y1].setScale(new mapScale(selectedMap->getAxis(1), axis2dp));
        coords->axes[Qwt3D::Y2].setScale(new mapScale(selectedMap->getAxis(1), axis2dp));
        coords->axes[Qwt3D::Y3].setScale(new mapScale(selectedMap->getAxis(1), axis2dp));
        coords->axes[Qwt3D::Y4].setScale(new mapScale(selectedMap->getAxis(1), axis2dp));
    }
    int mapdp = selectedMap->getDecimalPoints();
    coords->axes[Qwt3D::Z1].setScale(new mapScale(selectedMap->getZValues(), mapdp));
    coords->axes[Qwt3D::Z2].setScale(new mapScale(selectedMap->getZValues(), mapdp));
    coords->axes[Qwt3D::Z3].setScale(new mapScale(selectedMap->getZValues(), mapdp));
    coords->axes[Qwt3D::Z4].setScale(new mapScale(selectedMap->getZValues(), mapdp));

    QString xLabel, yLabel, zLabel;
    xLabel = selectedMap->getAxis(0)->getLabel();
    if (selectedMap->getAxis(0)->getUnits() != "") {
        xLabel += "(" + selectedMap->getAxis(0)->getUnits() + ")";
    }
    if (selectedMap->getNumAxes() > 1) {
        yLabel = selectedMap->getAxis(1)->getLabel();
        if (selectedMap->getAxis(1)->getUnits() != "") {
            yLabel += "(" + selectedMap->getAxis(1)->getUnits() + ")";
        }
    }
    else {
        yLabel = "";
    }
    zLabel = selectedMap->getUnits();

    coords->axes[Qwt3D::X1].setLabelString(xLabel);
    coords->axes[Qwt3D::X2].setLabelString(xLabel);
    coords->axes[Qwt3D::X3].setLabelString(xLabel);
    coords->axes[Qwt3D::X4].setLabelString(xLabel);
    coords->axes[Qwt3D::Y1].setLabelString(yLabel);
    coords->axes[Qwt3D::Y2].setLabelString(yLabel);
    coords->axes[Qwt3D::Y3].setLabelString(yLabel);
    coords->axes[Qwt3D::Y4].setLabelString(yLabel);
    coords->axes[Qwt3D::Z1].setLabelString(zLabel);
    coords->axes[Qwt3D::Z2].setLabelString(zLabel);
    coords->axes[Qwt3D::Z3].setLabelString(zLabel);
    coords->axes[Qwt3D::Z4].setLabelString(zLabel);

    // calcs for scaling and tic length
    double mins[3], maxs[3], diff[3], scale[3], biggestDiff = 0;
    for (int i = 0; i < 3; i++) {
        coords->axes.at(i).limits(mins[i], maxs[i]);
        diff[i] = maxs[i] - mins[i];
        if (diff[i] > biggestDiff) {
            biggestDiff = diff[i];
        }
    }
    for (int i = 0; i < 3; i++) {
        if (diff[i] == 0) {
            scale[i] = 1.0;
            continue;
        }
        scale[i] = (1.0 / diff[i]) * biggestDiff;
    }

    double majorLength = 0.1 * biggestDiff;
    double minorLength = 0.03 * biggestDiff;

    if (ui->actionAutoFit->isChecked()) {
        ui->mapPlot->setZoom(0.4);
        ui->mapPlot->setScale(scale[0], scale[1], scale[2]);

        // workaround to undo scaling on tics
        double xdir = 1.0 / scale[1];
        double ydir = 1.0 / scale[0];

        coords->axes[Qwt3D::X1].setTicLength(majorLength * xdir, minorLength * xdir);
        coords->axes[Qwt3D::X2].setTicLength(majorLength * xdir, minorLength * xdir);
        coords->axes[Qwt3D::X3].setTicLength(majorLength * xdir, minorLength * xdir);
        coords->axes[Qwt3D::X4].setTicLength(majorLength * xdir, minorLength * xdir);
        coords->axes[Qwt3D::Y1].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Y2].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Y3].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Y4].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Z1].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Z2].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Z3].setTicLength(majorLength * ydir, minorLength * ydir);
        coords->axes[Qwt3D::Z4].setTicLength(majorLength * ydir, minorLength * ydir);
    }
    else {
        ui->mapPlot->setZoom(0.9);

        coords->axes[Qwt3D::X1].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::X2].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::X3].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::X4].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Y1].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Y2].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Y3].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Y4].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Z1].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Z2].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Z3].setTicLength(majorLength, minorLength);
        coords->axes[Qwt3D::Z4].setTicLength(majorLength, minorLength);
    }

    ui->mapPlot->updateData();
    ui->mapPlot->updateGL();

    ui->textEdit_table->setText(selectedMap->getTableText(ui->checkBox_colourise->isChecked()));
}

void MainWindow::refreshUiValues()
{
    if (!selectedMap)
        return;

    // update map UI elements
    ui->labelMap->setText(selectedMap->getLabel());
    ui->unitsMap->setText(selectedMap->getUnits());
    ui->edit_scaleMap->setText(QString::number(selectedMap->getScaleFactor()));
    ui->edit_offsetMap->setText(QString::number(selectedMap->getOffset()));
    ui->presetMap->setCurrentIndex(0);
    switch (selectedMap->getWordSize()) {
    case 1:
        ui->button_8bitMap->setChecked(true);
        break;
    case 2:
        ui->button_16bitMap->setChecked(true);
        break;
    case 4:
        ui->button_32bitMap->setChecked(true);
        break;
    }

    if (selectedMap->getSigned()) {
        ui->signedMap->setChecked(true);
    }
    else {
        ui->unsignedMap->setChecked(true);
    }

    if (selectedMap->getEndian()) {
        ui->bigEndianMap->setChecked(true);
    }
    else {
        ui->littleEndianMap->setChecked(true);
    }

    ui->edit_dpMap->setText(QString::number(selectedMap->getDecimalPoints()));

    if (selectedMap->getUpdateFriends()) {
        ui->updateParallel->setChecked(true);
    }
    else {
        ui->updateParallel->setChecked(false);
    }
    ui->addrMap->setText(selectedMap->getAddressStr(false, true));

    // update axis 1 UI elements
    ui->labelAxis1->setText(selectedMap->getAxis(0)->getLabel());
    ui->unitsAxis1->setText(selectedMap->getAxis(0)->getUnits());
    ui->edit_scaleAxis1->setText(QString::number(selectedMap->getAxis(0)->getScaleFactor()));
    ui->edit_offsetAxis1->setText(QString::number(selectedMap->getAxis(0)->getOffset()));
    ui->presetAxis1->setCurrentIndex(0);
    switch (selectedMap->getAxis(0)->getWordSize()) {
    case 1:
        ui->button_8bitAxis1->setChecked(true);
        break;
    case 2:
        ui->button_16bitAxis1->setChecked(true);
        break;
    case 4:
        ui->button_32bitAxis1->setChecked(true);
        break;
    }

    if (selectedMap->getAxis(0)->getSigned()) {
        ui->signedAxis1->setChecked(true);
    }
    else {
        ui->unsignedAxis1->setChecked(true);
    }

    if (selectedMap->getAxis(0)->getEndian()) {
        ui->bigEndianAxis1->setChecked(true);
    }
    else {
        ui->littleEndianAxis1->setChecked(true);
    }

    ui->edit_dpAxis1->setText(QString::number(selectedMap->getAxis(0)->getDecimalPoints()));
    ui->addrAxis1->setText(selectedMap->getAxis(0)->getAddressStr(false, true));

    // update axis 2 UI elements (if it exists, otherwise disable)
    if (selectedMap->getAxis(1)) {
        if (!ui->tabAxis2->isEnabled()) {
            ui->tabAxis2->setDisabled(false);
            ui->tabWidget_params->insertTab(2, ui->tabAxis2, "Axis 2");
        }
        ui->labelAxis2->setText(selectedMap->getAxis(1)->getLabel());
        ui->unitsAxis2->setText(selectedMap->getAxis(1)->getUnits());
        ui->edit_scaleAxis2->setText(QString::number(selectedMap->getAxis(1)->getScaleFactor()));
        ui->edit_offsetAxis2->setText(QString::number(selectedMap->getAxis(1)->getOffset()));
        ui->presetAxis2->setCurrentIndex(0);
        switch (selectedMap->getAxis(1)->getWordSize()) {
        case 1:
            ui->button_8bitAxis2->setChecked(true);
            break;
        case 2:
            ui->button_16bitAxis2->setChecked(true);
            break;
        case 4:
            ui->button_32bitAxis2->setChecked(true);
            break;
        }

        if (selectedMap->getAxis(1)->getSigned()) {
            ui->signedAxis2->setChecked(true);
        }
        else {
            ui->unsignedAxis2->setChecked(true);
        }

        if (selectedMap->getAxis(1)->getEndian()) {
            ui->bigEndianAxis2->setChecked(true);
        }
        else {
            ui->littleEndianAxis2->setChecked(true);
        }

        ui->edit_dpAxis2->setText(QString::number(selectedMap->getAxis(1)->getDecimalPoints()));
        ui->addrAxis2->setText(selectedMap->getAxis(1)->getAddressStr(false, true));
    }
    else {
        ui->labelAxis2->setText("");
        ui->edit_scaleAxis2->setText("");
        ui->edit_offsetAxis2->setText("");
        ui->tabWidget_params->removeTab(2);
        ui->tabAxis2->setDisabled(true);
        ui->addrAxis2->setText("");
    }
}

void MainWindow::newFileLoaded()
{
    statusWidget->setText(searcher->getInfoStr());
    //ui->tableView_maps->resizeColumnsToContents();
}

void MainWindow::presetSelected(int item)
{
    mapPreset* preset = presets.at(item);
    if (QObject::sender() == ui->presetMap) {
        ui->labelMap->setText(preset->label);
        ui->unitsMap->setText(preset->units);
        ui->edit_scaleMap->setText(QString::number(preset->scale));
        ui->edit_offsetMap->setText(QString::number(preset->offset));
        ui->edit_dpMap->setText(QString::number(preset->decimalPoints));
    }
    else if (QObject::sender() == ui->presetAxis1) {
        ui->labelAxis1->setText(preset->label);
        ui->unitsAxis1->setText(preset->units);
        ui->edit_scaleAxis1->setText(QString::number(preset->scale));
        ui->edit_offsetAxis1->setText(QString::number(preset->offset));
        ui->edit_dpAxis1->setText(QString::number(preset->decimalPoints));
    }
    else if (QObject::sender() == ui->presetAxis2) {
        ui->labelAxis2->setText(preset->label);
        ui->unitsAxis2->setText(preset->units);
        ui->edit_scaleAxis2->setText(QString::number(preset->scale));
        ui->edit_offsetAxis2->setText(QString::number(preset->offset));
        ui->edit_dpAxis2->setText(QString::number(preset->decimalPoints));
    }
}

void MainWindow::sortMapList()
{
    ui->lineEdit_filter->clear();
    ui->tableView_maps->sortByColumn(0, Qt::AscendingOrder);
}

void MainWindow::on_actionReset_Docks_triggered()
{
    ui->mapsDock->setFloating(false);
    this->removeDockWidget(ui->mapsDock);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->mapsDock);
    ui->mapsDock->setVisible(true);

    ui->paramDock->setFloating(false);
    this->removeDockWidget(ui->paramDock);
    this->addDockWidget(Qt::RightDockWidgetArea, ui->paramDock);
    ui->paramDock->setVisible(true);

    ui->logDock->setFloating(false);
    this->removeDockWidget(ui->logDock);
    this->addDockWidget(Qt::BottomDockWidgetArea, ui->logDock);
    ui->logDock->setVisible(true);
}

void MainWindow::on_actionAbout_triggered()
{
    aboutDialog->show();
}

void MainWindow::on_actionExport_to_XDF_triggered()
{
    if (!searcher->fileIsValid() || searcher->getBdmFileSize() <= 0 || searcher->getMapList()->length() == 0) {
        return;
    }

    QString format("XDF Files (*.xdf)");
    QString filename = QFileDialog::getSaveFileName(this, "Save XDF File", "", "XDF Files (*.xdf);;All Files (*.*)", &format);
    if (filename == "") {
        return;
    }
    if (format == "XDF Files (*.xdf)") {
        if (!filename.endsWith(".xdf", Qt::CaseInsensitive)) {
            filename += ".xdf";
        }
    }

    exporter->setFilename(filename);
    exporter->setBinSize(searcher->getBdmFileSize());
    QMetaObject::invokeMethod(exporter, "exportXDF", Qt::QueuedConnection,
                              Q_ARG(bool, ui->actionExport_Flat->isChecked()),
                              Q_ARG(bool, ui->actionExport_Zero->isChecked()),
                              Q_ARG(bool, ui->actionExport_1D_Maps->isChecked()),
                              Q_ARG(bool, ui->actionExport_DTCs->isChecked()));

}

void MainWindow::on_actionExport_to_A2L_triggered()
{
    if (!searcher->fileIsValid() || searcher->getBdmFileSize() <= 0 || searcher->getMapList()->length() == 0) {
        return;
    }

    QString format("A2L Files (*.a2l)");
    QString filename = QFileDialog::getSaveFileName(this, "Save A2L File", "", "A2L Files (*.a2l);;All Files (*.*)", &format);
    if (filename == "") {
        return;
    }
    if (format == "A2L Files (*.a2l)") {
        if (!filename.endsWith(".a2l", Qt::CaseInsensitive)) {
            filename += ".a2l";
        }
    }

    exporter->setFilename(filename);
    exporter->setBinSize(searcher->getBdmFileSize());
    QMetaObject::invokeMethod(exporter, "exportA2L", Qt::QueuedConnection,
                              Q_ARG(bool, ui->actionExport_Flat->isChecked()),
                              Q_ARG(bool, ui->actionExport_Zero->isChecked()),
                              Q_ARG(bool, ui->actionExport_1D_Maps->isChecked()),
                              Q_ARG(bool, ui->actionExport_DTCs->isChecked()));
}

void MainWindow::flushLogBuffer()
{
    if (!logBuffer.empty()) {
        ui->plainTextEdit_output->appendPlainText(logBuffer.join("\n"));
        logBuffer.clear();
    }
}

void MainWindow::connectMap(map *target)
{
    connect(ui->labelMap, SIGNAL(textChanged(QString)), target, SLOT(setLabel(QString)));
    connect(ui->unitsMap, SIGNAL(textChanged(QString)), target, SLOT(setUnits(QString)));
    connect(ui->edit_scaleMap, SIGNAL(textChanged(QString)), target, SLOT(setScaleFactor(QString)));
    connect(ui->edit_offsetMap, SIGNAL(textChanged(QString)), target, SLOT(setOffset(QString)));
    connect(ui->button_8bitMap, SIGNAL(clicked()), target, SLOT(set8bit()));
    connect(ui->button_16bitMap, SIGNAL(clicked()), target, SLOT(set16bit()));
    connect(ui->button_32bitMap, SIGNAL(clicked()), target, SLOT(set32bit()));
    connect(ui->signedMap, SIGNAL(clicked()), target, SLOT(setSigned()));
    connect(ui->unsignedMap, SIGNAL(clicked()), target, SLOT(setUnsigned()));
    connect(ui->bigEndianMap, SIGNAL(clicked()), target, SLOT(setBigEndian()));
    connect(ui->littleEndianMap, SIGNAL(clicked()), target, SLOT(setLittleEndian()));
    connect(ui->edit_dpMap, SIGNAL(textChanged(QString)), target, SLOT(setDecimalPoints(QString)));
    connect(ui->updateParallel, SIGNAL(toggled(bool)), target, SLOT(setUpdateFriends(bool)));

    connect(ui->labelAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setLabel(QString)));
    connect(ui->unitsAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setUnits(QString)));
    connect(ui->edit_scaleAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setScaleFactor(QString)));
    connect(ui->edit_offsetAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setOffset(QString)));
    connect(ui->button_8bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set8bit()));
    connect(ui->button_16bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set16bit()));
    connect(ui->button_32bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set32bit()));
    connect(ui->signedAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setSigned()));
    connect(ui->unsignedAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setUnsigned()));
    connect(ui->bigEndianAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setBigEndian()));
    connect(ui->littleEndianAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setLittleEndian()));
    connect(ui->edit_dpAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setDecimalPoints(QString)));

    if (target->getAxis(1)) {
        connect(ui->labelAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setLabel(QString)));
        connect(ui->unitsAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setUnits(QString)));
        connect(ui->edit_scaleAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setScaleFactor(QString)));
        connect(ui->edit_offsetAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setOffset(QString)));
        connect(ui->button_8bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set8bit()));
        connect(ui->button_16bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set16bit()));
        connect(ui->button_32bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set32bit()));
        connect(ui->signedAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setSigned()));
        connect(ui->unsignedAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setUnsigned()));
        connect(ui->bigEndianAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setBigEndian()));
        connect(ui->littleEndianAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setLittleEndian()));
        connect(ui->edit_dpAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setDecimalPoints(QString)));
    }
}

void MainWindow::disconnectMap(map *target)
{
    disconnect(target, SIGNAL(changed()), this, SLOT(refreshPlot()));

    disconnect(ui->labelMap, SIGNAL(textChanged(QString)), target, SLOT(setLabel(QString)));
    disconnect(ui->unitsMap, SIGNAL(textChanged(QString)), target, SLOT(setUnits(QString)));
    disconnect(ui->edit_scaleMap, SIGNAL(textChanged(QString)), target, SLOT(setScaleFactor(QString)));
    disconnect(ui->edit_offsetMap, SIGNAL(textChanged(QString)), target, SLOT(setOffset(QString)));
    disconnect(ui->button_8bitMap, SIGNAL(clicked()), target, SLOT(set8bit()));
    disconnect(ui->button_16bitMap, SIGNAL(clicked()), target, SLOT(set16bit()));
    disconnect(ui->button_32bitMap, SIGNAL(clicked()), target, SLOT(set32bit()));
    disconnect(ui->signedMap, SIGNAL(clicked()), target, SLOT(setSigned()));
    disconnect(ui->unsignedMap, SIGNAL(clicked()), target, SLOT(setUnsigned()));
    disconnect(ui->bigEndianMap, SIGNAL(clicked()), target, SLOT(setBigEndian()));
    disconnect(ui->littleEndianMap, SIGNAL(clicked()), target, SLOT(setLittleEndian()));
    disconnect(ui->edit_dpMap, SIGNAL(textChanged(QString)), target, SLOT(setDecimalPoints(QString)));
    disconnect(ui->updateParallel, SIGNAL(toggled(bool)), target, SLOT(setUpdateFriends(bool)));

    disconnect(ui->labelAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setLabel(QString)));
    disconnect(ui->unitsAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setUnits(QString)));
    disconnect(ui->edit_scaleAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setScaleFactor(QString)));
    disconnect(ui->edit_offsetAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setOffset(QString)));
    disconnect(ui->button_8bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set8bit()));
    disconnect(ui->button_16bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set16bit()));
    disconnect(ui->button_32bitAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(set32bit()));
    disconnect(ui->signedAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setSigned()));
    disconnect(ui->unsignedAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setUnsigned()));
    disconnect(ui->bigEndianAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setBigEndian()));
    disconnect(ui->littleEndianAxis1, SIGNAL(clicked()), target->getAxis(0), SLOT(setLittleEndian()));
    disconnect(ui->edit_dpAxis1, SIGNAL(textChanged(QString)), target->getAxis(0), SLOT(setDecimalPoints(QString)));

    if (target->getAxis(1)) {
        disconnect(ui->labelAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setLabel(QString)));
        disconnect(ui->unitsAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setUnits(QString)));
        disconnect(ui->edit_scaleAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setScaleFactor(QString)));
        disconnect(ui->edit_offsetAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setOffset(QString)));
        disconnect(ui->button_8bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set8bit()));
        disconnect(ui->button_16bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set16bit()));
        disconnect(ui->button_32bitAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(set32bit()));
        disconnect(ui->signedAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setSigned()));
        disconnect(ui->unsignedAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setUnsigned()));
        disconnect(ui->bigEndianAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setBigEndian()));
        disconnect(ui->littleEndianAxis2, SIGNAL(clicked()), target->getAxis(1), SLOT(setLittleEndian()));
        disconnect(ui->edit_dpAxis2, SIGNAL(textChanged(QString)), target->getAxis(1), SLOT(setDecimalPoints(QString)));
    }
}

void MainWindow::on_checkBox_colourise_toggled(bool checked)
{
    ui->textEdit_table->setText(selectedMap->getTableText(checked));
}

void MainWindow::on_actionMore_Verbose_triggered(bool checked)
{
    if (checked) {
        debugLevel = standardLvl | debugLvl;
    }
    else {
        debugLevel = standardLvl;
    }
}
