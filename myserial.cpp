#include "myserial.h"

mySerial::mySerial(QObject *parent) : QObject(parent)
{

}

static unsigned short crc_table [256] = {

    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
    0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
    0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
    0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
    0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
    0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
    0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
    0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
    0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
    0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
    0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
    0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
    0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
    0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
    0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
    0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
    0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
    0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
    0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
    0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
    0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
    0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
    0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
    0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
    0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
    0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
    0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

QByteArray rawSerial;
QByteArray dataSerial;
uint commandSerial;
uint state = 0;
int dataLengthSerial = 0;

void mySerial::startSerial()
{
    serial = new QSerialPort(this);

    #ifdef Q_OS_LINUX
        //linux code goes here
        serial->setPortName("/dev/ttyUSB0");
    #endif
    #ifdef Q_OS_WIN
        // windows code goes here
        serial->setPortName("COM51");
    #endif

    serial->setBaudRate(QSerialPort::Baud57600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if(serial->open(QIODevice::ReadWrite))
    {
        qDebug() << "Connected to " + serial->portName() + ": " + QString::number(serial->baudRate()) + "bps, 8Data, NoParity, 1Stop, NoFlow";
    }
    else
    {
        qDebug() << "Serial Start Error: " << serial->errorString();
    }

    connect(serial, &QSerialPort::readyRead, this, &mySerial::readSerial);
    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &mySerial::errorSerial);
}

void mySerial::stopSerial()
{
    if (serial->isOpen())
    {
        serial->close();
    }
}

QByteArray mySerial::makeMessage(quint8 command, QByteArray data)
{
    QByteArray cantTouchThis;
    //crc hesaplanacak data
    cantTouchThis.append(command);
    cantTouchThis.append(data.length());
    cantTouchThis.append(data);

    cantTouchThis.append(CalculateCRC16(cantTouchThis));
    cantTouchThis.append(0x55);
    cantTouchThis.prepend(0x10); //direction
    cantTouchThis.prepend(0xAA); //header
    return cantTouchThis;
}

void mySerial::readSerial()
{
    QByteArray data = serial->readAll();
    qDebug() << "Incoming Raw Data Packet:";
    qDebug() << "Start";
    qDebug() << QString(data.toHex());
    qDebug() << "End";
    parserSerial(data);
}

void mySerial::parserSerial(QByteArray data)
{
    rawSerial.append(data);

    int length = rawSerial.length();
    //qDebug() << "Current Processed Raw Data Length: " << length;

    switch (state)
    {
    case(0):

        if (length >0)
        {
            if (rawSerial.at(0) == char(0xAA))
            {
                state++;
                dataSerial.clear();
                qDebug() << "Head (AA) check is OK.";
            }
            else
            {
                state=0;
                rawSerial.clear();

                qDebug() << "Head (AA) check is NO GO. Starting over...";
                break;
            }
        }

    case(1):

        if (length > 1)
        {
            if (rawSerial.at(1) == char(0x01))
            {
                state++;
                //qDebug() << "Direction (01) check is OK.";
            }
            else
            {
                state=0;
                rawSerial.clear();
                //qDebug() << "Direction (01) check is NO GO. Starting over...";
                break;
            }
        }

    case(2):

        if (length > 2)
        {
            commandSerial = rawSerial[2];
            state++;
            qDebug() << "Command Extracted: " << QString::number(commandSerial, 16);
        }

    case(3):

        if (length > 3)
        {
            dataLengthSerial = rawSerial[3];
            state++;
            //qDebug() << "DataLength Extracted: " << QString::number(dataLengthSerial, 16);
        }

    case(4):

        if (length > dataLengthSerial + 3)
        {

            for(int i=0; i< dataLengthSerial; i++)
            {
                dataSerial[i] = rawSerial[i+4];
                //qDebug().nospace() << "Extracted Data[" << i << "] : " << QString::number(dataSerial[i], 16);
            }
            state++;
        }

    case(5):

        if (length > dataLengthSerial + 5)
        {
            QByteArray incomingCRC;
            incomingCRC[0] = rawSerial[dataLengthSerial + 4];
            incomingCRC[1] = rawSerial[dataLengthSerial + 5];
            //qDebug() << "Extracted CRC: " << QString::number(incomingCRC[0], 16)
            //        << QString::number(incomingCRC[1], 16);

            unsigned char feedCRC[dataLengthSerial+2];
            feedCRC[0]=commandSerial;
            feedCRC[1]=dataLengthSerial;
            for(int i=0; i<dataLengthSerial; i++)
            {
                feedCRC[i+2] = dataSerial[i];
            }

            //qDebug() << "feedCRC:";
            //qDebug() << "Start";
            for (int i=0; i<dataLengthSerial+2; i++)
            {
                //qDebug() << QString::number(feedCRC[i], 16);
            }
            //qDebug() << "End";

            QByteArray calculatedCRC;
            unsigned short crc;
            crc = CalculateCRC16(feedCRC, dataLengthSerial + 2, 0xffff, 0);
            calculatedCRC[1] = crc >> 8;
            calculatedCRC[0] = crc & 0x00FF;

            //qDebug() << "Calculated CRC: " << QString::number(calculatedCRC[0], 16)
            //        << QString::number(calculatedCRC[1], 16);

            if (incomingCRC == calculatedCRC)
            {
                state++;
                //qDebug() << "CRC compare OK. ";
            }
            else
            {
                state=0;
                rawSerial.clear();
                qDebug() << "CRC compare NO GO. Starting over...";
                break;
            }
        }

    case(6):

        if (length > dataLengthSerial + 6)
        {
            if (rawSerial.at(dataLengthSerial + 6) == 0x55)
            {
                state=0;
                //qDebug() << "A full message received.";
                messageSerial(commandSerial, dataSerial);
                rawSerial.clear();
                //rawSerial.remove(0, dataLengthSerial + 7);
            }
            else
            {
                state=0;
                rawSerial.clear();
                qDebug() << "Tail (55) check is NO GO. Starting over...";
                break;
            }
        }
    }
}

unsigned short mySerial::CalculateCRC16(unsigned char *data, unsigned short length, unsigned short seed, unsigned short final)
{

    unsigned short count;
    unsigned int crc = seed;
    unsigned int temp;

    for (count = 0; count < length; ++count)
    {
        temp = (*data++ ^ (crc >> 8)) & 0xff;
        crc = crc_table[temp] ^ (crc << 8);
    }

    return (unsigned short)(crc ^ final);
}
QByteArray mySerial::CalculateCRC16(QByteArray data)
{

    unsigned short count;
    unsigned int crc = 0xFFFF;
    unsigned int final = 0x0000;

    unsigned int temp;

    for (count = 0; count < data.length(); ++count)
    {
        temp = (data.at(count) ^ (crc >> 8)) & 0xff;
        crc = crc_table[temp] ^ (crc << 8);
    }

    QByteArray qCrc;
    qCrc.append((unsigned short)(crc ^ final) & 0x00FF);
    qCrc.append((unsigned short)(crc ^ final) >> 8);
    return qCrc;
}
bool mySerial::writeSerial(const QByteArray &data)
{
    if (serial->write(data))
    {

        qDebug() << "SND: "<<QTime::currentTime().toString("hh.mm.ss.zzz")<< QString(data.toHex());
        return true;
    }
    else
    {
        return false;
    }
}

void mySerial::errorSerial(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        qDebug() << "Serial Error: " << serial->errorString();
        stopSerial();
    }
}
