/** Copyright (C) Fuqing Medical and USTC BMEC RFLab - All Rights Reserved.
 ** Unauthorized copying of this file, via any medium is strictly prohibited.
 ** Proprietary and confidential.
 ** Created on 20230805, by yue.wang.
 **/

#include "eshimworker.h"
#include <QMutexLocker>
#include "hwtool.hxx"
#include "LabTool.h"
const int DATA_LENGTH = 8;
const int CHANNEL_NUMBER = 6;
const int SEND_INTERVAL = 200;
const char DEFAULT_DATA = 0x00;

EShimWorker::EShimWorker(QObject *parent)
    : SerialWorker{parent}, m_busVoltage(0), m_maxOutVoltageLimit(0),
    m_maxOutPowerLimit(0), m_maxTotalPowerLimit(0), m_chSetCurrent(CHANNEL_NUMBER, 0),
    m_chActCurrent(CHANNEL_NUMBER, 0), m_chVoltage(CHANNEL_NUMBER,0),
    m_chPower(CHANNEL_NUMBER, 0), m_chError(CHANNEL_NUMBER, 0)
{
    m_lastCmdTime = std::chrono::high_resolution_clock::now();
}

QJsonObject EShimWorker::getAll()
{
    QJsonObject data;
    bool isConnected = getConnected();
    bool isEnabled = getEnabled();
    data.insert("enabled", isEnabled);
    data.insert("connected", isConnected);
    data.insert("lastErr", getLastError());
    if(!isConnected || !isEnabled){
        qDebug() <<"EShimWorker::getAll() !getEnabled() || !getConnected()";
        return data;
    }

    data.insert("busVoltage", getBusVoltage());
    data.insert("maxOutVoltage", getMaxOutVoltageLimit());
    data.insert("maxOutPower", getMaxOutPowerLimit());
    data.insert("maxTotalPower", getMaxTotalPowerLimit());

    QJsonArray deviceStatus;
    Q_FOREACH(auto v, getDeviceStatus()){
        deviceStatus.append(v);
    }
    data.insert("deviceStatus", deviceStatus);

    QJsonArray chSetCurrent;
    Q_FOREACH(auto v, getChSetCurrent()){
        chSetCurrent.append(v);
    }
    data.insert("chSetCurrent", chSetCurrent);

    QJsonArray chActCurrent;
    Q_FOREACH(auto v, getChActCurrent()){
        chActCurrent.append(v);
    }
    data.insert("chActCurrent", chActCurrent);

    QJsonArray chVoltage;
    Q_FOREACH(auto v, getChVoltage()){
        chVoltage.append(v);
    }
    data.insert("chVoltage", chVoltage);

    QJsonArray chPower;
    Q_FOREACH(auto v, getChPower()){
        chPower.append(v);
    }
    data.insert("chPower", chPower);

    QJsonArray chError;
    Q_FOREACH(auto v, getChError()){
        chError.append(v);
    }
    data.insert("chError", chError);
    return data;
}

void EShimWorker::sendAction(const QByteArray &data)
{
    while(SEND_INTERVAL >= (std::chrono::high_resolution_clock::now() - m_lastCmdTime).count() / 1e6){
    }
    sendData(data);
    m_lastCmdTime = std::chrono::high_resolution_clock::now();
}

void EShimWorker::parseBuffer()
{
    QString bufferData = m_buffer.toHex().toUpper();
    QRegExp regex("5AA540(.{2})(.{8})");
    int pos = 0;
    while((pos = regex.indexIn(bufferData, pos)) != -1){
        QString addr = regex.cap(1);
        QString data = regex.cap(2);
        bool ok;
        quint8 addrValue = addr.toUInt(&ok, 16);
        if(ok){
            if(Address::Status01 == addrValue){
                parseStatus(data);
            }else if(Address::Ch1Error == addrValue || Address::Ch2Error == addrValue || Address::Ch3Error == addrValue ||
                Address::Ch4Error == addrValue || Address::Ch5Error == addrValue || Address::Ch6Error == addrValue){
                parseChError(addr.rightRef(1).toInt(), data);
            }else{
                parseValue(addr, hex2Int32(data));
            }
        }else{
            qWarning() << "Failed to use toUInt:" << addr;
        }
        m_buffer.remove(0, pos + regex.matchedLength());    // delete matched data from buffer
        pos += regex.matchedLength();  // continue matching from the next position
    }
}

