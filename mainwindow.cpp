#include "mainwindow.h"
#include "ui_mainwindow.h"

char appName[] = {'i', 'n', '.', 'D', 'e', 't', 'a', 'i', 'l', ' ','t','e','r','m',' ', 'V', '1', '.', '0'};

profileStruct tProfileSave[MaxProfile];
profileStruct pProfileSave[MaxProfile];
profileStruct vProfileSave[MaxProfile];

profileStruct tProfileEdit[MaxProfile];
profileStruct pProfileEdit[MaxProfile];
profileStruct vProfileEdit[MaxProfile];

profileStruct tProfileLoad[MaxProfile];
profileStruct pProfileLoad[MaxProfile];
profileStruct vProfileLoad[MaxProfile];

PLC myPLC;

quint8 currentProfile = 0;
quint8 currentTStep = 0;
quint8 currentPStep = 0;
quint8 currentVStep = 0;

double oldTValue = 0;
double oldPValue = 0;
double oldVValue = 0;

int tempPeriod;
int vibPeriod;
int pressurePeriod;

double tKey = 0;
double pKey = 0;
double vKey = 0;

float pipe1Pressure;
float pipe2Pressure;
float pipe3Pressure;
float pipe4Pressure;
float pipe5Pressure;
float pipe6Pressure;
float cabinBottomTemperature;
float cabinTopTemperature;
float waterTankTemperature;
bool waterTankLiquidLevel;
float pipeVibrationFrequency;

quint32 tElapsedSeconds;
quint32 pElapsedSeconds;
quint32 vElapsedSeconds;
quint8 tStep;
quint8 pStep;
quint8 vStep;
quint32 pStepRepeat;
quint32 vStepRepeat;
quint16 tCycle;
quint16 pCycle;
quint16 vCycle;

quint32 totalTestDuration;

quint8 askCounter = 1;

QString testFolder;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle(appName);

    //    QPixmap bkgnd("png\\background.jpg");
    //    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
    //    QPalette palette;
    //    palette.setBrush(QPalette::Background, bkgnd);
    //    ui->tab_Main->setPalette(palette);
    //    ui->tab_Details->setPalette(palette);
    //    ui->tab_EditPro->setPalette(palette);
    //    ui->tab_NewPro->setPalette(palette);
    //    ui->tab_Manual->setPalette(palette);
    //    ui->tab_Maintenance->setPalette(palette);

#ifdef Q_OS_LINUX
    //linux code goes here
    if (!QDir("/home/pi/InDetail/profiles").exists())
    {
        QDir().mkdir("/home/pi/InDetail/profiles");
    }
    if (!QDir("/home/pi/InDetail/records").exists())
    {
        QDir().mkdir("/home/pi/InDetail/records");
    }
    if (!QDir("/home/pi/InDetail/screenshots").exists())
    {
        QDir().mkdir("/home/pi/InDetail/screenshots");
    }
#endif
#ifdef Q_OS_WIN
    // windows code goes here
    if (!QDir("profiles").exists())
    {
        QDir().mkdir("profiles");
    }
    if (!QDir("records").exists())
    {
        QDir().mkdir("records");
    }
    if (!QDir("screenshots").exists())
    {
        QDir().mkdir("screenshots");
    }
#endif

    serial = new mySerial(this);

    connect(serial, &mySerial::messageSerial, this, &MainWindow::serialMessage);

    serial->startSerial();
    proc = new SerialProcess(this);
    connect(proc, &SerialProcess::write,serial, &mySerial::writeSerial);
    connect(proc, &SerialProcess::profileReady, this, &MainWindow::profileSent);
    connect(proc, &SerialProcess::commStatus, this, &MainWindow::commInfo);

    setupTGraph();
    setupVGraph();
    setupPGraphs();
    setupPreviewGraphs();
    setupVisuals();

    setupComboBoxes();

    timer1000 = new QTimer(this);
    connect(timer1000, SIGNAL(timeout()), this, SLOT(getCurrentDateTime()));
    timer1000->start(1000);

    timer250 = new QTimer(this);
    connect(timer250, SIGNAL(timeout()), this, SLOT(askOtherStuff()));
    timer250->start(1000);

    timerTemp = new QTimer(this);
    connect(timerTemp, SIGNAL(timeout()), this, SLOT(updateTPlot()));
    /*
    timerVib = new QTimer(this);
    connect(timerVib, SIGNAL(timeout()), this, SLOT(updateVPlot()));
*/
    timerPressure = new QTimer(this);
    connect(timerPressure, SIGNAL(timeout()), this, SLOT(updatePPlots()));

    //myPLC.cabinTemperatureStat = true;
    //myPLC.tankTemperatureStat = true;
    //myPLC.pipePressureStat = true;

    askSensorValues();



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn;
    if ( myPLC.deviceState == char(0x02) )
    {
        resBtn = QMessageBox::question( this, appName,
                                        tr("There is a test in progress. Are you sure you want to exit?"),
                                        QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                        QMessageBox::Yes);
    }
    else
    {
        resBtn = QMessageBox::question( this, appName,
                                        tr("Are you sure you want to exit?"),
                                        QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                        QMessageBox::Yes);
    }

    if (resBtn != QMessageBox::Yes)
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

void MainWindow::profileSent()
{
    ui->bStartTest->setEnabled(true);
    ui->bStartTestManual->setEnabled(true);
    proc->start();
}

void MainWindow::commInfo(bool status)
{
    if (status == true)
    {
        ui->laCommErr->setStyleSheet("QLabel { color : green; }");
        ui->laCommErr->setText("OK");
    }
    else
    {
        ui->laCommErr->setStyleSheet("QLabel { color : red; }");
        ui->laCommErr->setText("NOK");
        proc->stop();
        proc->commandMessages.clear();
        proc->profileMessages.clear();
    }

}

void MainWindow::setupTGraph()
{
    // make a time ticker
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");

    // make a vertical Ticker for temperature
    QSharedPointer<QCPAxisTicker> tTicker(new QCPAxisTicker);
    tTicker->setTickCount(15);

    // make temperature plot general settings
    ui->tTestGraph->addGraph();
    ui->tTestGraph->graph(0)->setPen(QPen(Qt::red));
    ui->tTestGraph->addGraph();
    ui->tTestGraph->graph(1)->setPen(QPen(Qt::black));
    ui->tTestGraph->setInteractions(QCP::iRangeDrag | QCP::iSelectAxes |
                                    QCP::iSelectLegend| QCP::iRangeZoom | QCP::iSelectPlottables);
    //ui->tTestGraph->axisRect()->setRangeDrag(Qt::Horizontal);
    //ui->tTestGraph->axisRect()->setRangeZoom(Qt::Horizontal);



    // make Y axis temperature ticker
    ui->tTestGraph->yAxis->setTicker(tTicker);


    // make X axis as time ticker
    ui->tTestGraph->xAxis->setTicker(timeTicker);
    ui->tTestGraph->axisRect()->setupFullAxesBox();
    ui->tTestGraph->xAxis->setLabel("Time (hh:mm:ss)");
    ui->tTestGraph->yAxis->setLabel("Cabin Temperature (°C)");
    ui->tTestGraph->yAxis->setRange(-50.0, 250.0);
    ui->tTestGraph->setBackground(Qt::white);
    //   connect(ui->tTestGraph, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->tTestGraph, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(ui->tTestGraph, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));
    connect(ui->tTestGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->tTestGraph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->tTestGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->tTestGraph->yAxis2, SLOT(setRange(QCPRange)));
}

void MainWindow::setupPGraphs()
{
    // make a time ticker
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");

    // make a vertical Ticker for pressure
    QSharedPointer<QCPAxisTicker> pTicker(new QCPAxisTicker);
    pTicker->setTickCount(10);

    // make pressure plot general settings
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(0)->setPen(QPen(Qt::darkGreen));
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(1)->setPen(QPen(Qt::darkBlue));
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(2)->setPen(QPen(Qt::darkYellow));
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(3)->setPen(QPen(Qt::darkMagenta));
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(4)->setPen(QPen(Qt::darkRed));
    ui->pTestGraph->addGraph();
    ui->pTestGraph->graph(5)->setPen(QPen(Qt::darkCyan));

    ui->pTestGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->pTestGraph->axisRect()->setRangeDrag(Qt::Horizontal);
    ui->pTestGraph->axisRect()->setRangeZoom(Qt::Horizontal);

    // make Y axis pressure ticker
    ui->pTestGraph->yAxis->setTicker(pTicker);

    // make X axis as time ticker
    ui->pTestGraph->xAxis->setTicker(timeTicker);
    ui->pTestGraph->axisRect()->setupFullAxesBox();
    ui->pTestGraph->xAxis->setLabel("Time (hh:mm:ss)");
    ui->pTestGraph->yAxis->setLabel("Pipe Pressure (bar)");
    ui->pTestGraph->yAxis->setRange(0, 10.0);
    ui->pTestGraph->setBackground(Qt::white);
    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->pTestGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->pTestGraph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->pTestGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->pTestGraph->yAxis2, SLOT(setRange(QCPRange)));

}

void MainWindow::setupVGraph()
{
    // make a time ticker
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");

    // make a vertical Ticker for vibration
    QSharedPointer<QCPAxisTicker> vTicker(new QCPAxisTicker);
    vTicker->setTickCount(12);

    // make vibration plot general settings
    //ui->vTestGraph->addGraph();
    //ui->vTestGraph->graph(0)->setPen(QPen(Qt::yellow));
    //ui->vTestGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    //ui->vTestGraph->axisRect()->setRangeDrag(Qt::Horizontal);
    //ui->vTestGraph->axisRect()->setRangeZoom(Qt::Horizontal);

    // make Y axis vibration ticker
    //ui->vTestGraph->yAxis->setTicker(vTicker);

    // make X axis as time ticker
    //ui->vTestGraph->xAxis->setTicker(timeTicker);
    //ui->vTestGraph->axisRect()->setupFullAxesBox();
    //ui->vTestGraph->xAxis->setLabel("Time (hh:mm:ss)");
    //ui->vTestGraph->yAxis->setLabel("Vibration Frequency (Hz)");
    //ui->vTestGraph->yAxis->setRange(0, 60.0);
    //ui->vTestGraph->setBackground(Qt::white);
    // make left and bottom axes transfer their ranges to right and top axes:
    //connect(ui->vTestGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->vTestGraph->xAxis2, SLOT(setRange(QCPRange)));
    //connect(ui->vTestGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->vTestGraph->yAxis2, SLOT(setRange(QCPRange)));

}

void MainWindow::setupPreviewGraphs()
{
    ui->tPreview->addGraph();
    QPen tPen;
    tPen.setWidth(3);
    tPen.setColor(Qt::red);
    tPen.setStyle(Qt::SolidLine);
    ui->tPreview->graph(0)->setPen(tPen);
    ui->tPreview->xAxis2->setVisible(true);
    ui->tPreview->xAxis2->setTickLabels(false);
    ui->tPreview->yAxis2->setVisible(true);
    ui->tPreview->yAxis2->setTickLabels(false);
    ui->tPreview->yAxis->setLabel("Cabin Temperature (°C)");
    ui->tPreview->setBackground(Qt::white);
    ui->tPreview->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->tPreview_2->addGraph();
    QPen tPen_2;
    tPen_2.setWidth(3);
    tPen_2.setColor(Qt::red);
    tPen_2.setStyle(Qt::SolidLine);
    ui->tPreview_2->graph(0)->setPen(tPen);
    ui->tPreview_2->xAxis2->setVisible(true);
    ui->tPreview_2->xAxis2->setTickLabels(false);
    ui->tPreview_2->yAxis2->setVisible(true);
    ui->tPreview_2->yAxis2->setTickLabels(false);
    ui->tPreview_2->yAxis->setLabel("Cabin Temperature (°C)");
    ui->tPreview_2->setBackground(Qt::white);
    ui->tPreview_2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    /*
    ui->pPreview->addGraph();
    QPen pPen;
    pPen.setWidth(3);
    pPen.setColor(Qt::green);
    pPen.setStyle(Qt::SolidLine);
    ui->pPreview->graph(0)->setPen(pPen);
    ui->pPreview->xAxis2->setVisible(true);
    ui->pPreview->xAxis2->setTickLabels(false);
    ui->pPreview->yAxis2->setVisible(true);
    ui->pPreview->yAxis2->setTickLabels(false);
    ui->pPreview->yAxis->setLabel("Pipe Pressure (bar)");
    ui->pPreview->setBackground(Qt::white);
    ui->pPreview->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->vPreview->addGraph();
    QPen vPen;
    vPen.setWidth(3);
    vPen.setColor(Qt::yellow);
    vPen.setStyle(Qt::SolidLine);
    ui->vPreview->graph(0)->setPen(vPen);
    ui->vPreview->xAxis2->setVisible(true);
    ui->vPreview->xAxis2->setTickLabels(false);
    ui->vPreview->yAxis2->setVisible(true);
    ui->vPreview->yAxis2->setTickLabels(false);
    ui->vPreview->yAxis->setLabel("Vibration Frequency (Hz)");
    ui->vPreview->setBackground(Qt::white);
    ui->vPreview->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

*/
}

void MainWindow::setupVisuals()
{

    ui->warningTable->setColumnWidth(0, 140);
    ui->warningTable->setColumnWidth(1, 110);
    ui->warningTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tTestGraph->setVisible(false);
    ui->pTestGraph->setVisible(false);
    //ui->vTestGraph->setVisible(false);

    ui->tWidget->setCurrentIndex(0);
    //   ui->pWidget->setCurrentIndex(0);
    //   ui->vWidget->setCurrentIndex(0);

    ui->wdRepeatEdit->setVisible(false);
    ui->wdLinearEdit->setVisible(false);
    ui->wdTrapEdit->setVisible(false);
    ui->wdSinEdit->setVisible(false);
    ui->wdLogEdit->setVisible(false);

    ui->dsbTStartValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->sbTTotalCycle->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->sbTLDuration->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTLTarget->setButtonSymbols(QAbstractSpinBox::NoButtons);


    /*
    ui->dsbPStartValue->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->dsbPLDuration->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPLTarget->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTRise->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTUp->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTFall->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTDown->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTLow->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPTHigh->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPSPeriod->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPSMean->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPSAmp->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPRepeatValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPRepeatTime->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->dsbVStartValue->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->sbVLDuration->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVLTarget->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVLogRate->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVLogMin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVLogMax->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVRepeatValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbVRepeatTime->setButtonSymbols(QAbstractSpinBox::NoButtons);
*/
    ui->dsbRepeatDurationEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbStartValueEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbLDurationEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbLTargetEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTRiseEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTUpEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTFallEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTDownEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTLowEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTHighEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbSPeriodEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbSMeanEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbSAmpEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbLogRateEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbLogMinEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbLogMaxEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->dsbCabinTopTemp->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPipe1Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe2Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe3Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe4Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe5Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe6Pressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipeVibration->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbTankTemp->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->dsbCabinTopTempMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbPipe1PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe2PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe3PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe4PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe5PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipe6PressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPipeVibrationMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbTankTempMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbVibrationMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPressureMaintenance->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->sbTTotalCycleManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbPTotalCycleManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbVTotalCycleManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbTankTempSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dsbTTimeSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbPTimeSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->dsbVTimeSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->sbTStepSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbPStepSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbVStepSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->sbTStepRepeatSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbPStepRepeatSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbVStepRepeatSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->sbTCycleSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbPCycleSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);
    //ui->sbVCycleSetManual->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui->laTestStartEdit->setText("");

}

void MainWindow::serialMessage(uint command, QByteArray data)
{

    qDebug() <<"RCV: "<<QTime::currentTime().toString("hh.mm.ss.zzz")<< "Command: " << QString::number(command, 16) << " Data: " << QString(data.toHex());

    proc->PLCReply(command);
    switch (command)
    {
    case 0x0A:

        proc->start();

        break;

    case 0x0C:

        updateInfo(1,data);

        break;

    case 0x0D:

        updateInfo(2,data);

        break;

    case 0x0E:

        updateInfo(3,data);

        break;

    case 0x32:
        /*
        pipe1Pressure = quint8(data[0]) / 10.0;
        pipe2Pressure = quint8(data[1]) / 10.0;
        pipe3Pressure = quint8(data[2]) / 10.0;
        pipe4Pressure = quint8(data[3]) / 10.0;
        pipe5Pressure = quint8(data[4]) / 10.0;
        pipe6Pressure = quint8(data[5]) / 10.0;
*/
        pipe1Pressure = qint16(((data[1] & 0xff) << 8) | (data[0] & 0xff)) / 100.0;
        /*      pipe2Pressure = qint16(((data[3] & 0xff) << 8) | (data[2] & 0xff)) / 100.0;
        pipe3Pressure = qint16(((data[5] & 0xff) << 8) | (data[4] & 0xff)) / 100.0;
        pipe4Pressure = qint16(((data[7] & 0xff) << 8) | (data[6] & 0xff)) / 100.0;
        pipe5Pressure = qint16(((data[9] & 0xff) << 8) | (data[8] & 0xff)) / 100.0;
        pipe6Pressure = qint16(((data[11] & 0xff) << 8) | (data[10] & 0xff)) / 100.0;
*/
        ui->dsbPipe1Pressure->setValue(pipe1Pressure);
        ui->dsbPipe1PressureMaintenance->setValue(pipe1Pressure);
        /*     ui->dsbPipe2Pressure->setValue(pipe2Pressure);
        ui->dsbPipe2PressureMaintenance->setValue(pipe2Pressure);
        ui->dsbPipe3Pressure->setValue(pipe3Pressure);
        ui->dsbPipe3PressureMaintenance->setValue(pipe3Pressure);
        ui->dsbPipe4Pressure->setValue(pipe4Pressure);
        ui->dsbPipe4PressureMaintenance->setValue(pipe4Pressure);
        ui->dsbPipe5Pressure->setValue(pipe5Pressure);
        ui->dsbPipe5PressureMaintenance->setValue(pipe5Pressure);
        ui->dsbPipe6Pressure->setValue(pipe6Pressure);
        ui->dsbPipe6PressureMaintenance->setValue(pipe6Pressure);
*/        break;

    case 0x33:

        //       waterTankLiquidLevel = quint8(data[0]);
        //       waterTankTemperature = qint16(((data[2] & 0xff) << 8) | (data[1] & 0xff)) / 10.0;
        cabinTopTemperature = qint16(((data[1] & 0xff) << 8) | (data[0] & 0xff)) / 10.0;
        cabinBottomTemperature = qint16(((data[3] & 0xff) << 8) | (data[2] & 0xff)) / 10.0;
        //       pipeVibrationFrequency = quint16(((data[8] & 0xff) << 8) | (data[7] & 0xff)) / 10.0;

        //ui->dsbTankTemp->setValue(waterTankTemperature);
        //ui->dsbTankTempMaintenance->setValue(waterTankTemperature);
        ui->dsbCabinTopTemp->setValue(cabinTopTemperature);
        ui->dsbCabinTopTempMaintenance->setValue(cabinTopTemperature);
        //ui->dsbPipeVibration->setValue(pipeVibrationFrequency);
        //ui->dsbPipeVibrationMaintenance->setValue(pipeVibrationFrequency);



        break;

    }
}

void MainWindow::prepareTestTimers()
{
    tempPeriod = 10000;
    vibPeriod = 1000;
    pressurePeriod = 250;

    if (myPLC.temperatureTestActive)
    {
        timerTemp->start(tempPeriod);
    }

    if (myPLC.temperatureTestActive)
    {
        timerPressure->start(pressurePeriod);
    }
    /*
    if (myPLC.vibrationTestActive)
    {
       timerVib->start(vibPeriod);
    }
*/
}

