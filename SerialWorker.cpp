#include "SerialWorker.h"
#include <QDebug>
#include <QCoreApplication>
#include <QRegExp>
#include <QSerialPortInfo>
#include <QtEndian>

SerialWorker::SerialWorker(QObject *parent) : QObject(parent), m_standardOutput(stdout), m_isOpen(false)
{
    m_serialPort = new QSerialPort(this);
    getDevicePorts();
}

void SerialWorker::write(const QByteArray &writeData)
{
    if (writeData.isEmpty()) {
        m_standardOutput << QObject::tr("Either no data was currently available on "
                                        "the standard input for reading, "
                                        "or an error occurred for port %1, error: %2")
                                .arg(m_serialPort->portName()).arg(m_serialPort->errorString())
                         << "\n";
        return;
    }
    qDebug().noquote() << formatHex(writeData);

    m_writeData = writeData;
    const qint64 bytesWritten = m_serialPort->write(writeData);
    if (bytesWritten == -1) {
        m_standardOutput << QObject::tr("Failed to write the data to port %1, error: %2")
                                .arg(m_serialPort->portName())
                                .arg(m_serialPort->errorString())
                         << "\n";
        return;
    } else if (bytesWritten != m_writeData.size()) {
        m_standardOutput << QObject::tr("Failed to write all the data to port %1, error: %2")
                                .arg(m_serialPort->portName())
                                .arg(m_serialPort->errorString())
                         << "\n";
        return;
    }
}

const int DATA_LENGTH = 8;
void SerialWorker::parseData()
{
    if(DATA_LENGTH > m_readData.length()) return;
    qDebug() << "parseData:" << formatHex(m_readData);
    QString bufferData = m_readData.toHex().toUpper();
    QRegExp regex("5AA540(.{2})(.{8})");

    int pos = 0;
    while ((pos = regex.indexIn(bufferData, pos)) != -1) {
        QString matchedValue = regex.cap(0);
        qDebug() << "matchedValue:" << matchedValue;
        QString addr = regex.cap(1);
        QString data = regex.cap(2);
        qDebug() << "addr:" << addr;
        qDebug() << "data:" << data;
        qDebug() << "value:" << hex2Int32(data);

        bool ok;
        quint8 addrValue = addr.toUInt(&ok, 16);
        if(ok){
            if(Address::Ch1Error == addrValue || Address::Ch2Error == addrValue || Address::Ch3Error == addrValue ||
                Address::Ch4Error == addrValue || Address::Ch5Error == addrValue || Address::Ch6Error == addrValue){
                parseChError(addr, data);
            }else if(Address::Status01 == addrValue){
                parseStatus(data);
            }
        }else{
            qWarning() << "Failed to use toUInt:" << addr;
        }
        m_readData.remove(0, pos + regex.matchedLength());  // 从缓存中删除匹配到的数据
        pos += regex.matchedLength();  // 继续从下一个位置开始匹配
    }
}

void SerialWorker::parseChError(const QString &chNumber, const QString &strData)
{
    qDebug() << "parseChError:" << chNumber << strData;
    bool ok;
    quint8 value = strData.leftRef(2).toUInt(&ok, 16);
    if(!ok){
        qWarning() << "Failed to use toUInt in parseChError:" << strData;
    }
    QString errorInfo;
    switch (value) {
    case 1:
        errorInfo = "过压 ";
        break;
    case 2:
        errorInfo = "过功率 ";
        break;
    case 3:
        errorInfo = "电流错误 ";
        break;
    default:
        return;
    }
    qWarning().noquote() << ChanelErrorNameMap.value(chNumber) << "错误:" << errorInfo;
}


