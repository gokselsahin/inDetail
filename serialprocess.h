#ifndef SERIALPROCESS_H
#define SERIALPROCESS_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "commands.h"

class SerialProcess : public QObject
{
    Q_OBJECT
public:

    QList<QByteArray> profileMessages;
    QList<QByteArray> commandMessages;
    QByteArray  querryMessages;

    enum testState
    {
        sIdle,
        sProfileSend,
        sActive,
        sCompleted,
        sCancelled,
        sPaused
    };

    testState state;

    enum messageState
    {
        tic,//PLC status Messages are querried
        toc //Random commands(start-stop) are sent
    };
    enum replyState
    {
        waitIdle = 0x0,
        waitProfile = 0x1,
        waitPeriodic = 0x2,
        waitCommand = 0x4
    };
    explicit SerialProcess(QObject *parent = 0);
    ~SerialProcess();

    void insertProfileMessage(QByteArray m);
    void insertCommandMessage(QByteArray m);
    void setPeriodicMessage(QByteArray m);
    void start();
    void stop();
    void setProfile();
    void responseHandler();
signals:
    void write(QByteArray data);
    void profileReady();
    void commStatus(bool status);
    void testStatus(SerialProcess::testState s);
public slots:
    void PLCReply(quint8 command);
private slots:
    void processHandler();
private:
    QTimer *timer;
    messageState mState;
    quint32 reply;
    quint16 pCounter;
    quint16 cCounter;
    quint16 qCounter;

    void profileMessageHandler();
    void querryMessageHandler();
    void commandMessageHandler();
};

#endif // SERIALPROCESS_H