void MainWindow::updateInfo(quint8 index, QByteArray data)
{
    if (index == 3)
    {
        if (data[0] == char(0x01))
        {
            if (myPLC.deviceState == (data[0]))
            {

            }
            else
            {
                myPLC.deviceState = (data[0]);

                if (myPLC.deviceState)
                {
                    timerTemp->stop();
                    /*    timerVib->stop(); */
                    timerPressure->stop();

                    writeToLogTable("Device idle.");

                    ui->cbSelectProfileMain->setCurrentIndex(0);
                    ui->cbSelectProfileMain->setEnabled(true);

                    ui->bStartTest->setEnabled(true);
                    ui->bStopTest->setEnabled(false);
                    ui->bPauseTest->setEnabled(false);

                    ui->sbTTotalCycle->setEnabled(true);




                    ui->cbSelectProfileManual->setCurrentIndex(0);
                    ui->cbSelectProfileManual->setEnabled(true);

                    ui->bStartTestManual->setEnabled(true);
                    ui->bStopTestManual->setEnabled(false);
                    ui->bPauseTestManual->setEnabled(false);

                    ui->sbTTotalCycleManual->setEnabled(true);
                    //     ui->sbPTotalCycleManual->setEnabled(true);
                    //     ui->sbVTotalCycleManual->setEnabled(true);
                    //     ui->dsbTankTempSetManual->setEnabled(true);
                    //     ui->chbEllipticalVibrationSetManual->setEnabled(true);
                }
            }
        }
        else if (data[0] == char(0x02))
        {
            if (myPLC.deviceState == (data[0]))
            {

            }
            else
            {
                myPLC.deviceState = (data[0]);

                if (myPLC.deviceState)
                {
                    testFolder = ui->laSelectedProfileMain->text() + "-" +
                            QDate::currentDate().toString("yy.MM.dd") + "-" +
                            QTime::currentTime().toString("hh.mm.ss");
#ifdef Q_OS_LINUX
                    //linux code goes here
                    if (!QDir("/home/pi/InDetail/records/" + testFolder).exists())
                    {
                        QDir().mkdir("/home/pi/InDetail/records/" + testFolder);
                    }
#endif
#ifdef Q_OS_WIN
                    // windows code goes here
                    if (!QDir("records\\" + testFolder).exists())
                    {
                        QDir().mkdir("records\\" + testFolder);
                    }
#endif

                    writeToLogTable("Test started.");

                    ui->cbSelectProfileMain->setEnabled(false);
                    ui->bStartTest->setEnabled(false);
                    ui->bStopTest->setEnabled(true);
                    ui->bPauseTest->setEnabled(true);

                    ui->sbTTotalCycle->setEnabled(false);


                    ui->cbSelectProfileManual->setEnabled(false);
                    ui->bStartTestManual->setEnabled(false);
                    ui->bStopTestManual->setEnabled(true);
                    ui->bPauseTestManual->setEnabled(true);

                    ui->sbTTotalCycleManual->setEnabled(false);
                    //    ui->sbPTotalCycleManual->setEnabled(false);
                    //    ui->sbVTotalCycleManual->setEnabled(false);
                    //    ui->dsbTankTempSetManual->setEnabled(false);
                    //    ui->chbEllipticalVibrationSetManual->setEnabled(false);
                }
            }
        }
        else if (data[0] == char(0x03))
        {
            if (myPLC.deviceState == (data[0]))
            {

            }
            else
            {
                myPLC.deviceState = data[0];
                if (myPLC.deviceState)
                {
                    timerTemp->stop();
                    /*  timerVib->stop();*/
                    timerPressure->stop();

                    writeToLogTable("Test paused.");

                    ui->bPauseTestManual->setEnabled(false);
                    ui->bPauseTest->setEnabled(false);

                    ui->bStartTest->setEnabled(true);
                    ui->bStartTestManual->setEnabled(true);


                }
            }
        }
        else if (data[0] == char(0x04))
        {
            if (myPLC.deviceState == (data[0]))
            {

            }
            else
            {
                myPLC.deviceState = data[0];
                if (myPLC.deviceState)
                {
                    writeToLogTable("Maintenance mode activated.");
                }
            }
        }

        if (myPLC.pressurePrepActive == (data[1] & 0b00001000) >> 3)
        {

        }
        else
        {
            myPLC.pressurePrepActive = (data[1] & 0b00001000) >> 3;

            if (myPLC.pressurePrepActive)
            {
                writeToLogTable("Pressure prep. started.");
            }
            else
            {
                writeToLogTable("Pressure prep. completed.");
            }
        }

        if (myPLC.pressureTestCompleted == (data[1] & 0b00100000) >> 5)
        {

        }
        else
        {
            myPLC.pressureTestCompleted = (data[1] & 0b00100000) >> 5;

            if (myPLC.pressureTestCompleted)
            {
                writeToLogTable("Pressure test completed.");
            }
            else
            {

            }
        }

        if (myPLC.pressureTestActive == (data[1] & 0b00010000) >> 4)
        {

        }
        else
        {
            myPLC.pressureTestActive = (data[1] & 0b00010000) >> 4;

            if (myPLC.pressureTestActive)
            {
                writeToLogTable("Pressure test started.");
            }
            else
            {
                if (myPLC.pressureTestCompleted)
                {

                }
                else
                {
                    writeToLogTable("Pressure test canceled.");
                }
            }
        }

        if (myPLC.vibrationPrepActive == (data[1] & 0b01000000) >> 6)
        {

        }
        else
        {
            myPLC.vibrationPrepActive = (data[1] & 0b01000000) >> 6;

            if (myPLC.vibrationPrepActive)
            {
                writeToLogTable("Vibration prep. started.");
            }
            else
            {
                writeToLogTable("Vibration prep. completed.");
            }
        }

        if (myPLC.vibrationTestCompleted == (data[2] & 0b00000001))
        {

        }
        else
        {
            myPLC.vibrationTestCompleted = (data[2] & 0b00000001);

            if (myPLC.vibrationTestCompleted)
            {
                writeToLogTable("Vibration test completed.");
            }
            else
            {

            }
        }

        if (myPLC.vibrationTestActive == (data[1] & 0b10000000) >> 7)
        {

        }
        else
        {
            myPLC.vibrationTestActive = (data[1] & 0b10000000) >> 7;

            if (myPLC.vibrationTestActive)
            {
                writeToLogTable("Vibration test started.");
            }
            else
            {
                if (myPLC.vibrationTestCompleted)
                {

                }
                else
                {
                    writeToLogTable("Vibration test canceled.");
                }
            }
        }

        if (myPLC.temperaturePrepActive == (data[1] & 0b00000001))
        {

        }
        else
        {
            myPLC.temperaturePrepActive = (data[1] & 0b00000001);

            if (myPLC.temperaturePrepActive)
            {
                writeToLogTable("Temp. prep. started.");
            }
            else
            {
                writeToLogTable("Temp. prep. completed.");
            }
        }

        if (myPLC.temperatureTestCompleted == (data[1] & 0b00000100) >> 2)
        {

        }
        else
        {
            myPLC.temperatureTestCompleted = (data[1] & 0b00000100) >> 2;

            if (myPLC.temperatureTestCompleted)
            {
                writeToLogTable("Temp. test completed.");
            }
            else
            {

            }
        }

        if (myPLC.temperatureTestActive == (data[1] & 0b00000010) >> 1)
        {

        }
        else
        {
            myPLC.temperatureTestActive = (data[1] & 0b00000010) >> 1;

            if (myPLC.temperatureTestActive)
            {
                writeToLogTable("Temp. test started.");
            }
            else
            {
                if (myPLC.temperatureTestCompleted)
                {

                }
                else
                {
                    writeToLogTable("Temp. test canceled.");
                }
            }
        }

        if (myPLC.profileActive == (data[2] & 0b00000010) >> 1)
        {

        }
        else
        {
            myPLC.profileActive = (data[2] & 0b00000010) >> 1;

            if (myPLC.profileActive)
            {
                writeToLogTable("Profile started.");
                prepareTestTimers();
            }
            else
            {

            }
        }

        if (myPLC.cabinTemperatureStat == (data[2] & 0b00000100) >> 2)
        {

        }
        else
        {
            myPLC.cabinTemperatureStat = (data[2] & 0b00000100) >> 2;


        }

        if (myPLC.tankTemperatureStat == (data[2] & 0b00001000) >> 3)
        {

        }
        else
        {
            myPLC.tankTemperatureStat = (data[2] & 0b00001000) >> 3;


        }

        if (myPLC.pipePressureStat == (data[2] & 0b00010000) >> 4)
        {

        }
        else
        {
            myPLC.pipePressureStat = (data[2] & 0b00010000) >> 4;


        }

        if (myPLC.vibrationMotor1Stat == (data[2] & 0b00100000) >> 5)
        {

        }
        else
        {
            myPLC.vibrationMotor1Stat = (data[2] & 0b00100000) >> 5;


        }

        if (myPLC.vibrationMotor2Stat == (data[2] & 0b01000000) >> 6)
        {

        }
        else
        {

        }

        if (tCycle != quint16(((data[4] & 0xff) << 8) | (data[3] & 0xff)))
        {
            tCycle = quint16(((data[4] & 0xff) << 8) | (data[3] & 0xff));
            //          ui->laCurrentTCycleMain->setText(QString::number(tCycle));
            if (tCycle != 0)
            {
                tKey = 0;
                ui->tTestGraph->clearPlottables();
                setupTGraph();
            }
        }
        if ( pCycle != quint16(((data[6] & 0xff) << 8) | (data[5] & 0xff)) )
        {

            pCycle = quint16(((data[6] & 0xff) << 8) | (data[5] & 0xff));

            if (pCycle != 0)
            {
                pKey = 0;
                ui->pTestGraph->clearPlottables();
                setupPGraphs();
            }
        }
        if ( vCycle != quint16(((data[8] & 0xff) << 8) | (data[7] & 0xff)) )
        {
            vCycle = quint16(((data[8] & 0xff) << 8) | (data[7] & 0xff));

            if (vCycle != 0)
            {
                vKey = 0;
                //ui->vTestGraph->clearPlottables();
                //setupVGraph();
            }
        }

        if ( myPLC.deviceState == char(0x02) )
        {
#ifdef Q_OS_LINUX
            //linux code goes here
            QString filePath = "/home/pi/InDetail/records/" + testFolder + "/" + "testProgress.txt";
#endif
#ifdef Q_OS_WIN
            // windows code goes here
            QString filePath = "records\\" + testFolder + "\\" + "testProgress.txt";
#endif

            QFile file(filePath);

            if (file.open(QFile::WriteOnly|QFile::Truncate))
            {
                QTextStream stream(&file);
                stream << "Temperature Elapsed Seconds: " << tElapsedSeconds << "\n"
                       << "Pressure Elapsed Seconds: " << pElapsedSeconds << "\n"
                       << "Vibration Elapsed Seconds: " << vElapsedSeconds << "\n"
                       << "Temperature Step: " << tStep << "\n"
                       << "Pressure Step: " << pStep << "\t" << "Pressure Step Repeat: " << pStepRepeat << "\n"
                       << "Vibration Step: " << vStep << "\t" << "Vibration Step Repeat: " << vStepRepeat << "\n"
                       << "Temperature Cycle: " << tCycle << "\n"
                       << "Pressure Cycle: " << pCycle << "\n"
                       << "Vibration Cycle: " << vCycle << "\n";
                file.close();
            }
        }
    }
    else if (index == 2)
    {
        tStep = data[0];
        pStep = data[1];
        vStep = data[2];
        pStepRepeat = (data[3] & 0xFF) | ((data[4] & 0xFF) <<  8) |
                ((data[5] & 0xFF) << 16);
        vStepRepeat = (data[6] & 0xFF) | ((data[7] & 0xFF) <<  8) |
                ((data[8] & 0xFF) << 16);

        //   ui->laCurrentTStepMain->setText(QString::number(tStep));

        ui->laTCycleCounterDetails->setText(QString::number(tCycle));
        ui->laTStepCounterDetails->setText(QString::number(tStep));
        ui->laPCycleCounterDetails->setText(QString::number(pCycle));
        ui->laPStepCounterDetails->setText(QString::number(pStep));
        ui->laPRepeatCounterDetails->setText(QString::number(pStepRepeat));
        ui->laVCycleCounterDetails->setText(QString::number(vCycle));
        ui->laVStepCounterDetails->setText(QString::number(vStep));
        ui->laVRepeatCounterDetails->setText(QString::number(vStepRepeat));
    }
    else if (index == 1)
    {
        tElapsedSeconds = (data[0] & 0xFF) | ((data[1] & 0xFF) <<  8) |
                ((data[2] & 0xFF) << 16);
        pElapsedSeconds = (data[3] & 0xFF) | ((data[4] & 0xFF) <<  8) |
                ((data[5] & 0xFF) << 16);
        vElapsedSeconds = (data[6] & 0xFF) | ((data[7] & 0xFF) <<  8) |
                ((data[8] & 0xFF) << 16);
        ui->laTTestElepsedSecond->setText(QString::number(tElapsedSeconds));
    }
}

void MainWindow::writeToLogTable(QString info)
{
    ui->warningTable->insertRow(0);
    ui->warningTable->setItem(0, 0, new QTableWidgetItem(info));
    ui->warningTable->setItem(0, 1, new QTableWidgetItem(
                                  QDate::currentDate().toString(Qt::SystemLocaleShortDate) + "/" +
                                  QTime::currentTime().toString("hh.mm.ss")));
    if ( myPLC.deviceState == char(0x02) )
    {
#ifdef Q_OS_LINUX
        //linux code goes here
        QString filePath = "/home/pi/InDetail/records/" + testFolder + "/" + "testLogs.txt";
#endif
#ifdef Q_OS_WIN
        // windows code goes here
        QString filePath = "records\\" + testFolder + "\\" + "testLogs.txt";
#endif

        QFile file(filePath);

        if (file.open(QFile::WriteOnly|QFile::Append))
        {
            QTextStream stream(&file);
            stream << "Date: " << QDate::currentDate().toString(Qt::SystemLocaleShortDate)
                   << "\t" << "Time: " << QTime::currentTime().toString("hh.mm.ss")
                   << "\t" << "Info: " << info << "\n";
            file.close();
        }
    }
}

void MainWindow::on_cbSelectGraph_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->tTestGraph->setVisible(false);
        ui->pTestGraph->setVisible(false);
        //ui->vTestGraph->setVisible(false);
        ui->laCycleCounterDetails->setVisible(false);
        ui->laStepCounterDetails->setVisible(false);
        ui->laRepeatCounterDetails->setVisible(false);
        ui->laTCycleCounterDetails->setVisible(false);
        ui->laTStepCounterDetails->setVisible(false);
        ui->laPCycleCounterDetails->setVisible(false);
        ui->laPStepCounterDetails->setVisible(false);
        ui->laPRepeatCounterDetails->setVisible(false);
        ui->laVCycleCounterDetails->setVisible(false);
        ui->laVStepCounterDetails->setVisible(false);
        ui->laVRepeatCounterDetails->setVisible(false);
    }
    else if (index == 1)
    {
        ui->tTestGraph->setVisible(false);
        ui->pTestGraph->setVisible(false);
        //ui->vTestGraph->setVisible(false);
        ui->tTestGraph->setVisible(true);
        ui->laTCycleCounterDetails->setVisible(true);
        ui->laTStepCounterDetails->setVisible(true);
        ui->laPCycleCounterDetails->setVisible(false);
        ui->laPStepCounterDetails->setVisible(false);
        ui->laPRepeatCounterDetails->setVisible(false);
        ui->laVCycleCounterDetails->setVisible(false);
        ui->laVStepCounterDetails->setVisible(false);
        ui->laVRepeatCounterDetails->setVisible(false);

    }
    else if (index == 2)
    {
        ui->pTestGraph->setVisible(false);
        ui->tTestGraph->setVisible(false);
        //ui->vTestGraph->setVisible(false);
        ui->pTestGraph->setVisible(true);
        ui->laTCycleCounterDetails->setVisible(false);
        ui->laTStepCounterDetails->setVisible(false);
        ui->laPCycleCounterDetails->setVisible(true);
        ui->laPStepCounterDetails->setVisible(true);
        ui->laPRepeatCounterDetails->setVisible(true);
        ui->laVCycleCounterDetails->setVisible(false);
        ui->laVStepCounterDetails->setVisible(false);
        ui->laVRepeatCounterDetails->setVisible(false);

    }
    else if (index == 3)
    {
        //ui->vTestGraph->setVisible(false);
        ui->tTestGraph->setVisible(false);
        ui->pTestGraph->setVisible(false);
        //ui->vTestGraph->setVisible(true);
        ui->laTCycleCounterDetails->setVisible(false);
        ui->laTStepCounterDetails->setVisible(false);
        ui->laPCycleCounterDetails->setVisible(false);
        ui->laPStepCounterDetails->setVisible(false);
        ui->laPRepeatCounterDetails->setVisible(false);
        ui->laVCycleCounterDetails->setVisible(true);
        ui->laVStepCounterDetails->setVisible(true);
        ui->laVRepeatCounterDetails->setVisible(true);

    }
}

void MainWindow::on_bEditPro_clicked()
{
    clearProfileSlot('s', 't', currentProfile);
    clearProfileSlot('s', 'p', currentProfile);
    clearProfileSlot('s', 'v', currentProfile);

    currentTStep = 0;
    currentPStep = 0;
    currentVStep = 0;

    ui->tabWidget->setTabEnabled(0, false);
    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(2, false);
    ui->tabWidget->setTabEnabled(4, false);
    ui->tabWidget->setTabEnabled(5, false);

    ui->cbSelectProfile->setEnabled(false);

    ui->bSavePro->setEnabled(true);
    ui->bClearPro->setEnabled(true);
    ui->bEditPro->setEnabled(false);

    ui->bNewTStep->setEnabled(true);
    //  ui->bNewPStep->setEnabled(true);
    //  ui->bNewVStep->setEnabled(true);

    ui->cbTSelectSUnit->setEnabled(true);
    //  ui->cbPSelectSUnit->setEnabled(true);
    // ui->cbVSelectSUnit->setEnabled(true);
    //  ui->cbTSelectSType->setEnabled(true);
    //  ui->cbPSelectSType->setEnabled(true);
    //  ui->cbVSelectSType->setEnabled(true);

    ui->dsbTStartValue->setEnabled(true);
    //  ui->dsbPStartValue->setEnabled(true);
    //  ui->dsbVStartValue->setEnabled(true);

    ui->leProfileName->setEnabled(true);
}

void MainWindow::on_bClearPro_clicked()
{
    clearProfileSlot('s', 't', currentProfile);
    clearProfileSlot('s', 'p', currentProfile);
    clearProfileSlot('s', 'v', currentProfile);

    ui->cbSelectProfile->setCurrentIndex(0);
    ui->cbSelectProfile->setEnabled(true);

    ui->leProfileName->setText("");
    ui->leProfileName->setEnabled(false);

    ui->dsbTStartValue->setValue(0);
    ui->dsbTStartValue->setEnabled(false);
    //  ui->dsbPStartValue->setValue(0);
    //  ui->dsbPStartValue->setEnabled(false);
    //  ui->dsbVStartValue->setValue(0);
    //  ui->dsbVStartValue->setEnabled(false);

    ui->laTTotalStep->setText("0");
    //  ui->laPTotalStep->setText("0");
    //  ui->laVTotalStep->setText("0");

    ui->tWidget->setCurrentIndex(0);
    //  ui->pWidget->setCurrentIndex(0);
    //  ui->vWidget->setCurrentIndex(0);

    ui->cbTSelectSUnit->setEnabled(false);
    //  ui->cbPSelectSUnit->setEnabled(false);
    //  ui->cbVSelectSUnit->setEnabled(false);
    //  ui->cbTSelectSType->setEnabled(false);
    //  ui->cbPSelectSType->setEnabled(false);
    //  ui->cbVSelectSType->setEnabled(false);

    ui->tPreview->clearPlottables();
    //  ui->pPreview->clearPlottables();
    //  ui->vPreview->clearPlottables();

    currentTStep = 0;
    currentPStep = 0;
    currentVStep = 0;

    ui->tabWidget->setTabEnabled(0, true);
    ui->tabWidget->setTabEnabled(1, true);
    ui->tabWidget->setTabEnabled(2, true);
    ui->tabWidget->setTabEnabled(4, true);
    ui->tabWidget->setTabEnabled(5, true);

    ui->tPreview_2->clearPlottables();
    ui->tPreview_2->replot();

}

void MainWindow::on_cbSelectProfile_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->bEditPro->setEnabled(false);
        ui->bSavePro->setEnabled(false);
        ui->bClearPro->setEnabled(false);
        ui->bNewTStep->setEnabled(false);
        //      ui->bNewPStep->setEnabled(false);
        //      ui->bNewVStep->setEnabled(false);
    }
    else
    {
        ui->bEditPro->setEnabled(true);
        currentProfile = index - 1;
    }
}

void MainWindow::on_bNewTStep_clicked()
{
    if (tProfileSave[currentProfile].totalStep == 0)
    {
        oldTValue = ui->dsbTStartValue->value();
        ui->laOldTValue->setText(QString::number(oldTValue));
    }

    if (  ( ui->cbTSelectSUnit->currentIndex() == 0 ) )
    {
        //nothing selected for neither step type nor step time unit. Going forward should be banned.
    }
    else
    {
        if ( ui->cbTSelectSUnit->currentIndex() == 1 )
        {
            //second selected for step time unit, do what you gotta do.

            tProfileSave[currentProfile].active = 1;
            tProfileSave[currentProfile].step[currentTStep].stepUnit = 1;
            tProfileSave[currentProfile].startValue = ui->dsbTStartValue->value();

            ui->laTLinDurationSave->setText("s.");
        }
        else if ( ui->cbTSelectSUnit->currentIndex() == 2 )
        {
            //minute selected for step time unit, do what you gotta do.

            tProfileSave[currentProfile].active = 1;
            tProfileSave[currentProfile].step[currentTStep].stepUnit = 2;
            tProfileSave[currentProfile].startValue = ui->dsbTStartValue->value();

            ui->laTLinDurationSave->setText("m.");
        }
        else if ( ui->cbTSelectSUnit->currentIndex() == 3 )
        {
            //hour selected for step time unit, do what you gotta do.

            tProfileSave[currentProfile].active = 1;
            tProfileSave[currentProfile].step[currentTStep].stepUnit = 3;
            tProfileSave[currentProfile].startValue = ui->dsbTStartValue->value();

            ui->laTLinDurationSave->setText("h.");
        }
        else if ( ui->cbTSelectSUnit->currentIndex() == 4 )
        {
            //day selected for step time unit, do what you gotta do.

            tProfileSave[currentProfile].active = 1;
            tProfileSave[currentProfile].step[currentTStep].stepUnit = 4;
            tProfileSave[currentProfile].startValue = ui->dsbTStartValue->value();

            ui->laTLinDurationSave->setText("d.");
        }
        tProfileSave[currentProfile].step[currentTStep].stepType = 1;
        ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() + 1);
    }
}

void MainWindow::on_bTForward2_clicked()
{
    float delta;

    tProfileSave[currentProfile].step[currentTStep].lDuration = ui->sbTLDuration->value();
    tProfileSave[currentProfile].step[currentTStep].lTarget = ui->dsbTLTarget->value();

    float startValue = tProfileSave[currentProfile].startValue;
    float duration = tProfileSave[currentProfile].step[currentTStep].lDuration;
    float oldTarget = tProfileSave[currentProfile].step[currentTStep-1].lTarget;
    float target = tProfileSave[currentProfile].step[currentTStep].lTarget;

    if ( currentTStep == 0 )
    {
        delta = qFabs( target - startValue );
    }
    else
    {
        delta = qFabs( target - oldTarget );
    }

    if ( ui->cbTSelectSUnit->currentIndex() == 1 )
    {
        if ( ( delta * 60 / 5 ) <= ( duration * 1 ) )
        {
            ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() + 1);
            updateTPreview();
        }
    }
    else if ( ui->cbTSelectSUnit->currentIndex() == 2 )
    {
        if ( ( delta * 60 / 5 ) <= ( duration * 60 ) )
        {
            ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() + 1);
            updateTPreview();
        }
    }
    else if ( ui->cbTSelectSUnit->currentIndex() == 3 )
    {
        if ( ( delta * 60 / 5 ) <= ( duration * 60 * 60 ) )
        {
            ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() + 1);
            updateTPreview();
        }
    }
    else if ( ui->cbTSelectSUnit->currentIndex() == 4 )
    {
        if ( ( delta * 60 / 5 ) <= ( duration * 60 * 60 * 24 ) )
        {
            ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() + 1);
            updateTPreview();
        }
    }
}