void SerialWorker::parseStatus(const QString &strData)
{
    qDebug() << "parseStatus:" << strData;
    QString bigEndianHex;
    for (int i = strData.length() - 2; i >= 0; i -= 2) {
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
    qDebug() << "Binary:" << binaryString;

    for(int var = 0; var < binaryString.length(); ++var){
        if(14 == var || 19 == var || 20 == var || 21 == var || 23 == var){
            continue;
        }
        if(22 == var){
            qDebug().noquote() << "设备状态: 线圈" << binaryString.mid(22,2) << CoilStatusMap.value(binaryString.mid(22,2));
            continue;
        }
        if(0 == QString::compare(binaryString.at(var), "1")){
            qDebug().noquote() << "设备状态:" << StatusInfoMap.value(var);
        }
    }
}

QByteArray SerialWorker::str2LittleEndianHex(const QString &strData)
{
    qint32 intValue = strData.toInt();
    QByteArray byteArray;
    for (int i = 0; i < 4; ++i) {
        quint8 byteValue = (intValue >> (i * 8)) & 0xFF;
        byteArray.append(static_cast<char>(byteValue));
    }
    return byteArray;
}

QString SerialWorker::formatHex(const QByteArray &value)
{
    QString formattedOutput;
    for (int i = 0; i < value.size(); i++) {
        formattedOutput += QString("%1 ").arg(static_cast<quint8>(value[i]), 2, 16, QChar('0')).toUpper();
    }
    return formattedOutput.trimmed();
}

int32_t SerialWorker::hex2Int32(const QString &strData)
{
    int32_t value = 0;
    for (size_t i = 0; i < strData.length(); i += 2)
    {
        std::string byteString = strData.toStdString().substr(i, 2);
        uint8_t byte = std::stoi(byteString, nullptr, 16);
        value |= static_cast<int32_t>(byte) << (8 * i / 2);
    }
    return value;
}


void SerialWorker::setBoot()
{
    qDebug() << "\nsetBoot";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::On));
        writeData.append(static_cast<quint8>(OnData::Boot));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to setBoot: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::setShut()
{
    qDebug() << "\nsetShut";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::On));
        writeData.append(static_cast<char>(OnData::Shut));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to setShut: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::initChanel()
{
    qDebug() << "\ninitChanel";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::Clear));
        writeData.append(static_cast<quint8>(OnData::Boot));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to initChanel: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::setChannelCurrent(const int &channelNumber, const QString &current)
{
    qDebug() << "\nsetChannelCurrent:"<<channelNumber<< current;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        switch (channelNumber) {
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
            qWarning() << "Failed to setChannelCurrent: channelNumber is invaild";
            return;
        }
        writeData.append(str2LittleEndianHex(current));
        write(writeData);
    }else{
        qWarning() << "Failed to setChannelCurrent: m_serialPort == nullptr or m_isOpen = false";
    }

}

void SerialWorker::setMaxOutVoltage(const QString &voltage)
{
    qDebug() << "\nsetMaxOutVoltage:"<<voltage;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::MaxOutVoltage));
        writeData.append(str2LittleEndianHex(voltage));
        write(writeData);
    }else{
        qWarning() << "Failed to setMaxOutVoltage: m_serialPort == nullptr or m_isOpen = false";
    }

}

