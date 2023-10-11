/** Copyright (C) USTC BMEC RFLab and Fuqing Medical Cooperation - All Rights Reserved.
** Unauthorized copying of this file, via any medium is strictly prohibited.
** Proprietary and confidential.
**/
#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include "propSetGet.h"

class QSerialPort;
class SerialWorker : public QObject
{
    Q_OBJECT
    LAB_PROP_RS(enabled, bool, Enabled);
    LAB_PROP_RS(connected, bool, Connected);
    LAB_PROP_RS(lastError, QString, LastError);

public:
    explicit SerialWorker(QObject *parent = nullptr);
protected:
    /// send raw data to serial port directly
    void sendData(const QByteArray &data, const bool flush = false);
protected:
    QSerialPort *m_serialPort;
    QByteArray m_buffer;

Q_SIGNALS:
    LAB_PROP_SIGNAL(enabled, bool)
    LAB_PROP_SIGNAL(connected, bool)
    LAB_PROP_SIGNAL(lastError, QString);

protected Q_SLOTS:
    /// when serial port object emits error code
    QString parseErrorCode(quint8 error);
    /// when serial port is ready to read
    virtual void readData() = 0;
public Q_SLOTS:
    virtual void init();
    /// read config file and open serial port.
    virtual void open(const bool closeAndOpen = true) = 0;
    /// close serial port and update related status members.
    virtual void close();
};

#endif // SERIALWORKER_H