void MainWindow::on_bTBack2_clicked()
{
    ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() - 1);
}

void MainWindow::on_bTBack3_clicked()
{
    ui->tWidget->setCurrentIndex(ui->tWidget->currentIndex() - 1);
}

void MainWindow::on_bTSaveStep_clicked()
{
    ui->dsbTStartValue->setEnabled(false);

    tProfileSave[currentProfile].totalStep++;
    currentTStep++;
    ui->laTTotalStep->setText(QString::number(currentTStep));

    oldTValue = tProfileSave[currentProfile].step[currentTStep - 1].lTarget;
    ui->laOldTValue->setText(QString::number(oldTValue));

    ui->sbTLDuration->setValue(1);
    ui->dsbTLTarget->setValue(0);

    //ui->cbTSelectSType->setCurrentIndex(0);
    ui->cbTSelectSUnit->setCurrentIndex(0);

    ui->tWidget->setCurrentIndex(0);
}

void MainWindow::updateTPreview()
{
    //ui->tPreview->clearPlottables();
    //ui->tPreview->xAxis->setRange(0, ui->dsbTLTarget->value());
    //ui->tPreview->yAxis->setRange(-40, 250);

    if (tProfileSave[currentProfile].step[currentTStep].stepUnit == 1)
    {
        ui->tPreview->xAxis->setLabel("Time (seconds)");
        ui->tPreview_2->xAxis->setLabel("Time (seconds)");
    }
    else if (tProfileSave[currentProfile].step[currentTStep].stepUnit == 2)
    {
        ui->tPreview->xAxis->setLabel("Time (minutes)");
        ui->tPreview_2->xAxis->setLabel("Time (minutes)");
    }
    else if (tProfileSave[currentProfile].step[currentTStep].stepUnit == 3)
    {
        ui->tPreview->xAxis->setLabel("Time (hours)");
        ui->tPreview_2->xAxis->setLabel("Time (hours)");
    }
    else if (tProfileSave[currentProfile].step[currentTStep].stepUnit == 4)
    {
        ui->tPreview->xAxis->setLabel("Time (days)");
        ui->tPreview_2->xAxis->setLabel("Time (days)");
    }


    float delta;
    float duration = tProfileSave[currentProfile].step[currentTStep].lDuration;
    float startValue = tProfileSave[currentProfile].startValue;
    float oldTarget = tProfileSave[currentProfile].step[currentTStep-1].lTarget;
    float target = tProfileSave[currentProfile].step[currentTStep].lTarget;
    float oldDuration= tProfileSave[currentProfile].step[currentTStep-1].lDuration;

    QVector<double> tPreviewX( ( duration * 10 ) + 1 ), tPreviewY( ( duration * 10 ) + 1 );
    QVector<double> tPreviewX_2(21), tPreviewY_2( 21 );

    if (currentTStep == 0)
    {
        tPreviewY[0] = startValue;
        tPreviewX[0] = 0;
        delta = target - startValue;
        tPreviewY_2[1] = target;
        tPreviewX_2[1] = duration;
        tPreviewY_2[0] = startValue;
        tPreviewX_2[0] = 0.1;
    }
    else
    {
        tPreviewY[0] = oldTarget;
        tPreviewX[0] = 0;
        delta = target - oldTarget;
        if (currentTStep == 1)
        {
            tPreviewY_2[0] = startValue;
            tPreviewX_2[0] = 1;
            tPreviewY_2[1] = oldTarget;
            tPreviewX_2[1] = oldDuration;
            tPreviewY_2[2] = target;
            tPreviewX_2[2] = tPreviewX_2[1] + duration;
        }
        else if (currentTStep == 2)
        {
            tPreviewY_2[0] = startValue;
            tPreviewX_2[0] = 1;
            tPreviewY_2[1] = tProfileSave[currentProfile].step[currentTStep-2].lTarget;
            tPreviewX_2[1] = tProfileSave[currentProfile].step[currentTStep-2].lDuration;
            tPreviewY_2[2] = tProfileSave[currentProfile].step[currentTStep-1].lTarget;
            tPreviewX_2[2] = tPreviewX_2[1] + tProfileSave[currentProfile].step[currentTStep-1].lDuration;
            tPreviewY_2[3] = target;
            tPreviewX_2[3] = tPreviewX_2[2] + duration ;
        }
        else if (currentTStep == 3)
        {
            tPreviewY_2[0] = startValue;
            tPreviewX_2[0] = 1;
            tPreviewY_2[1] = tProfileSave[currentProfile].step[currentTStep-3].lTarget;
            tPreviewX_2[1] = tProfileSave[currentProfile].step[currentTStep-3].lDuration;
            tPreviewY_2[2] = tProfileSave[currentProfile].step[currentTStep-2].lTarget;
            tPreviewX_2[2] = tPreviewX_2[currentTStep - 2] + tProfileSave[currentProfile].step[currentTStep-2].lDuration;
            tPreviewY_2[3] = tProfileSave[currentProfile].step[currentTStep-1].lTarget;
            tPreviewX_2[3] = tPreviewX_2[currentTStep - 1] + tProfileSave[currentProfile].step[currentTStep-1].lDuration;
            tPreviewY_2[4] = target;
            tPreviewX_2[4] = tPreviewX_2[currentTStep] + duration ;
        }
        else if (currentTStep >= 4)
        {
            tPreviewY_2[0] = startValue;
            tPreviewX_2[0] = 1;
            for(int j=1; j < (currentTStep + 1 ); j++)
            {
                tPreviewY_2[j] = tProfileSave[currentProfile].step[j-1].lTarget;
                tPreviewX_2[j] = tPreviewX_2[j-1] + tProfileSave[currentProfile].step[j-1].lDuration;
            }
            tPreviewY_2[currentTStep + 1] = target;
            tPreviewX_2[currentTStep + 1] = tPreviewX_2[currentTStep] + duration ;
        }
    }

    for(int i=1; i < ( duration * 10) + 1; i++)
    {
        tPreviewY[i] = tPreviewY[i-1] + ( delta / ( duration * 10 ) );
        tPreviewX[i] = float(i) / 10;
    }

    ui->tPreview->graph(0)->setData(tPreviewX, tPreviewY);
    ui->tPreview->graph(0)->rescaleAxes();
    ui->tPreview->replot();

    connect(ui->tPreview_2->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->tPreview_2->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->tPreview_2->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->tPreview_2->yAxis2, SLOT(setRange(QCPRange)));
    ui->tPreview_2->graph(0)->setData(tPreviewX_2, tPreviewY_2);
    ui->tPreview_2->graph(0)->rescaleAxes();
    ui->tPreview_2->replot();

}

