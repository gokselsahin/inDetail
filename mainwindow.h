#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QValidator>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <QValidator>
#include <QtSerialPort/QSerialPort>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTime>
#include <QPixmap>
#include <QCloseEvent>
#include <QApplication>

#include "profilestruct.h"
#include "serialprocess.h"
#include "myserial.h"

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



private slots:

    void on_cbSelectGraph_currentIndexChanged(int index);
    void serialMessage(uint command, QByteArray data);
    void on_cbSelectProfile_currentIndexChanged(int index);
    void on_bEditPro_clicked();
    void on_bClearPro_clicked();
    void on_bSavePro_clicked();
    void updateTPreview();
    void on_bNewTStep_clicked();
    void on_bTForward2_clicked();
    void on_bTBack2_clicked();
    void on_bTBack3_clicked();
    void on_bTSaveStep_clicked();

    bool readProfiles(char rType, int index);
    void on_cbSelectProfileEdit_currentIndexChanged(int index);
    void on_cbSelectStepEdit_currentIndexChanged(int index);
    void on_cbSelectPTypeEdit_currentIndexChanged(int index);
    void on_cbSelectProfileMain_currentIndexChanged(int index);
    bool sendProfileOverSerial(QString mode, int index);
    void on_bStartTest_clicked();
    void on_bStopTest_clicked();
    void askSensorValues();
    void askOtherStuff();
    void getCurrentDateTime();
    void prepareTestTimers();
    void updateInfo(quint8 index, QByteArray data);
    void writeToLogTable(QString info);
    void commInfo(bool status);

    void updateTPlot();
    void updatePPlots();
 //   void updateVPlot();

    void profileSent();

    void setupTGraph();
    void setupPGraphs();
    void setupVGraph();

    void setupPreviewGraphs();

    void setupVisuals();
    void on_bScreenshot_clicked();

    void on_bStartCooler_clicked();
  //  void on_bStartVibration_clicked();
  //  void on_bStartPressure_clicked();
    void on_bRes_clicked();
    void on_bFan_clicked();
    void on_bLightsMain_clicked();


    void closeEvent(QCloseEvent *event);
    void on_cbVSelectSType_currentIndexChanged(int index);
    void on_bClearLogTable_clicked();
    void clearProfileSlot(char sType, char pType, quint8 index);
    void on_cbSelectMethodManual_currentIndexChanged(int index);
    void on_cbSelectProfileManual_currentIndexChanged(int index);
    void on_tabWidget_currentChanged(int index);
    void on_cbSelectSTypeEdit_currentIndexChanged(int index);
    void on_cbSelectSUnitEdit_currentIndexChanged(int index);
    void on_bSendProfileMain_clicked();
    void on_bStartTestManual_clicked();
    void on_bStopTestManual_clicked();
    void on_bSendProfileManual_clicked();
    void on_bPauseTestManual_clicked();
    void on_bPauseTest_clicked();
    void on_bStartMaintenance_clicked();
    void mousePress();
    void mouseWheel();
    void setupComboBoxes();

    void on_cbSelectProfileMain_2_currentIndexChanged(int index);

    void on_bStartTest_3_pressed();

    void on_ZoomInHor_pressed();

    void on_ZoomInHor_clicked();

    void on_ZoomOutHor_clicked();

    void on_ZoomInVer_clicked();

    void on_ZoomOutVer_clicked();

    bool on_bTemperatureSet_clicked();

    void on_bSetTemperatureStart_clicked();

    void on_bSetTemperatureStop_clicked();

private:
    Ui::MainWindow *ui;

    mySerial *serial;
    QTimer *timer1000, *timer250, *timerTemp, *timerVib, *timerPressure;
    SerialProcess *proc;
};

#endif // MAINWINDOW_H