void EShimWorker::parseChError(int chNumber, const QString &strData)
{
    if(chNumber < 0 || chNumber > CHANNEL_NUMBER - 1){
        qDebug() << "Failed to EShimWorker::parseChError: chNumber is invaild" << chNumber;
        return;
    }
    bool ok;
    quint8 value = strData.leftRef(2).toUInt(&ok, 16);
    if(!ok){
        qWarning() << "Failed to use toUInt in parseChError:" << strData;
    }
    QString errorInfo;
    switch(value){
    case 1:
        errorInfo = "过压";
        break;
    case 2:
        errorInfo = "过功率";
        break;
    case 3:
        errorInfo = "电流错误";
        break;
    default:
        return;
    }
    QMutexLocker locker(&m_mutex);
    m_chError[chNumber] = errorInfo;
}

void EShimWorker::parseStatus(const QString &strData)
{
    QString bigEndianHex;
    for(int i = strData.length() - 2; i >= 0; i -= 2){
        bigEndianHex.append(strData.midRef(i, 2));
    }
    bool ok;
    QString binaryString;
    quint32 decimalValue = bigEndianHex.toUInt(&ok, 16);
    if(ok){
        binaryString = QString::number(decimalValue, 2);
    }else{
        qWarning() << "Failed to use toUInt in parseStatus:" << bigEndianHex;
        return;
    }
    binaryString = binaryString.rightJustified(strData.length() * 4, '0');
    std::string reverseBinary = binaryString.toStdString();
    std::reverse(reverseBinary.begin(),reverseBinary.end());
    binaryString = QString::fromStdString(reverseBinary);

    QVector<QString> vecStatus;
    int len = binaryString.length();
    for(int var = 0; var < len; ++var){
        if(14 == var || 19 == var || 20 == var || 21 == var || 23 == var){
            continue;
        }
        if(22 == var){
            vecStatus.append(CoilStatusMap.value(binaryString.mid(var, 2)));
            continue;
        }
        if(0 == QString::compare(binaryString.at(var), "1")){
            vecStatus.append(StatusInfoMap.value(var));
        }
    }
    setDeviceStatus(vecStatus);
}

void EShimWorker::parseValue(const QString &strAddr, int value)
{
    bool ok;
    quint8 addrValue = strAddr.toUInt(&ok, 16);
    if(!ok){
        qWarning() << "EShimWorker::parseValue  Failed to use toUInt:" << strAddr;
        return;
    }
    if(Address::Ch1SetCurrent == addrValue || Address::Ch2SetCurrent == addrValue || Address::Ch3SetCurrent == addrValue ||
        Address::Ch4SetCurrent == addrValue || Address::Ch5SetCurrent == addrValue || Address::Ch6SetCurrent == addrValue){
        int resIndex = 0;
        if(checkVector(strAddr.right(1), 1, CHANNEL_NUMBER, resIndex)){
            QMutexLocker locker(&m_mutex);
            m_chSetCurrent[resIndex] = value;
        }else{
            qWarning() << "EShimWorker::parseValue  Failed to checkVector";
            return;
        }
    }else if(Address::Ch1ActCurrent == addrValue || Address::Ch2ActCurrent == addrValue || Address::Ch3ActCurrent == addrValue ||
               Address::Ch4ActCurrent == addrValue || Address::Ch5ActCurrent == addrValue || Address::Ch6ActCurrent == addrValue){
        int resIndex = 0;
        if(checkVector(strAddr, 7, CHANNEL_NUMBER, resIndex)){
            QMutexLocker locker(&m_mutex);
            m_chActCurrent[resIndex] = value;
        }else{
            qWarning() << "EShimWorker::parseValue  Failed to checkVector";
            return;
        }
    }else if(Address::Ch1GetVoltage == addrValue || Address::Ch2GetVoltage == addrValue || Address::Ch3GetVoltage == addrValue ||
               Address::Ch4GetVoltage == addrValue || Address::Ch5GetVoltage == addrValue || Address::Ch6GetVoltage == addrValue){
        int res = voltageAddrMap.value(strAddr) - 1;
        if(res >= 0 && res < CHANNEL_NUMBER){
            QMutexLocker locker(&m_mutex);
            m_chVoltage[res] = value;
        }else{
            qWarning() << "EShimWorker::parseValue  Failed to checkVector";
            return;
        }
    }else if(Address::Ch1Power == addrValue || Address::Ch2Power == addrValue || Address::Ch3Power == addrValue ||
               Address::Ch4Power == addrValue || Address::Ch5Power == addrValue || Address::Ch6Power == addrValue){
        int resIndex = 0;
        if(checkVector(strAddr, 28, CHANNEL_NUMBER, resIndex)){
            QMutexLocker locker(&m_mutex);
            m_chPower[resIndex] = value;
        }else{
            qWarning() << "EShimWorker::parseValue  Failed to checkVector";
            return;
        }
    }else if(Address::BusVoltage == addrValue){
        setBusVoltage(value);
    }else if(Address::MaxOutVoltage == addrValue){
        setMaxOutVoltageLimit(value);
    }else if(Address::MaxOutPower == addrValue){
        setMaxOutPowerLimit(value);
    }else if(Address::MaxTotalPower == addrValue){
        setMaxTotalPowerLimit(value);
    }
}