/*
void MainWindow::on_bNewVStep_clicked()
{
    ui->dsbVRepeatValue->setValue(0);
    ui->bVForward4->setEnabled(false);

    if (vProfileSave[currentProfile].totalStep == 0)
    {
        oldVValue = ui->dsbVStartValue->value();
        ui->laOldVValue->setText(QString::number(oldVValue));
    }

    if ( ( ui->cbVSelectSType->currentIndex() == 0 ) || ( ui->cbVSelectSUnit->currentIndex() == 0 ) )
    {
        //nothing selected for neither step type nor step time unit. Going forward should be banned.
    }
    else
    {
        if ( ui->cbVSelectSUnit->currentIndex() == 1 )
        {
            //second selected for step time unit, do what you gotta do.

            vProfileSave[currentProfile].active = 1;
            vProfileSave[currentProfile].step[currentVStep].stepUnit = 1;
            vProfileSave[currentProfile].startValue = ui->dsbVStartValue->value();

            ui->laVLinDurationSave->setText("s.");
        }
        else if ( ui->cbVSelectSUnit->currentIndex() == 2 )
        {
            //minute selected for step time unit, do what you gotta do.

            vProfileSave[currentProfile].active = 1;
            vProfileSave[currentProfile].step[currentVStep].stepUnit = 2;
            vProfileSave[currentProfile].startValue = ui->dsbVStartValue->value();

            ui->laVLinDurationSave->setText("m.");
        }
        else if ( ui->cbVSelectSUnit->currentIndex() == 3 )
        {
            //hour selected for step time unit, do what you gotta do.

            vProfileSave[currentProfile].active = 1;
            vProfileSave[currentProfile].step[currentVStep].stepUnit = 3;
            vProfileSave[currentProfile].startValue = ui->dsbVStartValue->value();

            ui->laVLinDurationSave->setText("h.");
        }
        else if ( ui->cbVSelectSUnit->currentIndex() == 4 )
        {
            //day selected for step time unit, do what you gotta do.

            vProfileSave[currentProfile].active = 1;
            vProfileSave[currentProfile].step[currentVStep].stepUnit = 4;
            vProfileSave[currentProfile].startValue = ui->dsbVStartValue->value();

            ui->laVLinDurationSave->setText("d.");
        }

        if ( ui->cbVSelectSType->currentIndex() == 1 )
        {
            //linear step type selected do what you gotta do.

            vProfileSave[currentProfile].step[currentVStep].stepType = 1;
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 1);
        }
        else if ( ui->cbVSelectSType->currentIndex() == 2 )
        {
            //logarithmic sweep step type selected do what you gotta do.

            vProfileSave[currentProfile].step[currentVStep].stepType = 4;
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 2);
        }

    }
}
*/
/*
void MainWindow::on_bNewPStep_clicked()
{
    ui->dsbPRepeatValue->setValue(0);
    ui->bPForward5->setEnabled(false);

    if (pProfileSave[currentProfile].totalStep == 0)
    {
        oldPValue = ui->dsbPStartValue->value();
        ui->laOldPValue->setText(QString::number(oldPValue));
    }

    if ( ( ui->cbPSelectSType->currentIndex() == 0 ) || ( ui->cbPSelectSUnit->currentIndex() == 0 ) )
    {
        //nothing selected for neither step type nor step time unit. Going forward should be banned.
    }
    else
    {
        if ( ui->cbPSelectSUnit->currentIndex() == 1 )
        {
            //second selected for step time unit, do what you gotta do.

            pProfileSave[currentProfile].active = 1;
            pProfileSave[currentProfile].step[currentPStep].stepUnit = 1;
            pProfileSave[currentProfile].startValue = ui->dsbPStartValue->value();

            ui->cbPStepRepeatUnit->clear();
            ui->cbPStepRepeatUnit->addItem("Select a step repeat time unit...");
            ui->cbPStepRepeatUnit->addItem("Seconds");
            ui->cbPStepRepeatUnit->addItem("Minutes");
            ui->cbPStepRepeatUnit->addItem("Hours");
            ui->cbPStepRepeatUnit->addItem("Days");
            ui->cbPStepRepeatUnit->addItem("Number of Times");

            ui->laPLinDurationSave->setText("s.");
            ui->laPTRiseSave->setText("s.");
            ui->laPTUpSave->setText("s.");
            ui->laPTFallSave->setText("s.");
            ui->laPTDownSave->setText("s.");
            ui->laPSPeriodSave->setText("s.");
        }
        else if ( ui->cbPSelectSUnit->currentIndex() == 2 )
        {
            //minute selected for step time unit, do what you gotta do.

            pProfileSave[currentProfile].active = 1;
            pProfileSave[currentProfile].step[currentPStep].stepUnit = 2;
            pProfileSave[currentProfile].startValue = ui->dsbPStartValue->value();

            ui->cbPStepRepeatUnit->clear();
            ui->cbPStepRepeatUnit->addItem("Select a step repeat time unit...");
            ui->cbPStepRepeatUnit->addItem("Minutes");
            ui->cbPStepRepeatUnit->addItem("Hours");
            ui->cbPStepRepeatUnit->addItem("Days");
            ui->cbPStepRepeatUnit->addItem("Number of Times");

            ui->laPLinDurationSave->setText("m.");
            ui->laPTRiseSave->setText("m.");
            ui->laPTUpSave->setText("m.");
            ui->laPTFallSave->setText("m.");
            ui->laPTDownSave->setText("m.");
            ui->laPSPeriodSave->setText("m.");
        }
        else if ( ui->cbPSelectSUnit->currentIndex() == 3 )
        {
            //hour selected for step time unit, do what you gotta do.

            pProfileSave[currentProfile].active = 1;
            pProfileSave[currentProfile].step[currentPStep].stepUnit = 3;
            pProfileSave[currentProfile].startValue = ui->dsbPStartValue->value();

            ui->cbPStepRepeatUnit->clear();
            ui->cbPStepRepeatUnit->addItem("Select a step repeat time unit...");
            ui->cbPStepRepeatUnit->addItem("Hours");
            ui->cbPStepRepeatUnit->addItem("Days");
            ui->cbPStepRepeatUnit->addItem("Number of Times");

            ui->laPLinDurationSave->setText("h.");
            ui->laPTRiseSave->setText("h.");
            ui->laPTUpSave->setText("h.");
            ui->laPTFallSave->setText("h.");
            ui->laPTDownSave->setText("h.");
            ui->laPSPeriodSave->setText("h.");
        }
        else if ( ui->cbPSelectSUnit->currentIndex() == 4 )
        {
            //day selected for step time unit, do what you gotta do.

            pProfileSave[currentProfile].active = 1;
            pProfileSave[currentProfile].step[currentPStep].stepUnit = 4;
            pProfileSave[currentProfile].startValue = ui->dsbPStartValue->value();

            ui->cbPStepRepeatUnit->clear();
            ui->cbPStepRepeatUnit->addItem("Select a step repeat time unit...");
            ui->cbPStepRepeatUnit->addItem("Days");
            ui->cbPStepRepeatUnit->addItem("Number of Times");

            ui->laPLinDurationSave->setText("d.");
            ui->laPTRiseSave->setText("d.");
            ui->laPTUpSave->setText("d.");
            ui->laPTFallSave->setText("d.");
            ui->laPTDownSave->setText("d.");
            ui->laPSPeriodSave->setText("d.");
        }

        if ( ui->cbPSelectSType->currentIndex() == 1 )
        {
            //linear step type selected do what you gotta do.

            pProfileSave[currentProfile].step[currentPStep].stepType = 1;
            ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
        }
        else if ( ui->cbPSelectSType->currentIndex() == 2 )
        {
            //trapezoid step type selected do what you gotta do.
            pProfileSave[currentProfile].step[currentPStep].stepType = 2;
            ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
        }
        else if ( ui->cbPSelectSType->currentIndex() == 3 )
        {
            //sinusoid step type selected do what you gotta do.
            pProfileSave[currentProfile].step[currentPStep].stepType = 3;
            ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 3);
        }
    }
}
*/
/*
void MainWindow::on_bPForward2_clicked()
{  
    if ( ( ui->dsbPLTarget->value() >= 0 ) && ( ui->dsbPLTarget->value() <= 10 ) )
    {
        pProfileSave[currentProfile].step[currentPStep].lDuration = ui->dsbPLDuration->value();
        pProfileSave[currentProfile].step[currentPStep].lTarget = ui->dsbPLTarget->value();

        float delta = qFabs(pProfileSave[currentProfile].step[currentPStep].lTarget - oldPValue);
        float duration = pProfileSave[currentProfile].step[currentPStep].lDuration;
        float pressureCondition;

        if ( ( delta >= 8 ) && ( delta <= 10 ) )
        {
            pressureCondition = 2.5;
        }
        else if ( ( delta >= 5 ) && ( delta < 8 ) )
        {
            pressureCondition = 2;
        }
        else if ( ( delta >= 2.5 ) && ( delta < 5 ) )
        {
            pressureCondition = 1.5;
        }
        else if ( ( delta >= 1 ) && ( delta < 2.5 ) )
        {
            pressureCondition = 1;
        }
        else if ( ( delta >= 0 ) && ( delta < 1 ) )
        {
            pressureCondition = 0.5;
        }

        if (ui->cbPSelectSUnit->currentIndex() == 1)
        {
            if ( ( pressureCondition ) <= ( duration * 1 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = duration * 1;
                updatePPreview();
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 4);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 2)
        {
            if ( ( pressureCondition ) <= ( duration * 60 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = duration * 60;
                updatePPreview();
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 4);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 3)
        {
            if ( ( pressureCondition ) <= ( duration * 60 * 60 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = duration * 60 * 60;
                updatePPreview();
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 4);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 4)
        {
            if ( ( pressureCondition ) <= ( duration * 60 * 60 * 24 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = duration * 60 * 60 * 24;
                updatePPreview();
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 4);
            }
        }
    }
    else
    {

    }
    qApp->processEvents();
}
*/
/*
void MainWindow::on_bPBack2_clicked()
{
    ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 1);
}
*/
/*
void MainWindow::on_bPForward3_clicked()
{
    ui->laOldPValue->setText(QString::number(oldPValue));

    if ( ui->dsbPTLow->value() == oldPValue )
    {
        pProfileSave[currentProfile].step[currentPStep].tRise = ui->dsbPTRise->value();
        pProfileSave[currentProfile].step[currentPStep].tUp = ui->dsbPTUp->value();
        pProfileSave[currentProfile].step[currentPStep].tFall = ui->dsbPTFall->value();
        pProfileSave[currentProfile].step[currentPStep].tDown = ui->dsbPTDown->value();
        pProfileSave[currentProfile].step[currentPStep].tLow = ui->dsbPTLow->value();
        pProfileSave[currentProfile].step[currentPStep].tHigh = ui->dsbPTHigh->value();

        float delta = pProfileSave[currentProfile].step[currentPStep].tHigh -
                pProfileSave[currentProfile].step[currentPStep].tLow;
        float rise = pProfileSave[currentProfile].step[currentPStep].tRise;
        float up = pProfileSave[currentProfile].step[currentPStep].tUp;
        float fall = pProfileSave[currentProfile].step[currentPStep].tFall;
        float down = pProfileSave[currentProfile].step[currentPStep].tDown;
        float period = (rise + up + fall + down);
        float pressureCondition;

        if ( ( delta >= 8 ) && ( delta <= 10 ) )
        {
            pressureCondition = 2.5;
        }
        else if ( ( delta >= 5 ) && ( delta < 8 ) )
        {
            pressureCondition = 2;
        }
        else if ( ( delta >= 2.5 ) && ( delta < 5 ) )
        {
            pressureCondition = 1.5;
        }
        else if ( ( delta >= 1 ) && ( delta < 2.5 ) )
        {
            pressureCondition = 1;
        }
        else if ( ( delta >= 0 ) && ( delta < 1 ) )
        {
            pressureCondition = 0.5;
        }

        if (ui->cbPSelectSUnit->currentIndex() == 1)
        {
            if ( ( pressureCondition <= ( rise * 1 ) ) && ( pressureCondition <= ( fall * 1 ) ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 1;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
            }
            else if ( pressureCondition > ( rise * 1 ))
            {
                bool ok = false;
                int adminPass = QInputDialog::getInt(this, tr(appName),
                                             tr("Girilen Pressure Rise değeri tanımlı cihaz fonksiyonunu aşmaktadır.\n"
                                                "bu değerlerle devam etmek için şifre girin. \n"
                                                "(UYARI! Cihaz garanti kapsamı dışında kalır.)"), 0, 0, 9999, 1, &ok);
                if (ok)
                {
                    if (adminPass == 1881)
                    {
                        writeToLogTable("Pressure Rise Value Out off limits");
                        pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 1;
                        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
                    }
                    else
                    {
                        QMessageBox::information(
                                    this,
                                    tr(appName),
                                    tr("Yanlış şifre girildi.") );
                    }
                }
             }
            else if ( pressureCondition > ( fall * 1 ))
            {
                bool ok = false;
                int adminPass = QInputDialog::getInt(this, tr(appName),
                                             tr("Girilen pressure fall değeri tanımlı cihaz fonksiyonunu aşmaktadır.\n"
                                                "bu değerlerle devam etmek için şifre girin \n"
                                                "(Cihaz garanti kapsamı dışında kalır)"), 0, 0, 9999, 1, &ok);
                if (ok)
                {
                    if (adminPass == 1881)
                    {
                        writeToLogTable("Pressure Fall Value Out off limits");
                        pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 1;
                        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
                    }
                    else
                    {
                        QMessageBox::information(
                                    this,
                                    tr(appName),
                                    tr("Yanlış şifre girildi.") );
                    }
                }

            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 2)
        {
            if ( ( pressureCondition <= ( rise * 60 ) ) && ( pressureCondition <= ( fall * 60 ) ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 60;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 3)
        {
            if ( ( pressureCondition <= ( rise * 60 * 60 ) ) && ( pressureCondition <= ( fall * 60 * 60 ) ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 60 * 60;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 4)
        {
            if ( ( pressureCondition <= ( rise * 60 * 60 * 24 ) ) && ( pressureCondition <= ( fall * 60 * 60 * 24 ) ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = ( period ) * 60 * 60 * 24;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 2);
            }
        }
    }
    else
    {
        // there must be linear step before this step to make the oldValue equal to trapezoid low value

    }
}
*/
/*
void MainWindow::on_bPBack3_clicked()
{
    ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 2);
}
*/
/*
void MainWindow::on_bPForward4_clicked()
{
    if ( ( ui->dsbPSMean->value() == oldPValue ) && ( ( ui->dsbPSMean->value() - ui->dsbPSAmp->value() ) >= 0 ) &&
         ( ( ui->dsbPSMean->value() + ui->dsbPSAmp->value() ) <= 10 ) )
    {
        pProfileSave[currentProfile].step[currentPStep].sPeriod = ui->dsbPSPeriod->value();
        pProfileSave[currentProfile].step[currentPStep].sMean = ui->dsbPSMean->value();
        pProfileSave[currentProfile].step[currentPStep].sAmp = ui->dsbPSAmp->value();

        float pressureCondition;
        float delta = pProfileSave[currentProfile].step[currentPStep].sAmp;
        float period = pProfileSave[currentProfile].step[currentPStep].sPeriod;

        if ( ( delta >= 8 ) && ( delta <= 10 ) )
        {
            pressureCondition = 2.5;
        }
        else if ( ( delta >= 5 ) && ( delta < 8 ) )
        {
            pressureCondition = 2;
        }
        else if ( ( delta >= 2.5 ) && ( delta < 5 ) )
        {
            pressureCondition = 1.5;
        }
        else if ( ( delta >= 1 ) && ( delta < 2.5 ) )
        {
            pressureCondition = 1;
        }
        else if ( ( delta >= 0 ) && ( delta < 1 ) )
        {
            pressureCondition = 0.5;
        }

        if (ui->cbPSelectSUnit->currentIndex() == 1)
        {
            if ( pressureCondition <= ( period * 1 / 4 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = period * 1;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 2)
        {
            if ( pressureCondition <= ( period * 60 / 4 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = period * 60;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 3)
        {
            if ( pressureCondition <= ( period * 60 * 60 / 4 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = period * 60 * 60;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
            }
        }
        else if (ui->cbPSelectSUnit->currentIndex() == 4)
        {
            if ( pressureCondition <= ( period * 60 * 60 * 24 / 4 ) )
            {
                pProfileSave[currentProfile].stepDuration[currentPStep] = period * 60 * 60 * 24;
                ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
            }
        }
    }
    else
    {
        // there must be linear step before this step to make the oldValue equal to sinusoid mean value
        // (mean - amp) value cannot be smaller than 0 bar
        // (mean + amp) value cannot be greater than 10 bar
    }
}
*/
/*
void MainWindow::on_bPBack4_clicked()
{
    ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 3);
}
*/
/*
void MainWindow::on_bPForward5_clicked()
{
    QString arg1 = ui->cbPStepRepeatUnit->currentText();

    if (arg1 == "Select a step repeat time unit...")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 0;
    }
    else if (arg1 == "Seconds")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 1;
    }
    else if (arg1 == "Minutes")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 2;
    }
    else if (arg1 == "Hours")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 3;
    }
    else if (arg1 == "Days")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 4;
    }
    else if (arg1 == "Number of Times")
    {
        pProfileSave[currentProfile].step[currentPStep].repeatUnit = 5;
    }

    pProfileSave[currentProfile].step[currentPStep].repeatDuration = ui->dsbPRepeatValue->value();
    updatePPreview();
    ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() + 1);
}
*/
/*
void MainWindow::on_bPBack5_clicked()
{
    if ( ui->cbPSelectSType->currentIndex() == 1 )
    {
        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 3);
    }
    else if ( ui->cbPSelectSType->currentIndex() == 2 )
    {
        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 2);
    }
    else if ( ui->cbPSelectSType->currentIndex() == 3 )
    {
        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 1);
    }
}
*/
/*
void MainWindow::on_bPSaveStep_clicked()
{
    ui->dsbPStartValue->setEnabled(false);

    ui->bPForward5->setEnabled(false);

    pProfileSave[currentProfile].totalStep++;
    currentPStep++;
    ui->laPTotalStep->setText(QString::number(currentPStep));

    if (pProfileSave[currentProfile].step[currentPStep - 1].stepType == 1)
    {
        oldPValue = pProfileSave[currentProfile].step[currentPStep - 1].lTarget;
    }
    else if (pProfileSave[currentProfile].step[currentPStep - 1].stepType == 2)
    {
        oldPValue = pProfileSave[currentProfile].step[currentPStep - 1].tLow;
    }
    else if (pProfileSave[currentProfile].step[currentPStep - 1].stepType == 3)
    {
        oldPValue = pProfileSave[currentProfile].step[currentPStep - 1].sMean;
    }

    ui->laOldPValue->setText(QString::number(oldPValue));

    ui->dsbPLDuration->setValue(1);
    ui->dsbPLTarget->setValue(0);

    ui->dsbPTFall->setValue(1);
    ui->dsbPTRise->setValue(1);
    ui->dsbPTUp->setValue(1);
    ui->dsbPTLow->setValue(0);
    ui->dsbPTHigh->setValue(0);

    ui->dsbPSMean->setValue(0);
    ui->dsbPSAmp->setValue(0);
    ui->dsbPSPeriod->setValue(1);

    ui->dsbPRepeatValue->setValue(0);
    ui->dsbPRepeatTime->setValue(0);

    ui->cbPSelectSType->setCurrentIndex(0);
    ui->cbPSelectSUnit->setCurrentIndex(0);
    ui->cbPStepRepeatUnit->setCurrentIndex(0);

    ui->pWidget->setCurrentIndex(0);
}
*/
/*
void MainWindow::on_bPBack6_clicked()
{
    if ( ui->cbPSelectSType->currentIndex() == 1 )
    {
        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 4);
    }
    else
    {
        ui->pWidget->setCurrentIndex(ui->pWidget->currentIndex() - 1);
    }

}
*/
/*
void MainWindow::on_cbPStepRepeatUnit_currentIndexChanged(const QString &arg1)
{
    ui->dsbPRepeatValue->setValue(0);

    if (arg1 == "Select a step repeat time unit...")
    {
        ui->laPRepeatSave->setText("");
    }
    else if (arg1 == "Seconds")
    {
        ui->laPRepeatSave->setText("s.");
    }
    else if (arg1 == "Minutes")
    {
        ui->laPRepeatSave->setText("m.");
    }
    else if (arg1 == "Hours")
    {
        ui->laPRepeatSave->setText("h.");
    }
    else if (arg1 == "Days")
    {
        ui->laPRepeatSave->setText("d.");
    }
    else if (arg1 == "Number of Times")
    {
        ui->laPRepeatSave->setText("t.");
    }

}
*/
/*
void MainWindow::on_dsbPRepeatValue_valueChanged(double arg1)
{
    float result;
    float duration = pProfileSave[currentProfile].stepDuration[currentPStep];

    ui->dsbPRepeatTime->setValue(0);

    if (ui->cbPStepRepeatUnit->currentText() == "Seconds")
    {
        result = ui->dsbPRepeatValue->value() * 1 / duration;
        ui->dsbPRepeatTime->setValue(result);
    }
    else if (ui->cbPStepRepeatUnit->currentText() == "Minutes")
    {
        result = ui->dsbPRepeatValue->value() * 60 / duration;
        ui->dsbPRepeatTime->setValue(result);
    }
    else if (ui->cbPStepRepeatUnit->currentText() == "Hours")
    {
        result = ui->dsbPRepeatValue->value() * 60 * 60 / duration;
        ui->dsbPRepeatTime->setValue(result);
    }
    else if (ui->cbPStepRepeatUnit->currentText() == "Days")
    {
        result = ui->dsbPRepeatValue->value() * 60 * 60 * 24 / duration;
        ui->dsbPRepeatTime->setValue(result);
    }
    else if (ui->cbPStepRepeatUnit->currentText() == "Number of Times")
    {
        result = ui->dsbPRepeatValue->value();
        ui->dsbPRepeatTime->setValue(result);
    }
    else
    {
        ui->dsbPRepeatTime->setValue(0);
    }

    if (ui->dsbPRepeatTime->value() == 0)
    {
        ui->bPForward5->setEnabled(false);
    }
    else
    {
        ui->bPForward5->setEnabled(true);
    }
}
*/
/*
void MainWindow::updatePPreview()
{
    //ui->pPreview->clearPlottables();
    //ui->pPreview->xAxis->setRange(0, ui->dsbPLTarget->value());
    //ui->pPreview->yAxis->setRange(1, 10);

    if (pProfileSave[currentProfile].step[currentPStep].stepUnit == 1)
    {
        ui->pPreview->xAxis->setLabel("Time (seconds)");
    }
    else if (pProfileSave[currentProfile].step[currentPStep].stepUnit == 2)
    {
        ui->pPreview->xAxis->setLabel("Time (minutes)");
    }
    else if (pProfileSave[currentProfile].step[currentPStep].stepUnit == 3)
    {
        ui->pPreview->xAxis->setLabel("Time (hours)");
    }
    else if (pProfileSave[currentProfile].step[currentPStep].stepUnit == 4)
    {
        ui->pPreview->xAxis->setLabel("Time (days)");
    }


    if (pProfileSave[currentProfile].step[currentPStep].stepType == 1)        //linear plot
    {
        float delta;
        float duration = pProfileSave[currentProfile].step[currentPStep].lDuration;
        float target = pProfileSave[currentProfile].step[currentPStep].lTarget;

        QVector<double> pPreviewX( ( quint16( duration * 10 )  ) + 1 ), pPreviewY( ( quint16( duration * 10 ) ) + 1 );

        pPreviewY[0] = oldPValue;
        pPreviewX[0] = 0;
        delta = target - oldPValue;

        for(int i=1; i < ( quint16( duration * 10 ) ) + 1; i++)
        {
            pPreviewY[i] = pPreviewY[i-1] + ( delta / ( duration * 10 ) );
            pPreviewX[i] = float(i) / 10;
        }

        ui->pPreview->graph(0)->setData(pPreviewX, pPreviewY);
        ui->pPreview->graph(0)->rescaleAxes();
        ui->pPreview->replot();
    }
    else if (pProfileSave[currentProfile].step[currentPStep].stepType == 2)   //trapezoid plot
    {
        // commented blocks make a sinusoid-like trapezoid with mean and amp values and rise up and down times.

        float rise = pProfileSave[currentProfile].step[currentPStep].tRise;
        float up = pProfileSave[currentProfile].step[currentPStep].tUp;
        float fall = pProfileSave[currentProfile].step[currentPStep].tFall;
        float down = pProfileSave[currentProfile].step[currentPStep].tDown;
        float low = pProfileSave[currentProfile].step[currentPStep].tLow;
        float high = pProfileSave[currentProfile].step[currentPStep].tHigh;
        float period = rise + up + fall + down;

        QVector<double> pPreviewX( quint16( period * 10 ) + 1 ), pPreviewY( quint16( period * 10 ) + 1 );
        //QVector<double> pPreviewX( quint16( period * 20 ) + 1 ), pPreviewY( quint16( period * 20 ) + 1 );

        pPreviewY[0] = oldPValue;
        pPreviewX[0] = 0;

        quint16 segment1 = ( rise * 10 ) + 1;
        quint16 segment2 = ( ( rise + up ) * 10) + 1;
        quint16 segment3 = ( ( rise + up + fall ) * 10) + 1;
        quint16 segment4 = ( ( rise + up + fall + down ) * 10) + 1;
        //quint16 segment3 = ( ( rise + up + ( fall * 2 ) ) * 10) + 1;
        //quint16 segment4 = ( ( rise + up + ( fall * 2 ) + up ) * 10) + 1;
        //quint16 segment5 = ( ( rise + up + ( fall * 2 ) + up + rise ) * 10) + 1;

        for(int i=1; i < segment1 ; i++)    // from low to high
        {
            pPreviewY[i] = pPreviewY[i-1] + ( (high - low) / ( rise * 10 ) );
            pPreviewX[i] = float(i) / 10;
        }

        for(int i = segment1; i < segment2; i++)    // stay at high
        {
            pPreviewY[i] = high;
            pPreviewX[i] = float(i) / 10;
        }

        for(int i = segment2; i < segment3; i++)    // from high to low
        {
            pPreviewY[i] = pPreviewY[i-1] - ( (high - low) / ( fall * 10 ) );
            pPreviewX[i] = float(i) / 10;
        }

        for(int i = segment3; i < segment4; i++)    // stay at low
        {
            pPreviewY[i] = low;
            pPreviewX[i] = float(i) / 10;
        }


        for(int i = segment3; i < segment4; i++)    // stay at amp*(-1)
        {
            pPreviewY[i] = mean - amp;
            pPreviewX[i] = float(i) / 10;
        }

        for(int i = segment4; i < segment5; i++)    // from amp*(-1) to mean
        {
            pPreviewY[i] = pPreviewY[i-1] + ( amp / ( rise * 10 ) );
            pPreviewX[i] = float(i) / 10;
        }


        ui->pPreview->graph(0)->setData(pPreviewX, pPreviewY);
        ui->pPreview->graph(0)->rescaleAxes();
        ui->pPreview->replot();

    }
    else if (pProfileSave[currentProfile].step[currentPStep].stepType == 3)   //sinusoid plot
    {
        float period = pProfileSave[currentProfile].step[currentPStep].sPeriod;
        float mean = pProfileSave[currentProfile].step[currentPStep].sMean;
        float amp = pProfileSave[currentProfile].step[currentPStep].sAmp;

        QVector<double> pPreviewX( ( quint16( period * 10 ) + 1 ) ), pPreviewY( ( quint16( period * 10 ) + 1 ) );

        pPreviewY[0] = oldPValue;
        pPreviewX[0] = 0;

        for(int i=1; i < ( quint16( period * 10 ) ) + 1; i++)
        {
            pPreviewY[i] = ( qSin( ( ( 2 * M_PI * i ) / ( period * 10 ) ) ) * amp ) + mean;
            pPreviewX[i] = float(i) / 10;
        }

        ui->pPreview->graph(0)->setData(pPreviewX, pPreviewY);
        ui->pPreview->graph(0)->rescaleAxes();
        ui->pPreview->replot();
    }
}
*/
/*
void MainWindow::on_bVForward2_clicked()
{
    float delta;

    vProfileSave[currentProfile].step[currentVStep].lDuration = ui->sbVLDuration->value();
    vProfileSave[currentProfile].step[currentVStep].lTarget = ui->dsbVLTarget->value();

    float startValue = vProfileSave[currentProfile].startValue;
    float duration = vProfileSave[currentProfile].step[currentVStep].lDuration;
    float oldTarget = vProfileSave[currentProfile].step[currentVStep-1].lTarget;
    float target = vProfileSave[currentProfile].step[currentVStep].lTarget;


    if ( currentVStep == 0 )
    {
        delta = qFabs( target - startValue );
    }
    else
    {
        delta = qFabs( target - oldTarget);
    }

    if ( ui->cbVSelectSUnit->currentIndex() == 1 )
    {
        if ( ( delta * 1 / 2 ) <= ( duration * 1 ) )
        {
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 3);
            updateVPreview();
        }
    }
    else if ( ui->cbVSelectSUnit->currentIndex() == 2 )
    {
        if ( ( delta * 1 / 2 ) <= ( duration * 60 ) )
        {
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 3);
            updateVPreview();
        }
    }
    else if ( ui->cbVSelectSUnit->currentIndex() == 3 )
    {
        if ( ( delta * 1 / 2 ) <= ( duration * 60 * 60 ) )
        {
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 3);
            updateVPreview();
        }
    }
    else if ( ui->cbVSelectSUnit->currentIndex() == 4 )
    {
        if ( ( delta * 1 / 2 ) <= ( duration * 60 * 60 * 24 ) )
        {
            ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 3);
            updateVPreview();
        }
    }
}
*/
/*
void MainWindow::on_bVBack2_clicked()
{
    ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() - 1);
}

void MainWindow::on_bVBack3_clicked()
{
    ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() - 2);
}

void MainWindow::on_bVForward3_clicked()
{
    if ( (ui->dsbVLogMax->value() > ui->dsbVLogMin->value()) )
    {
        vProfileSave[currentProfile].step[currentVStep].logRate = ui->dsbVLogRate->value();
        vProfileSave[currentProfile].step[currentVStep].logMin = ui->dsbVLogMin->value();
        vProfileSave[currentProfile].step[currentVStep].logMax = ui->dsbVLogMax->value();

        float rate = vProfileSave[currentProfile].step[currentVStep].logRate;
        float min = vProfileSave[currentProfile].step[currentVStep].logMin;
        float max = vProfileSave[currentProfile].step[currentVStep].logMax;
        float delta = max - min;
        double coeff = qPow(2, rate/60.0);
        quint16 duration=0;
        double buddy = min;
        while (buddy < max)
        {
            buddy = buddy * coeff;
            duration++;
        }

        // multiply by two because "her çıkışın bir inişi de vardır."
        vProfileSave[currentProfile].stepDuration[currentVStep] = duration * 2;

        if ( ui->cbVSelectSUnit->currentIndex() == 1 )
        {
            if ( ( delta * 1 ) <= ( duration ) )
            {
                ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 1);
            }
        }
    }
}
*/
/*
void MainWindow::on_bVForward4_clicked()
{
    QString arg1 = ui->cbVStepRepeatUnit->currentText();

    if (arg1 == "Select a step repeat time unit...")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 0;
    }
    else if (arg1 == "Seconds")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 1;
    }
    else if (arg1 == "Minutes")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 2;
    }
    else if (arg1 == "Hours")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 3;
    }
    else if (arg1 == "Days")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 4;
    }
    else if (arg1 == "Number of Times")
    {
        vProfileSave[currentProfile].step[currentVStep].repeatUnit = 5;
    }

    vProfileSave[currentProfile].step[currentVStep].repeatDuration = ui->dsbVRepeatValue->value();
    updateVPreview();
    ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() + 1);
}

void MainWindow::on_bVBack4_clicked()
{
    if ( ui->cbVSelectSType->currentIndex() == 2 )
    {
        ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() - 1);
    }
}
*/
/*
void MainWindow::on_bVBack5_clicked()
{
    if ( ui->cbVSelectSType->currentIndex() == 1 )
    {
        ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() - 3);
    }
    else if ( ui->cbVSelectSType->currentIndex() == 2 )
    {
        ui->vWidget->setCurrentIndex(ui->vWidget->currentIndex() - 1);
    }
}
*/
/*
void MainWindow::on_bVSaveStep_clicked()
{
    ui->dsbVStartValue->setEnabled(false);
    ui->bVForward4->setEnabled(false);

    vProfileSave[currentProfile].totalStep++;
    currentVStep++;
    ui->laVTotalStep->setText(QString::number(currentVStep));

    if (vProfileSave[currentProfile].step[currentVStep - 1].stepType == 1)
    {
        oldVValue = vProfileSave[currentProfile].step[currentVStep - 1].lTarget;
    }
    else if (vProfileSave[currentProfile].step[currentVStep - 1].stepType == 4)
    {
        oldVValue = vProfileSave[currentProfile].step[currentVStep - 1].logMin;
    }

    ui->laOldVValue->setText(QString::number(oldVValue));

    ui->cbVSelectSType->setCurrentIndex(0);
    ui->cbVSelectSUnit->setCurrentIndex(0);
    ui->cbVStepRepeatUnit->setCurrentIndex(0);

    ui->sbVLDuration->setValue(1);
    ui->dsbVLTarget->setValue(0);

    ui->dsbVLogRate->setValue(0.1);
    ui->dsbVLogMin->setValue(0.1);
    ui->dsbVLogMax->setValue(0.1);

    ui->dsbVRepeatValue->setValue(0);
    ui->dsbVRepeatTime->setValue(0);

    ui->vWidget->setCurrentIndex(0);
}
*/
/*
void MainWindow::on_cbVStepRepeatUnit_currentIndexChanged(const QString &arg1)
{
    ui->dsbVRepeatValue->setValue(0);

    if (arg1 == "Select a step repeat time unit...")
    {
        ui->laVRepeatSave->setText("");
    }
    else if (arg1 == "Seconds")
    {
        ui->laVRepeatSave->setText("s.");
    }
    else if (arg1 == "Minutes")
    {
        ui->laVRepeatSave->setText("m.");
    }
    else if (arg1 == "Hours")
    {
        ui->laVRepeatSave->setText("h.");
    }
    else if (arg1 == "Days")
    {
        ui->laVRepeatSave->setText("d.");
    }
    else if (arg1 == "Number of Times")
    {
        ui->laVRepeatSave->setText("t.");
    }

}
*/
/*
void MainWindow::on_dsbVRepeatValue_valueChanged(double arg1)
{
    float result;
    float duration = vProfileSave[currentProfile].stepDuration[currentVStep];

    ui->dsbVRepeatTime->setValue(0);

    if (ui->cbVStepRepeatUnit->currentText() == "Seconds")
    {
        result = ui->dsbVRepeatValue->value() * 1 / duration;
        ui->dsbVRepeatTime->setValue(result);
    }
    else if (ui->cbVStepRepeatUnit->currentText() == "Minutes")
    {
        result = ui->dsbVRepeatValue->value() * 60 / duration;
        ui->dsbVRepeatTime->setValue(result);
    }
    else if (ui->cbVStepRepeatUnit->currentText() == "Hours")
    {
        result = ui->dsbVRepeatValue->value() * 60 * 60 / duration;
        ui->dsbVRepeatTime->setValue(result);
    }
    else if (ui->cbVStepRepeatUnit->currentText() == "Days")
    {
        result = ui->dsbVRepeatValue->value() * 60 * 60 * 24 / duration;
        ui->dsbVRepeatTime->setValue(result);
    }
    else if (ui->cbVStepRepeatUnit->currentText() == "Number of Times")
    {
        result = ui->dsbVRepeatValue->value();
        ui->dsbVRepeatTime->setValue(result);
    }
    else
    {
        ui->dsbVRepeatTime->setValue(0);
    }

    if (ui->dsbVRepeatTime->value() == 0)
    {
        ui->bVForward4->setEnabled(false);
    }
    else
    {
        ui->bVForward4->setEnabled(true);
    }
}
*/
/*
void MainWindow::updateVPreview()
{
    //ui->vPreview->clearPlottables();
    //ui->vPreview->xAxis->setRange(0, ui->dsbVLTarget->value());
    //ui->vPreview->yAxis->setRange(1, 60);

    if (vProfileSave[currentProfile].step[currentVStep].stepUnit == 1)
    {
        ui->vPreview->xAxis->setLabel("Time (seconds)");
    }
    else if (vProfileSave[currentProfile].step[currentVStep].stepUnit == 2)
    {
        ui->vPreview->xAxis->setLabel("Time (minutes)");
    }
    else if (vProfileSave[currentProfile].step[currentVStep].stepUnit == 3)
    {
        ui->vPreview->xAxis->setLabel("Time (hours)");
    }
    else if (vProfileSave[currentProfile].step[currentVStep].stepUnit == 4)
    {
        ui->vPreview->xAxis->setLabel("Time (days)");
    }

    if (vProfileSave[currentProfile].step[currentVStep].stepType == 1)
    {
        float delta;
        float duration = vProfileSave[currentProfile].step[currentVStep].lDuration;
        float startValue = vProfileSave[currentProfile].startValue;
        float oldTarget = vProfileSave[currentProfile].step[currentVStep - 1].lTarget;
        float target = vProfileSave[currentProfile].step[currentVStep].lTarget;

        QVector<double> vPreviewX( ( duration * 10) + 1 ), vPreviewY( ( duration * 10) + 1 );

        if (currentVStep == 0)
        {
            vPreviewY[0] = startValue;
            vPreviewX[0] = 0;
            delta = target - startValue;
        }
        else
        {
            vPreviewY[0] = oldTarget;
            vPreviewX[0] = 0;
            delta = target - oldTarget;
        }

        for(int i=1; i < ( duration * 10) + 1; i++)
        {
            vPreviewY[i] = vPreviewY[i-1] + ( delta / ( duration * 10 ) );
            vPreviewX[i] = float(i) / 10;
        }

        ui->vPreview->graph(0)->setData(vPreviewX, vPreviewY);
        ui->vPreview->graph(0)->rescaleAxes();
        ui->vPreview->replot();
    }
    else if (vProfileSave[currentProfile].step[currentVStep].stepType == 4)
    {
        float rate = vProfileSave[currentProfile].step[currentVStep].logRate;
        float min = vProfileSave[currentProfile].step[currentVStep].logMin;
        float max = vProfileSave[currentProfile].step[currentVStep].logMax;
        double coeff = qPow(2, rate/600.0);
        quint16 duration=0;
        double buddy = min;
        while (buddy < max)
        {
            buddy = buddy * coeff;
            duration++;
        }

        duration = duration * 2;

        QVector<double> vPreviewX( ( duration) + 1 ), vPreviewY( ( duration) + 1 );

        vPreviewY[0] = min;
        vPreviewX[0] = 0;

        for(int i=1; i < ( duration / 2 ) + 1; i++)
        {
            vPreviewY[i] = vPreviewY[i-1] * coeff ;
            vPreviewX[i] = float(i) / 10;
        }
        for(int i=( duration / 2 ) + 1; i < (duration) + 1; i++)
        {
            vPreviewY[i] = vPreviewY[i-1] / coeff ;
            vPreviewX[i] = float(i) / 10;
        }

        ui->vPreview->graph(0)->setData(vPreviewX, vPreviewY);
        ui->vPreview->graph(0)->rescaleAxes();
        ui->vPreview->replot();

    }
}
*/