void SerialWorker::setMaxOutPower(const QString &power)
{
    qDebug() << "\nsetMaxOutPower:"<<power;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::MaxOutPower));
        writeData.append(str2LittleEndianHex(power));
        write(writeData);
    }else{
        qWarning() << "Failed to setMaxOutPower: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::setMaxTotalPower(const QString &power)
{
    qDebug() << "\nsetMaxTotalPower:"<<power;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Write));
        writeData.append(static_cast<quint8>(Address::MaxTotalPower));
        writeData.append(str2LittleEndianHex(power));
        write(writeData);
    }else{
        qWarning() << "Failed to setMaxTotalPower: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getChannelSetCurrent(const int &channelNumber)
{
    qDebug() << "\ngetChannelSetCurrent:"<<channelNumber;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        switch (channelNumber) {
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
            qWarning() << "Failed to getChannelSetCurrent: channelNumber is invaild";
            break;
        }
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getChannelSetCurrent: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getChannelActCurrent(const int &channelNumber)
{
    qDebug() << "\ngetChannelActCurrent:"<<channelNumber;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        switch (channelNumber) {
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
            qWarning() << "Failed to getChannelActCurrent: channelNumber is invaild";
            break;
        }
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getChannelActCurrent: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getChannelVoltage(const int &channelNumber)
{
    qDebug() << "\ngetChannelVoltage:"<<channelNumber;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        switch (channelNumber) {
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
            qWarning() << "Failed to getChannelVoltage: channelNumber is invaild";
            break;
        }
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getChannelVoltage: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getBusVoltage()
{
    qDebug() << "\ngetBusVoltage:";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        writeData.append(static_cast<quint8>(Address::BusVoltage));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getBusVoltage: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getMaxOutVoltage()
{
    qDebug() << "\ngetMaxOutVoltage:";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        writeData.append(static_cast<quint8>(Address::MaxOutVoltage));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getMaxOutVoltage: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getChannelPower(const int &channelNumber)
{
    qDebug() << "\ngetChannelPower:"<<channelNumber;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        switch (channelNumber) {
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
            qWarning() << "Failed to getChannelPower: channelNumber is invaild";
            break;
        }
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getChannelPower: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getMaxOutPower()
{
    qDebug() << "\ngetMaxOutPower:";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        writeData.append(static_cast<quint8>(Address::MaxOutPower));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getMaxOutPower: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getMaxTotalPower()
{
    qDebug() << "\ngetMaxTotalPower:";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        writeData.append(static_cast<quint8>(Address::MaxTotalPower));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getMaxTotalPower: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getChannelErr(const int &channelNumber)
{
    qDebug() << "\ngetChannelErr:"<<channelNumber;
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        switch (channelNumber) {
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
            qWarning() << "Failed to getChannelErr: channelNumber is invaild";
            break;
        }
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getChannelErr: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::getStatus()
{
    qDebug() << "\ngetStatus:";
    if(m_serialPort != nullptr && m_isOpen){
        QByteArray writeData;
        writeData.append(static_cast<quint8>(Header::First));
        writeData.append(static_cast<quint8>(Header::Second));
        writeData.append(static_cast<quint8>(Operation::Read));
        writeData.append(static_cast<quint8>(Address::Status01));
        writeData.append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA).append(DEFAULT_DATA);
        write(writeData);
    }else{
        qWarning() << "Failed to getStatus: m_serialPort == nullptr or m_isOpen = false";
    }
}

void SerialWorker::clearReadData()
{
    qDebug()<< "\nclear read data";
    m_readData.clear();
}

void SerialWorker::getDevicePorts()
{
    qDebug() << "getDevicePorts ";
    QList<QSerialPortInfo> portInfoList = QSerialPortInfo::availablePorts();
    QStringList allPorts;
    foreach (const QSerialPortInfo& portInfo, portInfoList) {
        QString portName = portInfo.portName();
        allPorts.append(portName);
    }
    setPortNames(allPorts);
}


void SerialWorker::handleBytesWritten(qint64 bytes)
{
    m_bytesWritten += bytes;
    if (m_bytesWritten == m_writeData.size()) {
        m_bytesWritten = 0;
        m_standardOutput << QObject::tr("Data successfully sent to port %1")
                                .arg(m_serialPort->portName()) << "\n";
        return;
    }
}

void SerialWorker::handleReadyRead()
{
    // Will be cleared after readAll()
    QByteArray newData = m_serialPort->readAll();
    qDebug() << "new data :" << newData.toHex();
    m_readData.append(newData);
    parseData();
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::WriteError) {
        m_standardOutput << QObject::tr("An I/O error occurred while writing"
                                        " the data to port %1, error: %2")
                                .arg(m_serialPort->portName())
                                .arg(m_serialPort->errorString())
                         << "\n";
        return;
    }
}

QStringList SerialWorker::portNames() const
{
    return m_portNames;
}

void SerialWorker::setPortNames(const QStringList &newPortNames)
{
    qDebug()<< "setPortNames:"<< newPortNames;
    if (m_portNames == newPortNames)
        return;
    m_portNames = newPortNames;
    emit portNamesChanged();
}

void SerialWorker::initPort(const QString &portName, const int &baudRate)
{
    qDebug() << "initPort:" << portName << baudRate;
    QTextStream standardOutput(stdout);

    m_serialPort->setPortName(portName);
    switch (baudRate) {
    case 9600:
        m_serialPort->setBaudRate(QSerialPort::Baud9600);
        break;
    case 19200:
        m_serialPort->setBaudRate(QSerialPort::Baud19200);
        break;
    case 38400:
        m_serialPort->setBaudRate(QSerialPort::Baud38400);
        break;
    case 115200:
        m_serialPort->setBaudRate(QSerialPort::Baud115200);
        break;
    default:
        qWarning() << "Failed to initPort: baudRate is invaild";
        return;
    }
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    if(m_serialPort->open(QIODevice::ReadWrite)){
        m_standardOutput << QObject::tr("Success to open port %1")
                                .arg(portName)
                         << "\n";
        m_isOpen = true;
    }else{
        standardOutput << QObject::tr("Failed to open port %1, error: %2")
                              .arg(portName)
                              .arg(m_serialPort->errorString())
                       << "\n";
        m_isOpen = false;
    }

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
    connect(m_serialPort, &QSerialPort::bytesWritten, this, &SerialWorker::handleBytesWritten);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
    return;
}


