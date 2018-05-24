#ifndef PROFILESTRUCT_H
#define PROFILESTRUCT_H
#include "QtCore"

#define MaxProfile 20
#define MaxStep 20

struct stepStruct
{
    quint8 stepType;
    quint8 stepUnit;
    double lDuration;
    double lTarget;
    double logRate;
    double logMin;
    double logMax;
    double tRise;
    double tUp;
    double tFall;
    double tDown;
    double tLow;
    double tHigh;
    double sPeriod;
    double sMean;
    double sAmp;
    quint8 repeatUnit;
    quint32 repeatDuration;
};

struct profileStruct
{
    quint8 active;
    double startValue;
    quint8 totalStep;
    quint16 totalTestCycle;
    double stepDuration[MaxStep];
    stepStruct step[MaxStep];
};

struct PLC
{
    quint8 deviceState;

    bool temperaturePrepActive;
    bool temperatureTestActive;
    bool temperatureTestCompleted;

    bool pressurePrepActive;
    bool pressureTestActive;
    bool pressureTestCompleted;

    bool vibrationPrepActive;
    bool vibrationTestActive;
    bool vibrationTestCompleted;

    bool profileActive;

    bool cabinTemperatureStat;
    bool tankTemperatureStat;

    bool pipePressureStat;

    bool vibrationMotor1Stat;
    bool vibrationMotor2Stat;

    bool commStat;
};

#endif // PROFILESTRUCT_H