QByteArray EShimWorker::int2LittleEndianHex(int value)
{
    QByteArray byteArray;
    for(int i = 0; i < 4; ++i){
        quint8 byteValue = (value >> (i * 8)) & 0xFF;
        byteArray.append(static_cast<char>(byteValue));
    }
    return byteArray;
}

QString EShimWorker::formatHex(const QByteArray &value)
{
    QString formattedOutput;
    int len = value.size();
    for(int i = 0; i < len; ++i){
        formattedOutput += QString("%1 ").arg(static_cast<quint8>(value[i]), 2, 16, QChar('0')).toUpper();
    }
    return formattedOutput.trimmed();
}

int32_t EShimWorker::hex2Int32(const QString &strData)
{
    int32_t value = 0;
    int len = strData.length();
    for(int i = 0; i < len; i += 2){
        std::string byteString = strData.toStdString().substr(i, 2);
        uint8_t byte = std::stoi(byteString, nullptr, 16);
        value |= static_cast<int32_t>(byte) << (8 * i / 2);
    }
    return value;
}

void EShimWorker::addDefaultData(QByteArray &command, int number)
{
    while(number > 0){
        command.append(DEFAULT_DATA);
        number--;
    }
}

bool EShimWorker::checkVector(QString strValue, int index, int length, int& resIndex)
{
    bool ok;
    int addrValue = strValue.toInt(&ok, 16);
    if(!ok){
        qWarning() << "Failed to use toInt:" << strValue;
        return false;
    }
    int res = addrValue - index;
    if(res < 0 || res >= length){   // exceed the bound of array
        return false;
    }
    resIndex = res;
    return true;
}

void EShimWorker::queryAll()
{
    qDebug() << "EShimWorker::queryAll()";
    for(int i = 1; i <= CHANNEL_NUMBER; ++i){
        queryChannelErr(i);
        queryChannelActCurrent(i);
        queryChannelPower(i);
        queryChannelVoltage(i);
    }
    queryStatus();
}

void EShimWorker::readData()
{
    if(!m_serialPort) return;
    QByteArray newData = m_serialPort->readAll();   // Will be cleared after readAll()
    m_buffer.append(newData);
    if(DATA_LENGTH > m_buffer.length()) return;
    parseBuffer();
}

void EShimWorker::init()
{
    qDebug() << "EShimWorker::init";
    if(getConnected()){
        qDebug() << "EShimWorker::init failed: already open";
        return;
    }
    SerialWorker::init();
}

void EShimWorker::open(const bool closeAndOpen)
{
    if(m_serialPort && m_serialPort->isOpen()){
        if(closeAndOpen){
            m_serialPort->close();
        }else{
            qDebug() << "EShim port is already open, set closeAndOpen to true to close and reopen.";
            return;
        }
    }
    // find available port
    const auto configJ = LabTool::ReadJsonFile("config/exthw.json").value("eshim").toObject();
    setEnabled(configJ.value("enabled").toBool());
    if(!getEnabled()){
        qDebug() << "EShim module is disabled or file config/exthw.json is invalid.";
        setConnected(false);
        return;
    }
    const auto serialJ = configJ.value("serial").toObject();
    const auto port = HwTool::findSerialPort(serialJ);
    if(port.isNull()){
        qDebug() << "Invalid serial port config for EShim: " << serialJ;
        setConnected(false);
        return;
    }else{
        qDebug() << "Setting EShim port as:" << port.portName();
    }
    auto baudRate = serialJ.value("baudRate").toInt(9600);
    if(!HwTool::checkBaudRate(baudRate)){
        qDebug() << "baud rate in config file is invalid: " << baudRate << ". Setting to default 9600.";
        baudRate = 9600;
    }
    m_serialPort->setPort(port);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->open(QIODevice::ReadWrite);
    setConnected(m_serialPort->isOpen());
    if(m_serialPort->isOpen()){
       QTimer::singleShot(1000, this, &EShimWorker::queryAll);
    }
}

void EShimWorker::close()
{
    qDebug() << "EShimWorker::close";
    SerialWorker::close();
}

