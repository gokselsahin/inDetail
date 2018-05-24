#ifndef MYSERIAL_H
#define MYSERIAL_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QDebug>
#include <QTimer>
#include <QTime>
class mySerial : public QObject
{
    Q_OBJECT

public:
    explicit mySerial(QObject *parent = 0);

    void parserSerial(QByteArray data);
    unsigned short CalculateCRC16(unsigned char *data, unsigned short length, unsigned short seed, unsigned short final);
    static QByteArray CalculateCRC16(QByteArray data);
    //bool writeSerial(const QByteArray &data);
    void startSerial();
    void stopSerial();
    static QByteArray makeMessage(quint8 command, QByteArray data);
private slots:
    void readSerial();
    void errorSerial(QSerialPort::SerialPortError error);

private:
    QSerialPort *serial;


signals:
    void messageSerial (uint command, QByteArray data);

public slots:
    bool writeSerial(const QByteArray &data);
};

#endif // MYSERIAL_H
