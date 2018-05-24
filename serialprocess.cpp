#include "serialprocess.h"
// Profil gönderimi
// Periyodik sorgu
// Rastgele komut başlat - durdur

SerialProcess::SerialProcess(QObject *parent) : QObject(parent)
{
    state = sIdle;
    mState = tic;
    reply = 0;
    pCounter = 0;
    cCounter = 0;
    qCounter = 0;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &SerialProcess::processHandler);
    timer->start();
}

SerialProcess::~SerialProcess()
{
    timer->stop();
    timer->deleteLater();
}

void SerialProcess::insertProfileMessage(QByteArray m)
{
    profileMessages.append(m);
}

void SerialProcess::insertCommandMessage(QByteArray m)
{
    qDebug()<<"Command message inserted";
    commandMessages.append(m);
}

void SerialProcess::setPeriodicMessage(QByteArray m)
{
    querryMessages = m;
}
void SerialProcess::start()
{
    state = sActive;
}
void SerialProcess::stop()
{
    state = sCancelled;
}
void SerialProcess::setProfile()
{
    state = sProfileSend;
    qDebug()<<"Profile transfer started";
}

void SerialProcess::processHandler()
{
    switch(state)
    {
    case sIdle:
        break;
    case sProfileSend:
        profileMessageHandler();
        break;
    case sActive:
        if(mState == toc)
            querryMessageHandler();
        break;
    default:
        break;
    }

    if(mState == tic)
    {
        commandMessageHandler();
    }
    if(mState == toc)   mState = tic;
    else                mState = toc;
}

void SerialProcess::PLCReply(quint8 command)
{
    emit commStatus(true);
    if((command >= 0x64) && (command <= 0x6e) )
    {
        pCounter = 0;
        if(state == sProfileSend)
        {
            if(profileMessages.length() > 0)
            {
                profileMessages.takeFirst();
                reply &= 0x06;
            }

            if(profileMessages.length()  == 0)
            {
                qDebug()<<"Profile transfer completed";
                state = sIdle;
                emit profileReady();
            }

        }
    }
    //if(command == 0x32 || command == 0x33)
    if (command == 0x32)
    {
        qCounter = 0;
        reply &= 0x5;
    }
    // if(command == 0x0A || command == 0x0C || command == 0x0D || command == 0x0E)
    if(command == 0x0A || command == 0x0C || command == 0x0D || command == 0x0E || command == 0x33)
    {
        cCounter = 0;
        if(commandMessages.length() > 0)
        {
            commandMessages.takeFirst();
            reply &= 0x03;
        }

        if(commandMessages.length()  == 0)
        {
            qDebug()<<"Command Sent";
        }

    }

    if(command >= 0x96 && command <=0xB4)
    {
        if(commandMessages.length() > 0)
        {
            commandMessages.takeFirst();
            reply &= 0x03;
        }

        if(commandMessages.length()  == 0)
        {
            qDebug()<<"Command Sent";
        }

    }

    //profil mesajı 64, 65, 66, 67,68,69,6a,6b,6c
    //Querry - 32, not right now but maybe (33)
    //Start stop komut 0a,0b - 0x30

}
void SerialProcess::profileMessageHandler()
{
    if(reply & waitProfile)
    {
        qDebug()<<"Profile packet loss, re-sending";
        pCounter++;
        responseHandler();
    }
    if(profileMessages.length() > 0)
    {
        write(profileMessages.first());
        reply |= waitProfile;
    }
    else
    {
        //
    }
}
void SerialProcess::commandMessageHandler()
{
    if((reply & waitCommand) && commandMessages.length() > 0)
    {
        qDebug()<<"Command packet loss, re-sending";
        cCounter++;
        responseHandler();
    }
    if(commandMessages.length() > 0)
    {
        write(commandMessages.first());
        reply |= waitCommand;
    }
    else
    {
        //
    }
}
void SerialProcess::querryMessageHandler()
{
    if(reply & waitPeriodic)
    {
        qDebug()<<"Querry packet lost";
        qCounter++;
        responseHandler();
    }
    write(querryMessages);
    reply |= waitPeriodic;
}

void SerialProcess::responseHandler()
{
    if (pCounter == 50)
    {
        pCounter = 0;
        emit commStatus(false);
    }
    if (cCounter == 20)
    {
        cCounter = 0;
        emit commStatus(false);
    }
    if (qCounter == 10)
    {
        qCounter = 0;
        emit commStatus(false);
    }
}