bool MainWindow::readProfiles(char rType, int index)
{
    if (index == 0)
    {

    }
    else
    {
        QByteArray rProfile;
        quint16 pos;

#ifdef Q_OS_LINUX
        //linux code goes here
        QString filePath = "/home/pi/InDetail/profiles/Profile " + QString::number(index) + ".dat";
#endif
#ifdef Q_OS_WIN
        // windows code goes here
        QString filePath = "profiles\\Profile " + QString::number(index) + ".dat";
#endif

        QFile file(filePath);

        if(!file.open(QIODevice::ReadOnly))
        {
            if (rType != 'g')
            {
            QMessageBox::information(
                        this,
                        tr(appName),
                        tr("Could not open profile file to be read.") );
            return false;
            }
            else
            {
                ui->cbSelectProfileMain->addItem(QString::number(index) + " - empty slot");
                ui->cbSelectProfileMain->setItemData(index, QColor( Qt::gray ), Qt::TextColorRole );
            }
        }
        else
        {
            rProfile = file.readAll();

            QString name;

            if (rType == 'e')
            {
                pos = 0;

                if (rProfile[pos] == '/')
                {
                    ui->leProfileNameEdit->setText("No Name Given");
                }
                else
                {
                    while (rProfile[pos] != '/')
                    {
                        name.append(char(rProfile[pos]));
                        pos++;
                    }
                    ui->leProfileNameEdit->setText(name);
                }
                pos=pos+26;

                tProfileEdit[index - 1].active = rProfile[pos];
                pos = pos + 2;

                tProfileEdit[index - 1].startValue = qint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                tProfileEdit[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<tProfileEdit[index - 1].totalStep; i++)
                {
                    pos = pos + 7;

                    tProfileEdit[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    tProfileEdit[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    tProfileEdit[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                    pos = pos + 3;

                    tProfileEdit[index - 1].step[i].lTarget = qint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }

                pos = pos + 22;

                pProfileEdit[index - 1].active = rProfile[pos];
                pos = pos + 2;

                pProfileEdit[index - 1].startValue = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                pProfileEdit[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<pProfileEdit[index - 1].totalStep; i++)
                {
                    pos = pos + 7;
                    pProfileEdit[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    pProfileEdit[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    pProfileEdit[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].lTarget = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tRise = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tUp = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tFall = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tDown = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tLow = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].tHigh = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].sPeriod = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].sMean = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].sAmp = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileEdit[index - 1].step[i].repeatUnit = rProfile[pos];
                    pos = pos + 2;

                    pProfileEdit[index - 1].step[i].repeatDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }

                pos = pos + 23;

                //qDebug() << QByteArray::number(rProfile[pos],16);
                //qDebug() << QByteArray::number(rProfile[pos+1],16);

                vProfileEdit[index - 1].active = rProfile[pos];
                pos = pos + 2;

                vProfileEdit[index - 1].startValue = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                vProfileEdit[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<vProfileEdit[index - 1].totalStep; i++)
                {
                    pos = pos + 7;

                    vProfileEdit[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    vProfileEdit[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    vProfileEdit[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                    pos = pos + 3;

                    vProfileEdit[index - 1].step[i].lTarget = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileEdit[index - 1].step[i].logRate = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 1000.0;
                    pos = pos + 3;

                    vProfileEdit[index - 1].step[i].logMin = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileEdit[index - 1].step[i].logMax = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileEdit[index - 1].step[i].repeatUnit = rProfile[pos];
                    pos = pos + 2;

                    vProfileEdit[index - 1].step[i].repeatDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }
            }
            else if (rType == 'l')
            {
                pos = 0;

                if (rProfile[pos] == '/')
                {
                    ui->laSelectedProfileMain->setText("No Name Given");
                    ui->laSelectedProfileManual->setText("No Name Given");
                }
                else
                {
                    while (rProfile[pos] != '/')
                    {
                        name.append(char(rProfile[pos]));
                        pos++;
                    }
                    ui->laSelectedProfileMain->setText(name);
                    ui->laSelectedProfileManual->setText(name);
                }
                pos=pos+26;

                tProfileLoad[index - 1].active = rProfile[pos];
                pos = pos + 2;

                tProfileLoad[index - 1].startValue = qint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                tProfileLoad[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<tProfileLoad[index - 1].totalStep; i++)
                {
                    pos = pos + 7;

                    tProfileLoad[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    tProfileLoad[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    tProfileLoad[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                    pos = pos + 3;

                    tProfileLoad[index - 1].step[i].lTarget = qint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }

                pos = pos + 22;

                pProfileLoad[index - 1].active = rProfile[pos];
                pos = pos + 2;

                pProfileLoad[index - 1].startValue = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                pProfileLoad[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<pProfileLoad[index - 1].totalStep; i++)
                {
                    pos = pos + 7;

                    pProfileLoad[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    pProfileLoad[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    pProfileLoad[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].lTarget = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tRise = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tUp = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tFall = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tDown = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tLow = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].tHigh = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].sPeriod = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].sMean = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].sAmp = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    pProfileLoad[index - 1].step[i].repeatUnit = rProfile[pos];
                    pos = pos + 2;

                    pProfileLoad[index - 1].step[i].repeatDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }

                pos = pos + 23;

                vProfileLoad[index - 1].active = rProfile[pos];
                pos = pos + 2;

                vProfileLoad[index - 1].startValue = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                pos = pos + 3;

                vProfileLoad[index - 1].totalStep = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                pos = pos + 3;

                for(int i=0; i<vProfileLoad[index - 1].totalStep; i++)
                {
                    pos = pos + 7;

                    vProfileLoad[index - 1].step[i].stepUnit = rProfile[pos];
                    pos = pos + 2;

                    vProfileLoad[index - 1].step[i].stepType = rProfile[pos];
                    pos = pos + 2;

                    vProfileLoad[index - 1].step[i].lDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff));
                    pos = pos + 3;

                    vProfileLoad[index - 1].step[i].lTarget = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileLoad[index - 1].step[i].logRate = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 1000.0;
                    pos = pos + 3;

                    vProfileLoad[index - 1].step[i].logMin = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileLoad[index - 1].step[i].logMax = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;

                    vProfileLoad[index - 1].step[i].repeatUnit = rProfile[pos];
                    pos = pos + 2;

                    vProfileLoad[index - 1].step[i].repeatDuration = quint16(((rProfile[pos+1] & 0xff) << 8) | (rProfile[pos] & 0xff)) / 10.0;
                    pos = pos + 3;
                }
            }
            else if(rType == 'g')
            {
                pos = 0;

                if (rProfile[pos] == '/')
                {
                    ui->cbSelectProfileMain->addItem(QString::number(index) + " - " + "isimsiz");

                }
                else
                {
                    while (rProfile[pos] != '/')
                    {
                        name.append(char(rProfile[pos]));
                        pos++;
                    }
                    ui->cbSelectProfileMain->addItem( QString::number(index) + " - " + name);
                }

            }
        }
    }
    return true;
}

void MainWindow::on_bSavePro_clicked()
{

    QByteArray profile;

#ifdef Q_OS_LINUX
    //linux code goes here
    QString filePath = "/home/pi/InDetail/profiles/" + ui->cbSelectProfile->currentText() + ".dat";
#endif
#ifdef Q_OS_WIN
    // windows code goes here
    QString filePath = "profiles\\" + ui->cbSelectProfile->currentText() + ".dat";
#endif

    QFile file(filePath);

    file.remove();

    profile.append(ui->leProfileName->text().toLocal8Bit());
    profile.append("/");

    profile.append("Temperature General Info");
    profile.append("/");
    profile.append(tProfileSave[currentProfile].active);
    profile.append("/");
    float tStart = tProfileSave[currentProfile].startValue*10.0;
    profile.append(qint16(tStart) & 0x00FF);
    profile.append(qint16(tStart) >> 8);
    profile.append("/");
    float tTotalStep = tProfileSave[currentProfile].totalStep;
    profile.append(quint16(tTotalStep) & 0x00FF);
    profile.append(quint16(tTotalStep) >> 8);

    for(int i=0; i<tProfileSave[currentProfile].totalStep; i++)
    {
        profile.append("/");
        profile.append("Step-");
        profile.append(i+1);
        profile.append("/");
        quint8 tStepUnit = tProfileSave[currentProfile].step[i].stepUnit;
        profile.append(tStepUnit);
        profile.append("/");
        quint8 tStepType = tProfileSave[currentProfile].step[i].stepType;
        profile.append(tStepType);
        profile.append("/");
        float tLDuration = tProfileSave[currentProfile].step[i].lDuration;
        profile.append(quint16(tLDuration) & 0x00FF);
        profile.append(quint16(tLDuration) >> 8);
        profile.append("/");
        float tLTarget = tProfileSave[currentProfile].step[i].lTarget*10.0;
        profile.append(qint16(tLTarget) & 0x00FF);
        profile.append(qint16(tLTarget) >> 8);
    }

    profile.append("/");
    profile.append("Pressure General Info");
    profile.append("/");
    profile.append(pProfileSave[currentProfile].active);
    profile.append("/");
    float pStart = pProfileSave[currentProfile].startValue*10.0;
    profile.append(quint16(pStart) & 0x00FF);
    profile.append(quint16(pStart) >> 8);
    profile.append("/");
    float pTotalStep = pProfileSave[currentProfile].totalStep;
    profile.append(quint16(pTotalStep) & 0x00FF);
    profile.append(quint16(pTotalStep) >> 8);

    for(int i=0; i<pProfileSave[currentProfile].totalStep; i++)
    {
        profile.append("/");
        profile.append("Step-");
        profile.append(i+1);
        profile.append("/");
        quint8 pStepUnit = pProfileSave[currentProfile].step[i].stepUnit;
        profile.append(pStepUnit);
        profile.append("/");
        quint8 pStepType = pProfileSave[currentProfile].step[i].stepType;
        profile.append(pStepType);
        profile.append("/");
        float pLDuration = pProfileSave[currentProfile].step[i].lDuration*10.0;
        profile.append(quint16(pLDuration) & 0x00FF);
        profile.append(quint16(pLDuration) >> 8);
        profile.append("/");
        float pLTarget = pProfileSave[currentProfile].step[i].lTarget*10.0;
        profile.append(quint16(pLTarget) & 0x00FF);
        profile.append(quint16(pLTarget) >> 8);
        profile.append("/");
        float pTRise = pProfileSave[currentProfile].step[i].tRise*10.0;
        profile.append(quint16(pTRise) & 0x00FF);
        profile.append(quint16(pTRise) >> 8);
        profile.append("/");
        float pTUp = pProfileSave[currentProfile].step[i].tUp*10.0;
        profile.append(quint16(pTUp) & 0x00FF);
        profile.append(quint16(pTUp) >> 8);
        profile.append("/");
        float pTFall = pProfileSave[currentProfile].step[i].tFall*10.0;
        profile.append(quint16(pTFall) & 0x00FF);
        profile.append(quint16(pTFall) >> 8);
        profile.append("/");
        float pTDown = pProfileSave[currentProfile].step[i].tDown*10.0;
        profile.append(quint16(pTDown) & 0x00FF);
        profile.append(quint16(pTDown) >> 8);
        profile.append("/");
        float ptLow = pProfileSave[currentProfile].step[i].tLow*10.0;
        profile.append(quint16(ptLow) & 0x00FF);
        profile.append(quint16(ptLow) >> 8);
        profile.append("/");
        float ptHigh = pProfileSave[currentProfile].step[i].tHigh*10.0;
        profile.append(quint16(ptHigh) & 0x00FF);
        profile.append(quint16(ptHigh) >> 8);
        profile.append("/");
        float pSPeriod = pProfileSave[currentProfile].step[i].sPeriod*10.0;
        profile.append(quint16(pSPeriod) & 0x00FF);
        profile.append(quint16(pSPeriod) >> 8);
        profile.append("/");
        float pSMean = pProfileSave[currentProfile].step[i].sMean*10.0;
        profile.append(quint16(pSMean) & 0x00FF);
        profile.append(quint16(pSMean) >> 8);
        profile.append("/");
        float pSAmp = pProfileSave[currentProfile].step[i].sAmp*10.0;
        profile.append(quint16(pSAmp) & 0x00FF);
        profile.append(quint16(pSAmp) >> 8);
        profile.append("/");
        quint8 pRepeatUnit = pProfileSave[currentProfile].step[i].repeatUnit;
        profile.append(pRepeatUnit);
        profile.append("/");
        float pRepeatDuration = pProfileSave[currentProfile].step[i].repeatDuration*10.0;
        profile.append(quint16(pRepeatDuration) & 0x00FF);
        profile.append(quint16(pRepeatDuration) >> 8);
    }

    profile.append("/");
    profile.append("Vibration General Info");
    profile.append("/");
    profile.append(vProfileSave[currentProfile].active);
    profile.append("/");
    float vStart = vProfileSave[currentProfile].startValue*10.0;
    profile.append(quint16(vStart) & 0x00FF);
    profile.append(quint16(vStart) >> 8);
    profile.append("/");
    float vTotalStep = vProfileSave[currentProfile].totalStep;
    profile.append(quint16(vTotalStep) & 0x00FF);
    profile.append(quint16(vTotalStep) >> 8);

    for(int i=0; i<vProfileSave[currentProfile].totalStep; i++)
    {
        profile.append("/");
        profile.append("Step-");
        profile.append(i+1);
        profile.append("/");
        quint8 vStepUnit = vProfileSave[currentProfile].step[i].stepUnit;
        profile.append(vStepUnit);
        profile.append("/");
        quint8 vStepType = vProfileSave[currentProfile].step[i].stepType;
        profile.append(vStepType);
        profile.append("/");
        float vLDuration = vProfileSave[currentProfile].step[i].lDuration;
        profile.append(quint16(vLDuration) & 0x00FF);
        profile.append(quint16(vLDuration) >> 8);
        profile.append("/");
        float vLTarget = vProfileSave[currentProfile].step[i].lTarget*10.0;
        profile.append(quint16(vLTarget) & 0x00FF);
        profile.append(quint16(vLTarget) >> 8);
        profile.append("/");
        float vLogRate = vProfileSave[currentProfile].step[i].logRate*1000;
        profile.append(quint16(vLogRate) & 0x00FF);
        profile.append(quint16(vLogRate) >> 8);
        profile.append("/");
        float vLogMin = vProfileSave[currentProfile].step[i].logMin*10.0;
        profile.append(quint16(vLogMin) & 0x00FF);
        profile.append(quint16(vLogMin) >> 8);
        profile.append("/");
        float vLogMax = vProfileSave[currentProfile].step[i].logMax*10.0;
        profile.append(quint16(vLogMax) & 0x00FF);
        profile.append(quint16(vLogMax) >> 8);
        profile.append("/");
        quint8 vRepeatUnit = vProfileSave[currentProfile].step[i].repeatUnit;
        profile.append(vRepeatUnit);
        profile.append("/");
        float vRepeatDuration = vProfileSave[currentProfile].step[i].repeatDuration*10.0;
        profile.append(quint16(vRepeatDuration) & 0x00FF);
        profile.append(quint16(vRepeatDuration) >> 8);
    }

    profile.append("/End of Profile.");


    QString name = QString::number(currentProfile + 1) +" - "+ ui->leProfileName->text();
    ui->cbSelectProfileMain->setItemText(currentProfile + 1, name);

    ui->cbSelectProfile->setCurrentIndex(0);
    ui->cbSelectProfile->setEnabled(true);

    ui->leProfileName->setText("");
    ui->leProfileName->setEnabled(false);

    ui->dsbTStartValue->setValue(0);
    ui->dsbTStartValue->setEnabled(false);
    //  ui->dsbPStartValue->setValue(0);
    //  ui->dsbPStartValue->setEnabled(false);
    //  ui->dsbVStartValue->setValue(0);
    //  ui->dsbVStartValue->setEnabled(false);

    ui->laTTotalStep->setText("0");
    //  ui->laPTotalStep->setText("0");
    //  ui->laVTotalStep->setText("0");

    ui->tWidget->setCurrentIndex(0);
    //  ui->pWidget->setCurrentIndex(0);
    //  ui->vWidget->setCurrentIndex(0);

    ui->cbTSelectSUnit->setEnabled(false);
    //  ui->cbPSelectSUnit->setEnabled(false);
    //  ui->cbVSelectSUnit->setEnabled(false);
    //ui->cbTSelectSType->setEnabled(false);
    //  ui->cbPSelectSType->setEnabled(false);
    //  ui->cbVSelectSType->setEnabled(false);

    ui->tPreview->clearPlottables();
    //  ui->pPreview->clearPlottables();
    //  ui->vPreview->clearPlottables();

    currentTStep = 0;
    currentPStep = 0;
    currentVStep = 0;

    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::information(
                    this,
                    tr(appName),
                    tr("Could not create profile file to be written.") );
    }
    else
    {
        if (file.write(profile))
        {
            file.flush();
            file.close();
            QMessageBox::information(
                        this,
                        tr(appName),
                        tr("Profile saved successfully.") );
        }
        else
        {
            QMessageBox::information(
                        this,
                        tr(appName),
                        tr("Error while writing profile info to profile file." ));
        }
    }

    ui->tabWidget->setTabEnabled(0, true);
    ui->tabWidget->setTabEnabled(1, true);
    ui->tabWidget->setTabEnabled(2, true);
    ui->tabWidget->setTabEnabled(4, true);
    ui->tabWidget->setTabEnabled(5, true);

}

void MainWindow::on_cbSelectProfileEdit_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->cbSelectPTypeEdit->setEnabled(false);
        ui->cbSelectPTypeEdit->setCurrentIndex(0);

        ui->cbSelectStepEdit->setEnabled(false);
        ui->cbSelectStepEdit->setCurrentIndex(0);

        ui->cbSelectSTypeEdit->setEnabled(false);
        ui->cbSelectSTypeEdit->setCurrentIndex(0);
    }
    else
    {
        ui->cbSelectPTypeEdit->setEnabled(true);
        ui->cbSelectPTypeEdit->setCurrentIndex(0);
        ui->cbSelectStepEdit->setCurrentIndex(0);
        ui->cbSelectSTypeEdit->setCurrentIndex(0);

        readProfiles('e', index);
    }
}

void MainWindow::on_cbSelectPTypeEdit_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->dsbStartValueEdit->setValue(0);
        ui->dsbStartValueEdit->setEnabled(false);

        ui->laTotalStepEdit->setText("");
        ui->laTotalStepEdit->setEnabled(false);

        ui->cbSelectSTypeEdit->setCurrentIndex(0);
        ui->cbSelectSTypeEdit->setEnabled(false);

        ui->cbSelectStepEdit->setCurrentIndex(0);
        ui->cbSelectStepEdit->setEnabled(false);

        ui->laTestStartEdit->setText("");


    }
    else if (index == 1)
    {
        ui->cbSelectStepEdit->setEnabled(true);

        ui->dsbStartValueEdit->setValue(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].startValue);
        ui->laTotalStepEdit->setText(QString::number(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep));

        ui->cbSelectSTypeEdit->clear();
        ui->cbSelectSTypeEdit->addItem("Select a step type...");
        ui->cbSelectSTypeEdit->addItem("Linear");
        ui->cbSelectSTypeEdit->setCurrentIndex(0);

        ui->cbSelectStepEdit->clear();
        ui->cbSelectStepEdit->addItem("Select a step number...");

        for(int i=0; i<tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep; i++)
        {
            ui->cbSelectStepEdit->addItem("Step " + QString::number(i+1));
        }
        ui->cbSelectStepEdit->setCurrentIndex(0);

        ui->laTestStartEdit->setText("°C");
    }
    else if (index == 2)
    {
        ui->cbSelectStepEdit->setEnabled(true);

        ui->dsbStartValueEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].startValue);
        ui->laTotalStepEdit->setText(QString::number(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep));

        ui->cbSelectSTypeEdit->clear();
        ui->cbSelectSTypeEdit->addItem("Select a step type...");
        ui->cbSelectSTypeEdit->addItem("Linear");
        ui->cbSelectSTypeEdit->addItem("Trapezoid");
        ui->cbSelectSTypeEdit->addItem("Sinusoid");
        ui->cbSelectSTypeEdit->setCurrentIndex(0);

        ui->cbSelectStepEdit->clear();
        ui->cbSelectStepEdit->addItem("Select a step number...");

        for(int i=0; i<pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep; i++)
        {
            ui->cbSelectStepEdit->addItem("Step " + QString::number(i+1));
        }
        ui->cbSelectStepEdit->setCurrentIndex(0);

        ui->laTestStartEdit->setText("bar");
    }
    else if (index == 3)
    {
        ui->cbSelectStepEdit->setEnabled(true);

        ui->dsbStartValueEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].startValue);
        ui->laTotalStepEdit->setText(QString::number(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep));

        ui->cbSelectSTypeEdit->clear();
        ui->cbSelectSTypeEdit->addItem("Select a step type...");
        ui->cbSelectSTypeEdit->addItem("Linear");
        ui->cbSelectSTypeEdit->addItem("Logarithmic Sweep");
        ui->cbSelectSTypeEdit->setCurrentIndex(0);

        ui->cbSelectStepEdit->clear();
        ui->cbSelectStepEdit->addItem("Select a step number...");

        for(int i=0; i<vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].totalStep; i++)
        {
            ui->cbSelectStepEdit->addItem("Step " + QString::number(i+1));
        }
        ui->cbSelectStepEdit->setCurrentIndex(0);

        ui->laTestStartEdit->setText("Hz");
    }
}

