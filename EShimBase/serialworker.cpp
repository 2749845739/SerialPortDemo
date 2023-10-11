/** Copyright (C) USTC BMEC RFLab and Fuqing Medical Cooperation - All Rights Reserved.
** Unauthorized copying of this file, via any medium is strictly prohibited.
** Proprietary and confidential.
**/
#include "serialworker.h"
#include <QSerialPort>
#include <QTimer>
#include "hwtool.hxx"

SerialWorker::SerialWorker(QObject *parent) : QObject{parent},
    m_enabled(false), m_connected(false), m_serialPort(nullptr)
{
}

void SerialWorker::sendData(const QByteArray &data, const bool flush)
{
    if(!getEnabled() || !getConnected() || !m_serialPort) return;
    const qint64 bytesWritten = m_serialPort->write(data);
    m_serialPort->waitForBytesWritten();
    if(bytesWritten == -1){
        auto message = QObject::tr("Failed to write the data to port %1, error: %2")
                     .arg(m_serialPort->portName(), m_serialPort->errorString());
        qDebug() << "message: " << message;
        setLastError(message);
    }else if(bytesWritten != data.size()){
        auto message = QObject::tr("Failed to write all the data to port %1, error: %2")
                            .arg(m_serialPort->portName(), m_serialPort->errorString());
        qDebug() << "message: " << message;
        setLastError(message);
    }
    if(flush){
        m_serialPort->flush();
    }
}

QString SerialWorker::parseErrorCode(quint8 error)
{
    QString message = HwTool::serialErrorToStr(error);
    if(m_serialPort){
        message = message +" "+ QObject::tr("From port %1, error: %2. %3")
                .arg(m_serialPort->portName(), m_serialPort->errorString(),
                     QTime::currentTime().toString("hh:mm:ss:zzz"));
    }
    return message;
}

void SerialWorker::init()
{
    m_serialPort = new QSerialPort;
    connect(m_serialPort, &QSerialPort::errorOccurred, this,
            &SerialWorker::parseErrorCode, Qt::QueuedConnection);
    /// \note QueuedConnection is used here instead of lambda expressions
    connect(m_serialPort, &QSerialPort::readyRead, this,
            &SerialWorker::readData, Qt::QueuedConnection);
    open();
}

void SerialWorker::close()
{
    if(m_serialPort && m_serialPort->isOpen()){
        m_serialPort->close();
    }
    setConnected(false);
}