void EShimWorker::setBoot()
{
    qDebug() << "EShimWorker::setBoot";
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::On));
    writeData.append(static_cast<quint8>(OnData::Boot));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::setShut()
{
    qDebug() << "EShimWorker::setShut";
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::On));
    // to solve 0x00
    writeData.append(static_cast<char>(OnData::Shut));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::initChannel()
{
    qDebug() << "EShimWorker::initChanel";
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::Clear));
    writeData.append(static_cast<quint8>(OnData::Boot));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::setChannelCurrent(int channelNumber, int current)
{
    qDebug() << "EShimWorker::setChannelCurrent" << channelNumber << current;
    if(current < -5000 || current > 5000){
        qWarning() << "Failed to EShimWorker::setChannelCurrent: current should be [-5000, 5000]";
        return;
    }
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1SetCurrent));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2SetCurrent));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3SetCurrent));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4SetCurrent));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5SetCurrent));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6SetCurrent));
        break;
    default:
        qWarning() << "Failed to EShimWorker::setChannelCurrent: channelNumber is invaild";
        return;
    }
    writeData.append(int2LittleEndianHex(current));
    sendAction(writeData);
}

void EShimWorker::setMaxOutVoltage(int voltage)
{
    qDebug() << "EShimWorker::setMaxOutVoltage" << voltage;
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::MaxOutVoltage));
    writeData.append(int2LittleEndianHex(voltage));
    sendAction(writeData);
}

void EShimWorker::setMaxOutPower(int power)
{
    qDebug() << "EShimWorker::setMaxOutPower" << power;
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::MaxOutPower));
    writeData.append(int2LittleEndianHex(power));
    sendAction(writeData);
}

void EShimWorker::setMaxTotalPower(int power)
{
    qDebug() << "EShimWorker::setMaxTotalPower" << power;
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Write));
    writeData.append(static_cast<quint8>(Address::MaxTotalPower));
    writeData.append(int2LittleEndianHex(power));
    sendAction(writeData);
}

void EShimWorker::queryChannelSetCurrent(int channelNumber)
{
    qDebug() << "EShimWorker::queryChannelSetCurrent" << channelNumber;
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1SetCurrent));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2SetCurrent));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3SetCurrent));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4SetCurrent));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5SetCurrent));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6SetCurrent));
        break;
    default:
        qWarning() << "Failed to EShimWorker::getChannelSetCurrent: channelNumber is invaild";
        break;
    }
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryChannelActCurrent(int channelNumber)
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1ActCurrent));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2ActCurrent));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3ActCurrent));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4ActCurrent));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5ActCurrent));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6ActCurrent));
        break;
    default:
        qWarning() << "Failed to EShimWorker::getChannelActCurrent: channelNumber is invaild";
        break;
    }
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryChannelVoltage(int channelNumber)
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1GetVoltage));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2GetVoltage));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3GetVoltage));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4GetVoltage));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5GetVoltage));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6GetVoltage));
        break;
    default:
        qWarning() << "Failed to EShimWorker::getChannelVoltage: channelNumber is invaild";
        break;
    }
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryBusVoltage()
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    writeData.append(static_cast<quint8>(Address::BusVoltage));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryMaxOutVoltage()
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    writeData.append(static_cast<quint8>(Address::MaxOutVoltage));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryChannelPower(int channelNumber)
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1Power));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2Power));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3Power));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4Power));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5Power));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6Power));
        break;
    default:
        qWarning() << "Failed to EShimWorker::getChannelPower: channelNumber is invaild";
        break;
    }
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryMaxOutPower()
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    writeData.append(static_cast<quint8>(Address::MaxOutPower));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryMaxTotalPower()
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    writeData.append(static_cast<quint8>(Address::MaxTotalPower));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryChannelErr(int channelNumber)
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    switch(channelNumber){
    case 1:
        writeData.append(static_cast<quint8>(Address::Ch1Error));
        break;
    case 2:
        writeData.append(static_cast<quint8>(Address::Ch2Error));
        break;
    case 3:
        writeData.append(static_cast<quint8>(Address::Ch3Error));
        break;
    case 4:
        writeData.append(static_cast<quint8>(Address::Ch4Error));
        break;
    case 5:
        writeData.append(static_cast<quint8>(Address::Ch5Error));
        break;
    case 6:
        writeData.append(static_cast<quint8>(Address::Ch6Error));
        break;
    default:
        qWarning() << "Failed to EShimWorker::getChannelErr: channelNumber is invaild";
        break;
    }
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}

void EShimWorker::queryStatus()
{
    QByteArray writeData;
    writeData.append(static_cast<quint8>(Header::First));
    writeData.append(static_cast<quint8>(Header::Second));
    writeData.append(static_cast<quint8>(Operation::Read));
    writeData.append(static_cast<quint8>(Address::Status01));
    addDefaultData(writeData, DATA_LENGTH - writeData.length());
    sendAction(writeData);
}