void MainWindow::on_cbSelectStepEdit_currentIndexChanged(int index)
{
    if (ui->cbSelectPTypeEdit->currentIndex() == 0)
    {
        ui->cbSelectSUnitEdit->setCurrentIndex(0);
        ui->cbSelectSTypeEdit->setCurrentIndex(0);

        ui->dsbLDurationEdit->setValue(0);
        ui->dsbLTargetEdit->setValue(0);

        ui->dsbTRiseEdit->setValue(0);
        ui->dsbTUpEdit->setValue(0);
        ui->dsbTFallEdit->setValue(0);
        ui->dsbTDownEdit->setValue(0);
        ui->dsbTLowEdit->setValue(0);
        ui->dsbTHighEdit->setValue(0);

        ui->dsbSPeriodEdit->setValue(0);
        ui->dsbSMeanEdit->setValue(0);
        ui->dsbSAmpEdit->setValue(0);

        ui->dsbLogRateEdit->setValue(0);
        ui->dsbLogMinEdit->setValue(0);
        ui->dsbLogMaxEdit->setValue(0);

        ui->dsbRepeatDurationEdit->setValue(0);
        ui->laStepDurationEdit->setText("");
        ui->cbSRepeatUnitEdit->setCurrentIndex(0);

    }
    else if (ui->cbSelectPTypeEdit->currentIndex() == 1)
    {
        ui->cbSelectSUnitEdit->setCurrentIndex(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepUnit);
        ui->cbSelectSTypeEdit->setCurrentIndex(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType);

        ui->dsbLDurationEdit->setValue(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lDuration);
        ui->dsbLTargetEdit->setValue(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lTarget);

        ui->dsbTRiseEdit->setValue(0);
        ui->dsbTUpEdit->setValue(0);
        ui->dsbTFallEdit->setValue(0);
        ui->dsbTDownEdit->setValue(0);
        ui->dsbTLowEdit->setValue(0);
        ui->dsbTHighEdit->setValue(0);

        ui->dsbSPeriodEdit->setValue(0);
        ui->dsbSMeanEdit->setValue(0);
        ui->dsbSAmpEdit->setValue(0);

        ui->dsbRepeatDurationEdit->setValue(0);
        ui->laStepDurationEdit->setText("");
        ui->cbSRepeatUnitEdit->setCurrentIndex(0);
    }
    else if (ui->cbSelectPTypeEdit->currentIndex() == 2)
    {
        ui->cbSelectSUnitEdit->setCurrentIndex(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepUnit);
        ui->cbSelectSTypeEdit->setCurrentIndex(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType);

        ui->cbSRepeatUnitEdit->setCurrentIndex(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit);
        ui->dsbRepeatDurationEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatDuration);

        if(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 1)
        {
            ui->laStepDurationEdit->setText("s.");
        }
        else if(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 2)
        {
            ui->laStepDurationEdit->setText("m.");
        }
        else if(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 3)
        {
            ui->laStepDurationEdit->setText("h.");
        }
        else if(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 4)
        {
            ui->laStepDurationEdit->setText("d.");
        }

        if (pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType == 1)
        {
            ui->dsbLDurationEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lDuration);
            ui->dsbLTargetEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lTarget);

            ui->dsbTRiseEdit->setValue(0);
            ui->dsbTUpEdit->setValue(0);
            ui->dsbTFallEdit->setValue(0);
            ui->dsbTDownEdit->setValue(0);
            ui->dsbTLowEdit->setValue(0);
            ui->dsbTHighEdit->setValue(0);

            ui->dsbSPeriodEdit->setValue(0);
            ui->dsbSMeanEdit->setValue(0);
            ui->dsbSAmpEdit->setValue(0);

            ui->dsbLogRateEdit->setValue(0);
            ui->dsbLogMinEdit->setValue(0);
            ui->dsbLogMaxEdit->setValue(0);
        }
        else if (pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType == 2)
        {
            ui->dsbTRiseEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tRise);
            ui->dsbTUpEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tUp);
            ui->dsbTFallEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tFall);
            ui->dsbTDownEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tDown);
            ui->dsbTLowEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tLow);
            ui->dsbTHighEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].tHigh);

            ui->dsbLDurationEdit->setValue(0);
            ui->dsbLTargetEdit->setValue(0);

            ui->dsbSPeriodEdit->setValue(0);
            ui->dsbSMeanEdit->setValue(0);
            ui->dsbSAmpEdit->setValue(0);

            ui->dsbLogRateEdit->setValue(0);
            ui->dsbLogMinEdit->setValue(0);
            ui->dsbLogMaxEdit->setValue(0);
        }
        else if (pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType == 3)
        {
            ui->dsbSPeriodEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].sPeriod);
            ui->dsbSMeanEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].sMean);
            ui->dsbSAmpEdit->setValue(pProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].sAmp);

            ui->dsbLDurationEdit->setValue(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lDuration);
            ui->dsbLTargetEdit->setValue(tProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lTarget);

            ui->dsbTRiseEdit->setValue(0);
            ui->dsbTUpEdit->setValue(0);
            ui->dsbTFallEdit->setValue(0);
            ui->dsbTDownEdit->setValue(0);
            ui->dsbTLowEdit->setValue(0);
            ui->dsbTHighEdit->setValue(0);

            ui->dsbLogRateEdit->setValue(0);
            ui->dsbLogMinEdit->setValue(0);
            ui->dsbLogMaxEdit->setValue(0);
        }
    }
    else if (ui->cbSelectPTypeEdit->currentIndex() == 3)
    {
        ui->cbSelectSUnitEdit->setCurrentIndex(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepUnit);

        if (vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType == 1)
        {
            ui->cbSelectSTypeEdit->setCurrentIndex(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType);

            ui->dsbLDurationEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lDuration);
            ui->dsbLTargetEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].lTarget);

            ui->dsbTRiseEdit->setValue(0);
            ui->dsbTUpEdit->setValue(0);
            ui->dsbTFallEdit->setValue(0);
            ui->dsbTDownEdit->setValue(0);
            ui->dsbTLowEdit->setValue(0);
            ui->dsbTHighEdit->setValue(0);

            ui->dsbSPeriodEdit->setValue(0);
            ui->dsbSMeanEdit->setValue(0);
            ui->dsbSAmpEdit->setValue(0);

            ui->dsbLogRateEdit->setValue(0);
            ui->dsbLogMinEdit->setValue(0);
            ui->dsbLogMaxEdit->setValue(0);

            ui->dsbRepeatDurationEdit->setValue(0);
            ui->laStepDurationEdit->setText("");
            ui->cbSRepeatUnitEdit->setCurrentIndex(0);
        }
        else if (vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType == 4)
        {
            ui->cbSelectSTypeEdit->setCurrentIndex(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].stepType - 2);

            ui->dsbLDurationEdit->setValue(0);
            ui->dsbLTargetEdit->setValue(0);

            ui->dsbTRiseEdit->setValue(0);
            ui->dsbTUpEdit->setValue(0);
            ui->dsbTFallEdit->setValue(0);
            ui->dsbTDownEdit->setValue(0);
            ui->dsbTLowEdit->setValue(0);
            ui->dsbTHighEdit->setValue(0);

            ui->dsbSPeriodEdit->setValue(0);
            ui->dsbSMeanEdit->setValue(0);
            ui->dsbSAmpEdit->setValue(0);

            ui->dsbLogRateEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].logRate);
            ui->dsbLogMinEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].logMin);
            ui->dsbLogMaxEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].logMax);

            ui->cbSRepeatUnitEdit->setCurrentIndex(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit);
            ui->dsbRepeatDurationEdit->setValue(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatDuration);

            if(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 1)
            {
                ui->laStepDurationEdit->setText("s.");
            }
            else if(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 2)
            {
                ui->laStepDurationEdit->setText("m.");
            }
            else if(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 3)
            {
                ui->laStepDurationEdit->setText("h.");
            }
            else if(vProfileEdit[ui->cbSelectProfileEdit->currentIndex()-1].step[index-1].repeatUnit == 4)
            {
                ui->laStepDurationEdit->setText("d.");
            }
        }
    }
}

void MainWindow::on_cbSelectSTypeEdit_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->wdRepeatEdit->setVisible(false);
        ui->wdLinearEdit->setVisible(false);
        ui->wdTrapEdit->setVisible(false);
        ui->wdSinEdit->setVisible(false);
        ui->wdLogEdit->setVisible(false);

        ui->laLinTargetEdit->setText("");

        ui->laTrapLowEdit->setText("");
        ui->laTrapHighEdit->setText("");

        ui->laSinMeanEdit->setText("");
        ui->laSinAmplitudeEdit->setText("");

        ui->laLogMinEdit->setText("");
        ui->laLogMaxEdit->setText("");

    }
    else if (index == 1)
    {
        ui->wdRepeatEdit->setVisible(false);
        ui->wdLinearEdit->setVisible(true);
        ui->wdTrapEdit->setVisible(false);
        ui->wdSinEdit->setVisible(false);
        ui->wdLogEdit->setVisible(false);

        if (ui->cbSelectPTypeEdit->currentIndex() == 1)
        {
            ui->laLinTargetEdit->setText("°C");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }
        else if (ui->cbSelectPTypeEdit->currentIndex() == 2)
        {
            ui->laLinTargetEdit->setText("bar");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }
        else if (ui->cbSelectPTypeEdit->currentIndex() == 3)
        {
            ui->laLinTargetEdit->setText("Hz");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }

    }
    else if (index == 2)
    {
        if (ui->cbSelectPTypeEdit->currentIndex() == 2)
        {
            ui->wdRepeatEdit->setVisible(true);
            ui->wdLinearEdit->setVisible(false);
            ui->wdTrapEdit->setVisible(true);
            ui->wdSinEdit->setVisible(false);
            ui->wdLogEdit->setVisible(false);

            ui->laLinTargetEdit->setText("");

            ui->laTrapLowEdit->setText("bar");
            ui->laTrapHighEdit->setText("bar");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");

        }
        else if (ui->cbSelectPTypeEdit->currentIndex() == 3)
        {
            ui->wdRepeatEdit->setVisible(true);
            ui->wdLinearEdit->setVisible(false);
            ui->wdTrapEdit->setVisible(false);
            ui->wdSinEdit->setVisible(false);
            ui->wdLogEdit->setVisible(true);

            ui->laLinTargetEdit->setText("");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("Hz");
            ui->laLogMaxEdit->setText("Hz");
        }
    }
    else if (index == 3)
    {
        ui->wdRepeatEdit->setVisible(true);
        ui->wdLinearEdit->setVisible(false);
        ui->wdTrapEdit->setVisible(false);
        ui->wdSinEdit->setVisible(true);
        ui->wdLogEdit->setVisible(false);

        if (ui->cbSelectPTypeEdit->currentIndex() == 1)
        {
            ui->laLinTargetEdit->setText("");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }
        else if (ui->cbSelectPTypeEdit->currentIndex() == 2)
        {
            ui->laLinTargetEdit->setText("");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("bar");
            ui->laSinAmplitudeEdit->setText("bar");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }
        else if (ui->cbSelectPTypeEdit->currentIndex() == 3)
        {
            ui->laLinTargetEdit->setText("");

            ui->laTrapLowEdit->setText("");
            ui->laTrapHighEdit->setText("");

            ui->laSinMeanEdit->setText("");
            ui->laSinAmplitudeEdit->setText("");

            ui->laLogMinEdit->setText("");
            ui->laLogMaxEdit->setText("");
        }
    }
}

void MainWindow::on_cbSelectSUnitEdit_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->laLinDurationEdit->setText("");

        ui->laTrapRiseEdit->setText("");
        ui->laTrapUpEdit->setText("");
        ui->laTrapFallEdit->setText("");
        ui->laTrapDownEdit->setText("");

        ui->laSinPeriodEdit->setText("");
    }
    else if (index == 1)
    {
        ui->laLinDurationEdit->setText("s.");

        ui->laTrapRiseEdit->setText("s.");
        ui->laTrapUpEdit->setText("s.");
        ui->laTrapFallEdit->setText("s.");
        ui->laTrapDownEdit->setText("s.");

        ui->laSinPeriodEdit->setText("s.");
    }
    else if (index == 2)
    {
        ui->laLinDurationEdit->setText("m.");

        ui->laTrapRiseEdit->setText("m.");
        ui->laTrapUpEdit->setText("m.");
        ui->laTrapFallEdit->setText("m.");
        ui->laTrapDownEdit->setText("m.");

        ui->laSinPeriodEdit->setText("m.");
    }
    else if (index == 3)
    {
        ui->laLinDurationEdit->setText("h.");

        ui->laTrapRiseEdit->setText("h.");
        ui->laTrapUpEdit->setText("h.");
        ui->laTrapFallEdit->setText("h.");
        ui->laTrapDownEdit->setText("h.");

        ui->laSinPeriodEdit->setText("h.");
    }
    else if (index == 4)
    {
        ui->laLinDurationEdit->setText("d.");

        ui->laTrapRiseEdit->setText("d.");
        ui->laTrapUpEdit->setText("d.");
        ui->laTrapFallEdit->setText("d.");
        ui->laTrapDownEdit->setText("d.");

        ui->laSinPeriodEdit->setText("d.");
    }

}

void MainWindow::on_cbSelectProfileMain_currentIndexChanged(int index)
{
    //ui->laCurrentTCycleMain->setText("0");

    ui->sbTTotalCycle->setValue(1);

    ui->bStartTest->setEnabled(false);

    ui->cbSelectProfileManual->setCurrentIndex(0);


    if (index == 0)
    {
        ui->laSelectedProfileMain->setText("No Profile Selected");
        ui->laSelectedProfileManual->setText("No Profile Selected");
        ui->bSendProfileMain->setEnabled(false);
        totalTestDuration = 0;
    }
    else
    {
        quint8 index = ui->cbSelectProfileMain->currentIndex();
        if (readProfiles('l', index))
        {
            if (tProfileLoad[index-1].active == 0)
            {
                ui->sbTTotalCycle->setValue(0);
                ui->sbTTotalCycle->setEnabled(false);
            }
            else
            {
                ui->sbTTotalCycle->setEnabled(true);
                for(int j=1; j <= (tProfileLoad[index-1].totalStep ); j++)
                {
                    totalTestDuration = totalTestDuration + tProfileLoad[index-1].step[j-1].lDuration;
                }
                ui->laTotalTestSecond->setText(QString::number(totalTestDuration)) ;
                ui->progressBar->setValue(0);
                ui->progressBar->setMinimum(0);
                ui->progressBar->setMaximum(totalTestDuration);
            }
        }
    }

    ui->bSendProfileMain->setEnabled(true);

}

bool MainWindow::sendProfileOverSerial(QString mode, int index)
{
    proc->stop();

    QByteArray cantTouchThis;           // bu satırda artık mıcıtmıyorum
    float tStart = tProfileLoad[index-1].startValue*10.0;
    float tTotalStep = tProfileLoad[index-1].totalStep;
    quint16 tTotalCycle;
    float tWaterTank;

    if (mode == "main")
    {
        tTotalCycle = ui->sbTTotalCycle->value();
        ui->laTTotalCycleMain->setText(QString::number(tTotalCycle));

    }
    else if (mode == "manual")
    {
        tTotalCycle = ui->sbTTotalCycleManual->value();
        ui->laTTotalCycleMain->setText(QString::number(tTotalCycle));
        //       tWaterTank = ui->dsbTankTempSetManual->value()*10.0;
    }

    cantTouchThis.append(tProfileLoad[index-1].active);
    cantTouchThis.append(qint16(tStart) & 0x00FF);
    cantTouchThis.append(qint16(tStart) >> 8);
    cantTouchThis.append(quint16(tTotalStep) & 0x00FF);
    cantTouchThis.append(quint16(tTotalStep) >> 8);
    cantTouchThis.append(quint16(tTotalCycle) & 0x00FF);
    cantTouchThis.append(quint16(tTotalCycle) >> 8);
    cantTouchThis.append(quint16(tWaterTank) & 0x00FF);
    cantTouchThis.append(quint16(tWaterTank) >> 8);

    proc->insertProfileMessage(mySerial::makeMessage(0x64,cantTouchThis));

    for(int i=0; i<tProfileLoad[index-1].totalStep; i++)
    {
        cantTouchThis.clear();

        quint8 tStepUnit = tProfileLoad[index-1].step[i].stepUnit;
        quint8 tStepType = tProfileLoad[index-1].step[i].stepType;
        float tLDuration = tProfileLoad[index-1].step[i].lDuration;
        float tLTarget = tProfileLoad[index-1].step[i].lTarget*10.0;

        cantTouchThis.append(i+1);
        cantTouchThis.append(tStepUnit);
        cantTouchThis.append(tStepType);
        cantTouchThis.append(quint16(tLDuration) & 0x00FF);
        cantTouchThis.append(quint16(tLDuration) >> 8);
        cantTouchThis.append(qint16(tLTarget) & 0x00FF);
        cantTouchThis.append(qint16(tLTarget) >> 8);

        proc->insertProfileMessage(mySerial::makeMessage(0x65,cantTouchThis));
    }

    cantTouchThis.clear();

    proc->insertProfileMessage(mySerial::makeMessage(0x66,cantTouchThis));

    cantTouchThis.clear();

    /*    float pStart = pProfileLoad[index-1].startValue*10.0;
    float pTotalStep = pProfileLoad[index-1].totalStep;
    quint16 pTotalCycle;



    cantTouchThis.append(pProfileLoad[index-1].active);
    cantTouchThis.append(quint16(pStart) & 0x00FF);
    cantTouchThis.append(quint16(pStart) >> 8);
    cantTouchThis.append(quint16(pTotalStep) & 0x00FF);
    cantTouchThis.append(quint16(pTotalStep) >> 8);
    cantTouchThis.append(quint16(pTotalCycle) & 0x00FF);
    cantTouchThis.append(quint16(pTotalCycle) >> 8);

    proc->insertProfileMessage(mySerial::makeMessage(0x67,cantTouchThis));

    for(int i=0; i<pProfileLoad[index-1].totalStep; i++)
    {
        if (pProfileLoad[index-1].step[i].stepType == 1)
        {
            cantTouchThis.clear();

            quint8 pStepUnit = pProfileLoad[index-1].step[i].stepUnit;
            quint8 pStepType = pProfileLoad[index-1].step[i].stepType;
            float pLDuration = pProfileLoad[index-1].step[i].lDuration*10.0;
            float pLTarget = pProfileLoad[index-1].step[i].lTarget*10.0;

            cantTouchThis.append(i+1);
            cantTouchThis.append(pStepUnit);
            cantTouchThis.append(pStepType);
            cantTouchThis.append(quint16(pLDuration) & 0x00FF);
            cantTouchThis.append(quint16(pLDuration) >> 8);
            cantTouchThis.append(quint16(pLTarget) & 0x00FF);
            cantTouchThis.append(quint16(pLTarget) >> 8);

            proc->insertProfileMessage(mySerial::makeMessage(0x68,cantTouchThis));
        }
        else if (pProfileLoad[index-1].step[i].stepType == 2)
        {
            cantTouchThis.clear();

            quint8 pStepUnit = pProfileLoad[index-1].step[i].stepUnit;
            quint8 pStepType = pProfileLoad[index-1].step[i].stepType;
            float pTRise = pProfileLoad[index-1].step[i].tRise*10.0;
            float pTUp = pProfileLoad[index-1].step[i].tUp*10.0;
            float pTFall = pProfileLoad[index-1].step[i].tFall*10.0;
            float pTDown = pProfileLoad[index-1].step[i].tDown*10.0;
            float ptLow = pProfileLoad[index-1].step[i].tLow*10.0;
            float ptHigh = pProfileLoad[index-1].step[i].tHigh*10.0;
            quint8 pRepeatUnit = pProfileLoad[index-1].step[i].repeatUnit;
            float pRepeatDuration = pProfileLoad[index-1].step[i].repeatDuration;

            cantTouchThis.append(i+1);
            cantTouchThis.append(pStepUnit);
            cantTouchThis.append(pStepType);
            cantTouchThis.append(quint16(pTRise) & 0x00FF);
            cantTouchThis.append(quint16(pTRise) >> 8);
            cantTouchThis.append(quint16(pTUp) & 0x00FF);
            cantTouchThis.append(quint16(pTUp) >> 8);
            cantTouchThis.append(quint16(pTFall) & 0x00FF);
            cantTouchThis.append(quint16(pTFall) >> 8);
            cantTouchThis.append(quint16(pTDown) & 0x00FF);
            cantTouchThis.append(quint16(pTDown) >> 8);
            cantTouchThis.append(quint16(ptLow) & 0x00FF);
            cantTouchThis.append(quint16(ptLow) >> 8);
            cantTouchThis.append(quint16(ptHigh) & 0x00FF);
            cantTouchThis.append(quint16(ptHigh) >> 8);
            cantTouchThis.append(pRepeatUnit);
            cantTouchThis.append(quint32(pRepeatDuration) & 0xFF);
            cantTouchThis.append((quint32(pRepeatDuration) >> 8)& 0xFF);
            cantTouchThis.append((quint32(pRepeatDuration) >> 16)& 0xFF);

            proc->insertProfileMessage(mySerial::makeMessage(0x68,cantTouchThis));
        }
        else if (pProfileLoad[index-1].step[i].stepType == 3)
        {
            cantTouchThis.clear();

            quint8 pStepUnit = pProfileLoad[index-1].step[i].stepUnit;
            quint8 pStepType = pProfileLoad[index-1].step[i].stepType;
            float pSPeriod = pProfileLoad[index-1].step[i].sPeriod*10.0;
            float pSMean = pProfileLoad[index-1].step[i].sMean*10.0;
            float pSAmp = pProfileLoad[index-1].step[i].sAmp*10.0;
            quint8 pRepeatUnit = pProfileLoad[index-1].step[i].repeatUnit;
            float pRepeatDuration = pProfileLoad[index-1].step[i].repeatDuration;

            cantTouchThis.append(i+1);
            cantTouchThis.append(pStepUnit);
            cantTouchThis.append(pStepType);
            cantTouchThis.append(quint16(pSPeriod) & 0x00FF);
            cantTouchThis.append(quint16(pSPeriod) >> 8);
            cantTouchThis.append(quint16(pSMean) & 0x00FF);
            cantTouchThis.append(quint16(pSMean) >> 8);
            cantTouchThis.append(quint16(pSAmp) & 0x00FF);
            cantTouchThis.append(quint16(pSAmp) >> 8);
            cantTouchThis.append(pRepeatUnit);
            cantTouchThis.append(quint16(pRepeatDuration) & 0x00FF);
            cantTouchThis.append(quint16(pRepeatDuration) >> 8);

            proc->insertProfileMessage(mySerial::makeMessage(0x68,cantTouchThis));
      }
  }
*/
    /*   cantTouchThis.clear();

    proc->insertProfileMessage(mySerial::makeMessage(0x69,cantTouchThis));

    cantTouchThis.clear();

    float vStart = vProfileLoad[index-1].startValue*10.0;
    float vTotalStep = vProfileLoad[index-1].totalStep;
    quint16 vTotalCycle;
    bool vEllipticalActive;



    cantTouchThis.append(vProfileLoad[index-1].active);
    cantTouchThis.append(quint16(vStart) & 0x00FF);
    cantTouchThis.append(quint16(vStart) >> 8);
    cantTouchThis.append(quint16(vTotalStep) & 0x00FF);
    cantTouchThis.append(quint16(vTotalStep) >> 8);
    cantTouchThis.append(quint16(vTotalCycle) & 0x00FF);
    cantTouchThis.append(quint16(vTotalCycle) >> 8);
    cantTouchThis.append(quint8(vEllipticalActive));

    proc->insertProfileMessage(mySerial::makeMessage(0x6A,cantTouchThis));

    for(int i=0; i<vProfileLoad[index-1].totalStep; i++)
    {
        if (vProfileLoad[index-1].step[i].stepType == 1)
        {
            cantTouchThis.clear();

            quint8 vStepUnit = vProfileLoad[index-1].step[i].stepUnit;
            quint8 vStepType = vProfileLoad[index-1].step[i].stepType;
            float vLDuration = vProfileLoad[index-1].step[i].lDuration;
            float vLTarget = vProfileLoad[index-1].step[i].lTarget*10.0;

            cantTouchThis.append(i+1);
            cantTouchThis.append(vStepUnit);
            cantTouchThis.append(vStepType);
            cantTouchThis.append(quint16(vLDuration) & 0x00FF);
            cantTouchThis.append(quint16(vLDuration) >> 8);
            cantTouchThis.append(quint16(vLTarget) & 0x00FF);
            cantTouchThis.append(quint16(vLTarget) >> 8);

            proc->insertProfileMessage(mySerial::makeMessage(0x6B,cantTouchThis));
        }
        else if (vProfileLoad[index-1].step[i].stepType == 4)
        {
            cantTouchThis.clear();

            quint8 vStepUnit = vProfileLoad[index-1].step[i].stepUnit;
            quint8 vStepType = vProfileLoad[index-1].step[i].stepType;
            float vLogRate = vProfileLoad[index-1].step[i].logRate*1000.0;
            float vLogMin = vProfileLoad[index-1].step[i].logMin*10.0;
            float vLogMax = vProfileLoad[index-1].step[i].logMax*10.0;
            quint8 vRepeatUnit = vProfileLoad[index-1].step[i].repeatUnit;
            float vRepeatDuration = vProfileLoad[index-1].step[i].repeatDuration;

            cantTouchThis.append(i+1);
            cantTouchThis.append(vStepUnit);
            cantTouchThis.append(vStepType);
            cantTouchThis.append(quint16(vLogRate) & 0x00FF);
            cantTouchThis.append(quint16(vLogRate) >> 8);
            cantTouchThis.append(quint16(vLogMin) & 0x00FF);
            cantTouchThis.append(quint16(vLogMin) >> 8);
            cantTouchThis.append(quint16(vLogMax) & 0x00FF);
            cantTouchThis.append(quint16(vLogMax) >> 8);
            cantTouchThis.append(vRepeatUnit);
            cantTouchThis.append(quint16(vRepeatDuration) & 0x00FF);
            cantTouchThis.append(quint16(vRepeatDuration) >> 8);

            proc->insertProfileMessage(mySerial::makeMessage(0x6B,cantTouchThis));
        }
    }
*/
    //    cantTouchThis.clear();

    //    proc->insertProfileMessage(mySerial::makeMessage(0x6C,cantTouchThis));

    if (mode == "main")
    {
        if (ui->cbSelectMethodManual->currentIndex() == 1)
        {
            cantTouchThis.clear();
            cantTouchThis.append(char(0x00));
            proc->insertProfileMessage(mySerial::makeMessage(0x6D,cantTouchThis));
        }
        else if (ui->cbSelectMethodManual->currentIndex() == 2)
        {
            cantTouchThis.clear();
            cantTouchThis.append(char(0x00));
            proc->insertProfileMessage(mySerial::makeMessage(0x6E,cantTouchThis));
        }

    }
    else if (mode == "manual")
    {
        quint16 tCycleToStart = ui->sbTCycleSetManual->value();
        //      quint16 pCycleToStart = ui->sbPCycleSetManual->value();
        //       quint16 vCycleToStart = ui->sbVCycleSetManual->value();

        if (ui->cbSelectMethodManual->currentIndex() == 1)
        {
            quint32 tSecondsToStart = ui->dsbTTimeSetManual->value();
            //         quint32 pSecondsToStart = ui->dsbPTimeSetManual->value();
            //         quint32 vSecondsToStart = ui->dsbVTimeSetManual->value();
            quint16 tCycleToStart = ui->sbTCycleSetManual->value();
            //         quint16 pCycleToStart = ui->sbPCycleSetManual->value();
            //        quint16 vCycleToStart = ui->sbVCycleSetManual->value();

            cantTouchThis.clear();
            cantTouchThis.append(0x01);

            cantTouchThis.append( (tSecondsToStart) & 0xFF );
            cantTouchThis.append( (tSecondsToStart >> 8) & 0xFF );
            cantTouchThis.append( (tSecondsToStart >> 16) & 0xFF );
            /*
            cantTouchThis.append( (pSecondsToStart) & 0xFF );
            cantTouchThis.append( (pSecondsToStart >> 8) & 0xFF );
            cantTouchThis.append( (pSecondsToStart >> 16) & 0xFF );

            cantTouchThis.append( (vSecondsToStart) & 0xFF );
            cantTouchThis.append( (vSecondsToStart >> 8) & 0xFF );
            cantTouchThis.append( (vSecondsToStart >> 16) & 0xFF );
*/
            cantTouchThis.append( (tCycleToStart) & 0x00FF);
            cantTouchThis.append( (tCycleToStart) >> 8);
            /*            cantTouchThis.append( (pCycleToStart) & 0x00FF);
            cantTouchThis.append( (pCycleToStart) >> 8);
            cantTouchThis.append( (vCycleToStart) & 0x00FF);
            cantTouchThis.append( (vCycleToStart) >> 8);
*/
            proc->insertProfileMessage(mySerial::makeMessage(0x6D,cantTouchThis));

            cantTouchThis.clear();
            cantTouchThis.append(char(0x00));
            proc->insertProfileMessage(mySerial::makeMessage(0x6E,cantTouchThis));
        }
        else if (ui->cbSelectMethodManual->currentIndex() == 2)
        {
            quint8 tStepToStart = ui->sbTStepSetManual->value();
            //            quint8 pStepToStart = ui->sbPStepSetManual->value();
            //           quint8 vStepToStart = ui->sbVStepSetManual->value();
            quint16 tStepSecondToStart = ui->sbTStepRepeatSetManual->value();
            //           quint16 pStepSecondToStart = ui->sbPStepRepeatSetManual->value();
            //           quint16 vStepSecondToStart = ui->sbVStepRepeatSetManual->value();

            cantTouchThis.clear();
            cantTouchThis.append(char(0x00));
            proc->insertProfileMessage(mySerial::makeMessage(0x6D,cantTouchThis));

            cantTouchThis.clear();
            cantTouchThis.append(0x01);
            cantTouchThis.append(tStepToStart);
            //            cantTouchThis.append(pStepToStart);
            //            cantTouchThis.append(vStepToStart);
            cantTouchThis.append( (tStepSecondToStart) & 0x00FF);
            cantTouchThis.append( (tStepSecondToStart) >> 8);
            /*          cantTouchThis.append( (pStepSecondToStart) & 0x00FF);
            cantTouchThis.append( (pStepSecondToStart) >> 8);
            cantTouchThis.append( (vStepSecondToStart) & 0x00FF);
            cantTouchThis.append( (vStepSecondToStart) >> 8);
 */           cantTouchThis.append( (tCycleToStart) & 0x00FF);
            cantTouchThis.append( (tCycleToStart) >> 8);
            /*           cantTouchThis.append( (pCycleToStart) & 0x00FF);
            cantTouchThis.append( (pCycleToStart) >> 8);
            cantTouchThis.append( (vCycleToStart) & 0x00FF);
            cantTouchThis.append( (vCycleToStart) >> 8);
*/
            proc->insertProfileMessage(mySerial::makeMessage(0x6E,cantTouchThis));
        }

    }

    proc->setProfile();

    return true;
}

void MainWindow::on_bStartTest_clicked()
{
    proc->stop();

    if (myPLC.deviceState == char(0x03))
    {

    }
    else
    {
        tKey = 0;
        pKey = 0;
        vKey = 0;

        //    ui->tTestGraph->graph(0)->data().clear();
        //    ui->pTestGraph->graph(0)->data().clear();
        //    ui->pTestGraph->graph(1)->data().clear();
        //    ui->pTestGraph->graph(2)->data().clear();
        //    ui->pTestGraph->graph(3)->data().clear();
        //    ui->pTestGraph->graph(4)->data().clear();
        //    ui->pTestGraph->graph(5)->data().clear();
        //    ui->vTestGraph->graph(0)->data().clear();
        //    ui->tTestGraph->replot();
        //    ui->pTestGraph->replot();
        //    ui->vTestGraph->replot();

        ui->tTestGraph->clearPlottables();
        ui->pTestGraph->clearPlottables();
        //ui->vTestGraph->clearPlottables();

        setupTGraph();
        setupPGraphs();
        setupVGraph();
    }

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x02);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));

}

void MainWindow::on_bStopTest_clicked()
{
    timerTemp->stop();
    //  timerVib->stop();
    timerPressure->stop();

    proc->stop();

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x01);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
}

void MainWindow::askSensorValues()
{
    QByteArray empty,cantTouchThis;

    cantTouchThis.append(mySerial::makeMessage(0x32,empty));
    //I will ask 0x33 with askOtherStuff function.
    //cantTouchThis.append(mySerial::makeMessage(0x33,empty));
    proc->setPeriodicMessage(cantTouchThis);
    proc->start();
}

void MainWindow::askOtherStuff()
{
    if ((ui->laCommErr->text() == "OK") && (proc->state != proc->sProfileSend))
    {
        QByteArray empty, cantTouchThis;
        cantTouchThis.clear();
        if (askCounter == 1)
        {
            cantTouchThis.append(mySerial::makeMessage(0x0C,empty));
        }
        else if (askCounter == 2)
        {
            cantTouchThis.append(mySerial::makeMessage(0x0D,empty));
        }
        else if (askCounter == 3)
        {
            cantTouchThis.append(mySerial::makeMessage(0x0E,empty));
        }
        else if (askCounter == 4)
        {
            cantTouchThis.append(mySerial::makeMessage(0x33,empty));
        }
        askCounter++;
        if (askCounter == 5) askCounter = 1;
        proc->insertCommandMessage(cantTouchThis);
    }
}

void MainWindow::getCurrentDateTime()
{
    ui->laTime->setText(QTime::currentTime().toString());
    ui->laDate->setText(QDate::currentDate().toString(Qt::SystemLocaleShortDate));
}

void MainWindow::updateTPlot()
{
    tKey = tElapsedSeconds + (double(tempPeriod)/1000.0) ;

    // add data to lines:
    ui->tTestGraph->graph(0)->addData(tKey, cabinTopTemperature);
    ui->tTestGraph->graph(1)->addData(tKey, cabinBottomTemperature);
    // rescale key (horizontal) axis to fit the current data:
    //  ui->tTestGraph->graph(0)->rescaleKeyAxis();
    // replot the graph with the added data
    ui->tTestGraph->replot();
    ui->progressBar->setValue(tElapsedSeconds);

#ifdef Q_OS_LINUX
    //linux code goes here
    QString filePath = "/home/pi/InDetail/records/" + testFolder + "/" + "temperature.csv";
#endif
#ifdef Q_OS_WIN
    // windows code goes here
    QString filePath = "records\\" + testFolder + "\\" + "temperature.csv";
#endif

    QFile file(filePath);

    if (file.open(QFile::WriteOnly|QFile::Append))
    {
        QTextStream stream(&file);
        stream << tKey << "," << cabinTopTemperature << "\n";
        file.close();
    }
}

void MainWindow::updatePPlots()
{
    pKey = pKey + (double(pressurePeriod)/1000.0);

    ui->pTestGraph->graph(0)->addData(pKey, pipe1Pressure);
    ui->pTestGraph->graph(1)->addData(pKey, pipe2Pressure);
    ui->pTestGraph->graph(2)->addData(pKey, pipe3Pressure);
    ui->pTestGraph->graph(3)->addData(pKey, pipe4Pressure);
    ui->pTestGraph->graph(4)->addData(pKey, pipe5Pressure);
    ui->pTestGraph->graph(5)->addData(pKey, pipe6Pressure);

    // make key axis range scroll with the data (at a constant range size of 30):
    ui->pTestGraph->xAxis->setRange(pKey, 60, Qt::AlignRight);

    /*
    // rescale key (horizontal) axis to fit the current data:
    ui->pTestGraph->graph(0)->rescaleKeyAxis();
    */

    // replot the graph with the added data
    ui->pTestGraph->replot();

#ifdef Q_OS_LINUX
    //linux code goes here
    QString filePath = "/home/pi/InDetail/records/" + testFolder + "/" + "pressure.csv";
#endif
#ifdef Q_OS_WIN
    // windows code goes here
    QString filePath = "records\\" + testFolder + "\\" + "pressure.csv";
#endif

    QFile file(filePath);

    if (file.open(QFile::WriteOnly|QFile::Append))
    {
        QTextStream stream(&file);
        stream << pKey << "," << pipe1Pressure << "," << pipe2Pressure << "," << pipe3Pressure << "," <<
                  pipe4Pressure << "," << pipe5Pressure << "," << pipe6Pressure << "\n";
        file.close();
    }

}
/*
void MainWindow::updateVPlot()
{
    vKey = vKey + (double(vibPeriod)/1000.0);

    ui->vTestGraph->graph(0)->addData(vKey, pipeVibrationFrequency);
    // rescale key (horizontal) axis to fit the current data:
    //ui->vTestGraph->graph(0)->rescaleKeyAxis();

    // make key axis range scroll with the data (at a constant range size of 30):
    ui->vTestGraph->xAxis->setRange(vKey, 60, Qt::AlignRight);

    // replot the graph with the added data
    ui->vTestGraph->replot();

#ifdef Q_OS_LINUX
    //linux code goes here
    QString filePath = "/home/pi/InDetail/records/" + testFolder + "/" + "vibration.csv";
#endif
#ifdef Q_OS_WIN
    // windows code goes here
    QString filePath = "records\\" + testFolder + "\\" + "vibration.csv";
#endif

    QFile file(filePath);

    if (file.open(QFile::WriteOnly|QFile::Append))
    {
    QTextStream stream(&file);
    stream << vKey << "\t" << pipeVibrationFrequency << "\n";
    file.close();
    }
}
*/
void MainWindow::on_bScreenshot_clicked()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QPixmap pixmap = screen->grabWindow(0);
        QString fileDir = "/home/pi/InDetail/screenshots/";
        QString fileName = QDate::currentDate().toString("dd.MM.yy") + "-" +
                QTime::currentTime().toString("hh.mm.ss");
        QString fileExtention = ".png";
        QFile file(fileDir + fileName + fileExtention);
        file.open(QIODevice::WriteOnly);
        pixmap.save(&file, "PNG");
        QMessageBox::information(
                    this,
                    tr(appName),
                    tr("Screenshot saved successfully.") );
    }
    else
    {
        QMessageBox::information(
                    this,
                    tr(appName),
                    tr("There is a problem while taking screenshot.") );
    }

}

void MainWindow::on_bStartMaintenance_clicked()
{
    if (ui->bStartMaintenance->isChecked())
    {
        bool ok;
        int i = QInputDialog::getInt(this, tr(appName),
                                     tr("Enter Password:"), 0, 0, 9999, 1, &ok);
        if (ok)
        {
            if (i == 1881)
            {
                ui->tabWidget->setTabEnabled(0, false);
                ui->tabWidget->setTabEnabled(1, false);
                ui->tabWidget->setTabEnabled(2, false);
                ui->tabWidget->setTabEnabled(3, false);
                ui->tabWidget->setTabEnabled(4, false);

                ui->bStartCooler->setEnabled(true);
                //  ui->bStartVibration->setEnabled(true);
                //  ui->dsbVibrationMaintenance->setEnabled(true);
                //  ui->bStartPressure->setEnabled(true);
                //  ui->dsbPressureMaintenance->setEnabled(true);
                ui->bRes->setEnabled(true);
                ui->chbRes1->setEnabled(true);
                ui->chbRes2->setEnabled(true);
                ui->chbRes3->setEnabled(true);
                ui->chbResTank->setEnabled(true);
                ui->bFan->setEnabled(true);
                ui->chbFan1->setEnabled(true);
                ui->chbFan2->setEnabled(true);
                ui->chbFan3->setEnabled(true);

                QByteArray cantTouchThis;
                cantTouchThis.clear();
                cantTouchThis.append(0x04);
                proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
            }
            else
            {
                QMessageBox::information(
                            this,
                            tr(appName),
                            tr("Wrong password is entered. No action will be processed.") );
                ui->bStartMaintenance->setChecked(false);
            }
        }
    }
    else
    {
        ui->tabWidget->setTabEnabled(0, true);
        ui->tabWidget->setTabEnabled(1, true);
        ui->tabWidget->setTabEnabled(2, true);
        ui->tabWidget->setTabEnabled(3, true);
        ui->tabWidget->setTabEnabled(4, true);

        ui->bStartCooler->setEnabled(false);
        // ui->bStartVibration->setEnabled(false);
        // ui->dsbVibrationMaintenance->setEnabled(false);
        // ui->bStartPressure->setEnabled(false);
        // ui->dsbPressureMaintenance->setEnabled(false);
        ui->bRes->setEnabled(false);
        ui->chbRes1->setEnabled(false);
        ui->chbRes2->setEnabled(false);
        ui->chbRes3->setEnabled(false);
        ui->chbResTank->setEnabled(false);
        ui->bFan->setEnabled(false);
        ui->chbFan1->setEnabled(false);
        ui->chbFan2->setEnabled(false);
        ui->chbFan3->setEnabled(false);

        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(0x04);
        proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
    }
}

void MainWindow::on_bRes_clicked()
{
    QByteArray cantTouchThis;
    cantTouchThis.clear();

    if (ui->chbRes1->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    if (ui->chbRes2->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    if (ui->chbRes3->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    if (ui->chbResTank->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    proc->insertCommandMessage(mySerial::makeMessage(0x97,cantTouchThis));
}

void MainWindow::on_bStartCooler_clicked()
{
    if (ui->bStartCooler->isChecked())
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(0x01);
        proc->insertCommandMessage(mySerial::makeMessage(0x98,cantTouchThis));
    }
    else
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(char(0x00));
        proc->insertCommandMessage(mySerial::makeMessage(0x98,cantTouchThis));
    }
}
/*
void MainWindow::on_bStartPressure_clicked()
{
    if (ui->bStartPressure->isChecked())
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(0x01);

        float pressureValue = ui->dsbPressureMaintenance->value() * 10.0;
        cantTouchThis.append(quint16(pressureValue) & 0x00FF);
        cantTouchThis.append(quint16(pressureValue) >> 8);

        proc->insertCommandMessage(mySerial::makeMessage(0x99,cantTouchThis));
    }
    else
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(char(0x00));

        float pressureValue = ui->dsbPressureMaintenance->value() * 10.0;
        cantTouchThis.append(quint16(pressureValue) & 0x00FF);
        cantTouchThis.append(quint16(pressureValue) >> 8);

        proc->insertCommandMessage(mySerial::makeMessage(0x99,cantTouchThis));
    }
}

void MainWindow::on_bStartVibration_clicked()
{
    if (ui->bStartVibration->isChecked())
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(0x01);

        float vibrationFreq = ui->dsbVibrationMaintenance->value() * 10.0;
        cantTouchThis.append(quint16(vibrationFreq) & 0x00FF);
        cantTouchThis.append(quint16(vibrationFreq) >> 8);

        proc->insertCommandMessage(mySerial::makeMessage(0x9A,cantTouchThis));
    }
    else
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(char(0x00));

        float vibrationFreq = ui->dsbVibrationMaintenance->value() * 10.0;
        cantTouchThis.append(quint16(vibrationFreq) & 0x00FF);
        cantTouchThis.append(quint16(vibrationFreq) >> 8);

        proc->insertCommandMessage(mySerial::makeMessage(0x9A,cantTouchThis));
    }
}
*/
void MainWindow::on_bFan_clicked()
{
    QByteArray cantTouchThis;
    cantTouchThis.clear();

    if (ui->chbFan1->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    if (ui->chbFan2->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    if (ui->chbFan3->isChecked())
    {
        cantTouchThis.append(0x01);
    }
    else
    {
        cantTouchThis.append(char(0x00));
    }

    proc->insertCommandMessage(mySerial::makeMessage(0x9B,cantTouchThis));
}

void MainWindow::on_bLightsMain_clicked()
{
    if (ui->bLightsMain->isChecked())
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(0x01);
        proc->insertCommandMessage(mySerial::makeMessage(0x9C,cantTouchThis));
    }
    else
    {
        QByteArray cantTouchThis;
        cantTouchThis.clear();
        cantTouchThis.append(char(0x00));
        proc->insertCommandMessage(mySerial::makeMessage(0x9C,cantTouchThis));
    }
}
/*
void MainWindow::on_cbVSelectSUnit_currentIndexChanged(int index)
{
    if (ui->cbVSelectSType->currentIndex() == 2)
    {
        ui->cbVSelectSUnit->setCurrentIndex(1);
    }
}
*/
/*
void MainWindow::on_cbVSelectSType_currentIndexChanged(int index)
{
    if (ui->cbVSelectSType->currentIndex() == 2)
    {
        ui->cbVSelectSUnit->setCurrentIndex(1);
    }
}
*/
void MainWindow::on_bClearLogTable_clicked()
{
    ui->warningTable->setRowCount(0);
}

void MainWindow::clearProfileSlot(char sType, char pType, quint8 index)
{
    if (sType == 's')
    {
        if (pType == 't')
        {
            tProfileSave[index].active = 0;
            tProfileSave[index].totalStep = 0;
            tProfileSave[index].totalTestCycle = 0;
            tProfileSave[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                tProfileSave[index].step[i].stepUnit = 0;
                tProfileSave[index].step[i].stepType = 0;
                tProfileSave[index].step[i].lDuration = 0;
                tProfileSave[index].step[i].lTarget = 0;
                tProfileSave[index].step[i].tRise = 0;
                tProfileSave[index].step[i].tUp = 0;
                tProfileSave[index].step[i].tFall = 0;
                tProfileSave[index].step[i].tDown = 0;
                tProfileSave[index].step[i].tLow = 0;
                tProfileSave[index].step[i].tHigh = 0;
                tProfileSave[index].step[i].sPeriod = 0;
                tProfileSave[index].step[i].sMean = 0;
                tProfileSave[index].step[i].sAmp = 0;
                tProfileSave[index].step[i].logRate = 0;
                tProfileSave[index].step[i].logMin = 0;
                tProfileSave[index].step[i].logMax = 0;
                tProfileSave[index].step[i].repeatUnit = 0;
                tProfileSave[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                tProfileSave[index].stepDuration[i] = 0;

        }
        else if (pType == 'p')
        {
            pProfileSave[index].active = 0;
            pProfileSave[index].totalStep = 0;
            pProfileSave[index].totalTestCycle = 0;
            pProfileSave[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                pProfileSave[index].step[i].stepUnit = 0;
                pProfileSave[index].step[i].stepType = 0;
                pProfileSave[index].step[i].lDuration = 0;
                pProfileSave[index].step[i].lTarget = 0;
                pProfileSave[index].step[i].tRise = 0;
                pProfileSave[index].step[i].tUp = 0;
                pProfileSave[index].step[i].tFall = 0;
                pProfileSave[index].step[i].tDown = 0;
                pProfileSave[index].step[i].tLow = 0;
                pProfileSave[index].step[i].tHigh = 0;
                pProfileSave[index].step[i].sPeriod = 0;
                pProfileSave[index].step[i].sMean = 0;
                pProfileSave[index].step[i].sAmp = 0;
                pProfileSave[index].step[i].logRate = 0;
                pProfileSave[index].step[i].logMin = 0;
                pProfileSave[index].step[i].logMax = 0;
                pProfileSave[index].step[i].repeatUnit = 0;
                pProfileSave[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                pProfileSave[index].stepDuration[i] = 0;
        }
        else if (pType == 'v')
        {
            vProfileSave[index].active = 0;
            vProfileSave[index].totalStep = 0;
            vProfileSave[index].totalTestCycle = 0;
            vProfileSave[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                vProfileSave[index].step[i].stepUnit = 0;
                vProfileSave[index].step[i].stepType = 0;
                vProfileSave[index].step[i].lDuration = 0;
                vProfileSave[index].step[i].lTarget = 0;
                vProfileSave[index].step[i].tRise = 0;
                vProfileSave[index].step[i].tUp = 0;
                vProfileSave[index].step[i].tFall = 0;
                vProfileSave[index].step[i].tDown = 0;
                vProfileSave[index].step[i].tLow = 0;
                vProfileSave[index].step[i].tHigh = 0;
                vProfileSave[index].step[i].sPeriod = 0;
                vProfileSave[index].step[i].sMean = 0;
                vProfileSave[index].step[i].sAmp = 0;
                vProfileSave[index].step[i].logRate = 0;
                vProfileSave[index].step[i].logMin = 0;
                vProfileSave[index].step[i].logMax = 0;
                vProfileSave[index].step[i].repeatUnit = 0;
                vProfileSave[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                vProfileSave[index].stepDuration[i] = 0;
        }
    }
    else if (sType == 'e')
    {
        if (pType == 't')
        {
            tProfileEdit[index].active = 0;
            tProfileEdit[index].totalStep = 0;
            tProfileEdit[index].totalTestCycle = 0;
            tProfileEdit[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                tProfileEdit[index].step[i].stepUnit = 0;
                tProfileEdit[index].step[i].stepType = 0;
                tProfileEdit[index].step[i].lDuration = 0;
                tProfileEdit[index].step[i].lTarget = 0;
                tProfileEdit[index].step[i].tRise = 0;
                tProfileEdit[index].step[i].tUp = 0;
                tProfileEdit[index].step[i].tFall = 0;
                tProfileEdit[index].step[i].tDown = 0;
                tProfileEdit[index].step[i].tLow = 0;
                tProfileEdit[index].step[i].tHigh = 0;
                tProfileEdit[index].step[i].sPeriod = 0;
                tProfileEdit[index].step[i].sMean = 0;
                tProfileEdit[index].step[i].sAmp = 0;
                tProfileEdit[index].step[i].logRate = 0;
                tProfileEdit[index].step[i].logMin = 0;
                tProfileEdit[index].step[i].logMax = 0;
                tProfileEdit[index].step[i].repeatUnit = 0;
                tProfileEdit[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                tProfileEdit[index].stepDuration[i] = 0;

        }
        else if (pType == 'p')
        {
            pProfileEdit[index].active = 0;
            pProfileEdit[index].totalStep = 0;
            pProfileEdit[index].totalTestCycle = 0;
            pProfileEdit[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                pProfileEdit[index].step[i].stepUnit = 0;
                pProfileEdit[index].step[i].stepType = 0;
                pProfileEdit[index].step[i].lDuration = 0;
                pProfileEdit[index].step[i].lTarget = 0;
                pProfileEdit[index].step[i].tRise = 0;
                pProfileEdit[index].step[i].tUp = 0;
                pProfileEdit[index].step[i].tFall = 0;
                pProfileEdit[index].step[i].tDown = 0;
                pProfileEdit[index].step[i].tLow = 0;
                pProfileEdit[index].step[i].tHigh = 0;
                pProfileEdit[index].step[i].sPeriod = 0;
                pProfileEdit[index].step[i].sMean = 0;
                pProfileEdit[index].step[i].sAmp = 0;
                pProfileEdit[index].step[i].logRate = 0;
                pProfileEdit[index].step[i].logMin = 0;
                pProfileEdit[index].step[i].logMax = 0;
                pProfileEdit[index].step[i].repeatUnit = 0;
                pProfileEdit[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                pProfileEdit[index].stepDuration[i] = 0;
        }
        else if (pType == 'v')
        {
            vProfileEdit[index].active = 0;
            vProfileEdit[index].totalStep = 0;
            vProfileEdit[index].totalTestCycle = 0;
            vProfileEdit[index].startValue = 0;
            for (int i=0; i<MaxStep; i++)
            {
                vProfileEdit[index].step[i].stepUnit = 0;
                vProfileEdit[index].step[i].stepType = 0;
                vProfileEdit[index].step[i].lDuration = 0;
                vProfileEdit[index].step[i].lTarget = 0;
                vProfileEdit[index].step[i].tRise = 0;
                vProfileEdit[index].step[i].tUp = 0;
                vProfileEdit[index].step[i].tFall = 0;
                vProfileEdit[index].step[i].tDown = 0;
                vProfileEdit[index].step[i].tLow = 0;
                vProfileEdit[index].step[i].tHigh = 0;
                vProfileEdit[index].step[i].sPeriod = 0;
                vProfileEdit[index].step[i].sMean = 0;
                vProfileEdit[index].step[i].sAmp = 0;
                vProfileEdit[index].step[i].logRate = 0;
                vProfileEdit[index].step[i].logMin = 0;
                vProfileEdit[index].step[i].logMax = 0;
                vProfileEdit[index].step[i].repeatUnit = 0;
                vProfileEdit[index].step[i].repeatDuration = 0;
            }
            for (int i=0; i<MaxStep; i++)
                vProfileEdit[index].stepDuration[i] = 0;
        }
    }
}

void MainWindow::on_cbSelectMethodManual_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->dsbTTimeSetManual->setValue(0);
        //    ui->dsbPTimeSetManual->setValue(0);
        //    ui->dsbVTimeSetManual->setValue(0);
        ui->sbTStepSetManual->setValue(1);
        //    ui->sbPStepSetManual->setValue(1);
        //    ui->sbVStepSetManual->setValue(1);
        ui->sbTStepRepeatSetManual->setValue(0);
        //    ui->sbPStepRepeatSetManual->setValue(0);
        //    ui->sbVStepRepeatSetManual->setValue(0);
        ui->sbTCycleSetManual->setValue(0);
        //    ui->sbPCycleSetManual->setValue(0);
        //    ui->sbVCycleSetManual->setValue(0);

        ui->dsbTTimeSetManual->setEnabled(false);
        //    ui->dsbPTimeSetManual->setEnabled(false);
        //    ui->dsbVTimeSetManual->setEnabled(false);
        ui->sbTStepSetManual->setEnabled(false);
        //    ui->sbPStepSetManual->setEnabled(false);
        //    ui->sbVStepSetManual->setEnabled(false);
        ui->sbTStepRepeatSetManual->setEnabled(false);
        //    ui->sbPStepRepeatSetManual->setEnabled(false);
        //    ui->sbVStepRepeatSetManual->setEnabled(false);
        ui->sbTCycleSetManual->setEnabled(false);
        //    ui->sbPCycleSetManual->setEnabled(false);
        //    ui->sbVCycleSetManual->setEnabled(false);
    }
    else if (index == 1)
    {
        ui->dsbTTimeSetManual->setValue(0);
        //     ui->dsbPTimeSetManual->setValue(0);
        //     ui->dsbVTimeSetManual->setValue(0);
        ui->sbTStepSetManual->setValue(0);
        //     ui->sbPStepSetManual->setValue(0);
        //     ui->sbVStepSetManual->setValue(0);
        ui->sbTStepRepeatSetManual->setValue(1);
        //     ui->sbPStepRepeatSetManual->setValue(1);
        //     ui->sbVStepRepeatSetManual->setValue(1);
        ui->sbTCycleSetManual->setValue(0);
        //     ui->sbPCycleSetManual->setValue(0);
        //     ui->sbVCycleSetManual->setValue(0);
        ui->sbTStepSetManual->setEnabled(false);
        //     ui->sbPStepSetManual->setEnabled(false);
        //     ui->sbVStepSetManual->setEnabled(false);
        ui->sbTStepRepeatSetManual->setEnabled(false);
        //     ui->sbPStepRepeatSetManual->setEnabled(false);
        //     ui->sbVStepRepeatSetManual->setEnabled(false);

        quint8 profileNumber = ui->cbSelectProfileManual->currentIndex() - 1;

        if (tProfileLoad[profileNumber].active)
        {
            ui->dsbTTimeSetManual->setEnabled(true);
            ui->sbTCycleSetManual->setEnabled(true);
        }
        else
        {
            ui->dsbTTimeSetManual->setEnabled(false);
            ui->sbTCycleSetManual->setEnabled(false);
        }
        if (pProfileLoad[profileNumber].active)
        {
            //        ui->dsbPTimeSetManual->setEnabled(true);
            //        ui->sbPCycleSetManual->setEnabled(true);
        }
        else
        {
            //        ui->dsbPTimeSetManual->setEnabled(false);
            //        ui->sbPCycleSetManual->setEnabled(false);
        }
        /*    if (vProfileLoad[profileNumber].active)
        {
    //        ui->dsbVTimeSetManual->setEnabled(true);
    //        ui->sbVCycleSetManual->setEnabled(true);
        }
        else
        {
            ui->dsbVTimeSetManual->setEnabled(false);
            ui->sbVCycleSetManual->setEnabled(false);
        }
        */
        // there could be a code which checks the total profile seconds and limit the max value of
        // elapsed seconds spinbox
    }
    else if (index == 2)
    {
        ui->dsbTTimeSetManual->setValue(0);
        //  ui->dsbPTimeSetManual->setValue(0);
        //  ui->dsbVTimeSetManual->setValue(0);
        ui->sbTStepSetManual->setValue(0);
        //  ui->sbPStepSetManual->setValue(0);
        //  ui->sbVStepSetManual->setValue(0);
        ui->sbTStepRepeatSetManual->setValue(1);
        //  ui->sbPStepRepeatSetManual->setValue(1);
        //  ui->sbVStepRepeatSetManual->setValue(1);
        ui->sbTCycleSetManual->setValue(0);
        //  ui->sbPCycleSetManual->setValue(0);
        //  ui->sbVCycleSetManual->setValue(0);

        ui->dsbTTimeSetManual->setEnabled(false);
        //  ui->dsbPTimeSetManual->setEnabled(false);
        //  ui->dsbVTimeSetManual->setEnabled(false);

        quint8 profileNumber = ui->cbSelectProfileManual->currentIndex() - 1;
        if (tProfileLoad[profileNumber].active)
        {
            ui->sbTStepSetManual->setEnabled(true);
            ui->sbTStepRepeatSetManual->setEnabled(true);
            ui->sbTCycleSetManual->setEnabled(true);
        }
        else
        {
            ui->sbTStepSetManual->setEnabled(false);
            ui->sbTStepRepeatSetManual->setEnabled(false);
            ui->sbTCycleSetManual->setEnabled(false);
        }
        /*   if (pProfileLoad[profileNumber].active)
        {
            ui->sbPStepSetManual->setEnabled(true);
            ui->sbPStepRepeatSetManual->setEnabled(true);
            ui->sbPCycleSetManual->setEnabled(true);
        }
        else
        {
            ui->sbPStepSetManual->setEnabled(false);
            ui->sbPStepRepeatSetManual->setEnabled(false);
            ui->sbPCycleSetManual->setEnabled(false);
        }
        */
        /*
        if (vProfileLoad[profileNumber].active)
        {
            ui->sbVStepSetManual->setEnabled(true);
            ui->sbVStepRepeatSetManual->setEnabled(true);
            ui->sbVCycleSetManual->setEnabled(true);
        }
        else
        {
            ui->sbVStepSetManual->setEnabled(false);
            ui->sbVStepRepeatSetManual->setEnabled(false);
            ui->sbVCycleSetManual->setEnabled(false);
        }
        */
    }
}

void MainWindow::on_cbSelectProfileManual_currentIndexChanged(int index)
{
    ui->bStartTestManual->setEnabled(false);
    ui->cbSelectProfileMain->setCurrentIndex(0);
    ui->sbTTotalCycleManual->setValue(1);
    //  ui->sbPTotalCycleManual->setValue(1);
    //  ui->sbVTotalCycleManual->setValue(1);
    ui->laTTotalCycleMain->setText(QString::number(1));


    if (index == 0)
    {
        ui->laSelectedProfileMain->setText("No Profile Selected");
        ui->laSelectedProfileManual->setText("No Profile Selected");
        ui->bSendProfileManual->setEnabled(false);
        ui->cbSelectMethodManual->setCurrentIndex(0);
        ui->cbSelectMethodManual->setEnabled(false);
        ui->laTTotalCycleMain->setText(QString::number(1));

    }
    else
    {
        ui->cbSelectMethodManual->setCurrentIndex(0);
        ui->cbSelectMethodManual->setEnabled(true);

        quint8 index = ui->cbSelectProfileManual->currentIndex();
        if (readProfiles('l', index))
        {
            if (tProfileLoad[index-1].active == 0)
            {
                ui->sbTTotalCycleManual->setValue(0);
                ui->sbTTotalCycleManual->setEnabled(false);
            }
            else
            {
                ui->sbTTotalCycleManual->setEnabled(true);
            }
            /*
            if (pProfileLoad[index-1].active == 0)
            {
                ui->sbPTotalCycleManual->setValue(0);
                ui->sbPTotalCycleManual->setEnabled(false);
            }
            else
            {
                ui->sbPTotalCycleManual->setEnabled(true);
            }
*/
            /*
            if (vProfileLoad[index-1].active == 0)
            {
                ui->sbVTotalCycleManual->setValue(0);
                ui->sbVTotalCycleManual->setEnabled(false);
                ui->chbEllipticalVibrationSetManual->setChecked(false);
                ui->chbEllipticalVibrationSetManual->setEnabled(false);
            }
            else
            {
                ui->sbVTotalCycleManual->setEnabled(true);
                ui->chbEllipticalVibrationSetManual->setChecked(false);
                ui->chbEllipticalVibrationSetManual->setEnabled(true);
            }
            */
        }

        ui->bSendProfileManual->setEnabled(true);
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 0)
    {

    }
    else if (index == 1)
    {
        ui->cbSelectGraph->setCurrentIndex(0);
        qApp->processEvents();
    }
    else if (index == 2)
    {

    }
    else if (index == 3)
    {

    }
    else if (index == 4)
    {

    }
    else if (index == 5)
    {

    }
}

void MainWindow::on_bSendProfileMain_clicked()
{
    quint8 index = ui->cbSelectProfileMain->currentIndex();
    if ((tProfileLoad[index-1].active == 0) && (pProfileLoad[index-1].active == 0) && (vProfileLoad[index-1].active == 0))
    {
        QMessageBox::information(
                    this,
                    tr(appName),
                    tr("This profile does not contain any step in temperature, pressure or vibration.") );
    }
    else
    {
        sendProfileOverSerial("main",index);
    }
}

void MainWindow::on_bStartTestManual_clicked()
{
    proc->stop();

    if (myPLC.deviceState == char(0x03))
    {

    }
    else
    {
        tKey = 0;
        pKey = 0;
        vKey = 0;

        //    ui->tTestGraph->graph(0)->data().clear();
        //    ui->pTestGraph->graph(0)->data().clear();
        //    ui->pTestGraph->graph(1)->data().clear();
        //    ui->pTestGraph->graph(2)->data().clear();
        //    ui->pTestGraph->graph(3)->data().clear();
        //    ui->pTestGraph->graph(4)->data().clear();
        //    ui->pTestGraph->graph(5)->data().clear();
        //    ui->vTestGraph->graph(0)->data().clear();
        //    ui->tTestGraph->replot();
        //    ui->pTestGraph->replot();
        //    ui->vTestGraph->replot();

        ui->tTestGraph->clearPlottables();
        ui->pTestGraph->clearPlottables();
        //ui->vTestGraph->clearPlottables();

        setupTGraph();
        setupPGraphs();
        setupVGraph();
    }

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x02);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
}

void MainWindow::on_bStopTestManual_clicked()
{
    timerTemp->stop();
    //    timerVib->stop();
    timerPressure->stop();

    proc->stop();

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x01);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
}

void MainWindow::on_bSendProfileManual_clicked()
{
    quint8 index = ui->cbSelectProfileManual->currentIndex();
    if ((tProfileLoad[index-1].active == 0) && (pProfileLoad[index-1].active == 0) && (vProfileLoad[index-1].active == 0))
    {
        QMessageBox::information(
                    this,
                    tr(appName),
                    tr("This profile does not contain any step in temperature, pressure or vibration.") );
    }
    else
    {
        sendProfileOverSerial("manual",index);
    }
}

void MainWindow::on_bPauseTestManual_clicked()
{
    timerTemp->stop();
    //   timerVib->stop();
    timerPressure->stop();

    proc->stop();

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x03);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
}

void MainWindow::on_bPauseTest_clicked()
{
    timerTemp->stop();
    //   timerVib->stop();
    timerPressure->stop();

    proc->stop();

    QByteArray cantTouchThis;
    cantTouchThis.clear();
    cantTouchThis.append(0x03);
    proc->insertCommandMessage(mySerial::makeMessage(0x0A,cantTouchThis));
}

void MainWindow::mousePress()
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (ui->tTestGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->tTestGraph->axisRect()->setRangeDrag(ui->tTestGraph->xAxis->orientation());
    else if (ui->tTestGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->tTestGraph->axisRect()->setRangeDrag(ui->tTestGraph->yAxis->orientation());
    else
        ui->tTestGraph->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (ui->tTestGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->tTestGraph->axisRect()->setRangeZoom(ui->tTestGraph->xAxis->orientation());
    else if (ui->tTestGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->tTestGraph->axisRect()->setRangeZoom(ui->tTestGraph->yAxis->orientation());
    else
        ui->tTestGraph->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::setupComboBoxes()
{
    for (int i = 0; i <= 20; i++)
    {
       if( readProfiles('g', i))
       {

       }
    }
}


