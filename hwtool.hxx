/** Copyright (C) USTC BMEC RFLab and Fuqing Medical- All Rights Reserved.
** Unauthorized copying of this file, via any medium is strictly prohibited.
** Proprietary and confidential.
**/
#ifndef HWTOOL_H
#define HWTOOL_H
#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QAbstractSocket>

namespace HwTool{
inline int bitCount(unsigned int u)
{
    unsigned int uCount;
    uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
    return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

inline uchar extractBit(uchar data, int bitPosition)
{
    return (data >> bitPosition) & 0b00000001;
}

inline uchar extractBit45(uchar data)
{
    return (data >> 4) & 0b00000011;
}

inline uchar extractBit456(uchar data)
{
    return (data >> 4) & 0b00000111;
}

inline uchar extractBit67(uchar data)
{
    return (data >> 6) & 0b11;
}

inline uchar extractBit0123(uchar data)
{
    return data & 0b00001111;
}

const unsigned char crc_8_table[] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};

inline quint8 calcCRC(const QByteArray &data)
{
    quint8 crc_old = 0, crc_new = 0;
    for( int i = 0; i < data.size(); i ++ ){
        quint8 in_byte = data[i];
        crc_old = crc_new;
        crc_new = crc_8_table[crc_old ^ in_byte];
    }
    return crc_new;
}

inline QString serialErrorToStr(quint8 error){
    QString message;
    switch (error) {
    case QSerialPort::NoError: message = "Successful: "; break;
    case QSerialPort::DeviceNotFoundError:
        message = "DeviceNotFoundError: ";
        break;
    case QSerialPort::PermissionError:
        message = "PermissionError: ";
        break;
    case QSerialPort::OpenError:
        message = "OpenError: ";
        break;
    case QSerialPort::ParityError:
        message = "ParityError: ";
        break;
    case QSerialPort::FramingError:
        message = "FramingError: ";
        break;
    case QSerialPort::BreakConditionError:
        message = "BreakConditionError: ";
        break;
    case QSerialPort::WriteError:
        message = "WriteError: ";
        break;
    case QSerialPort::ReadError:
        message = "ReadError: ";
        break;
    case QSerialPort::ResourceError:
        message = "ResourceError: ";
        break;
    case QSerialPort::UnsupportedOperationError:
        message = "UnsupportedOperationError: ";
        break;
    case QSerialPort::UnknownError:
        message = "UnknownError: ";
        break;
    case QSerialPort::TimeoutError:
        message = "TimeoutError: ";
        break;
    case QSerialPort::NotOpenError:
        message = "NotOpenError: ";
        break;
    }
    return message;
}

inline QVector<unsigned char> hexToBinaryList(quint8 data){
    QVector<unsigned char> list;
    list.append( data & 0x80 );
    list.append( data & 0x40 );
    list.append( data & 0x20 );
    list.append( data & 0x10 );
    list.append( data & 0x08 );
    list.append( data & 0x04 );
    list.append( data & 0x02 );
    list.append( data & 0x01 );
    return list;
}

template<typename T>
inline QJsonArray vecToJsArr(QVector<T> vec)
{
    QJsonArray arr;
    for(int i = 0; i < vec.size(); i++){
        arr.append(vec.at(i));
    }
    return arr;
}

/// Determine if it is a valid baud rate
inline bool checkBaudRate(quint32 baudRate)
{
    bool vaild = false;
    if( baudRate >= 1200 && baudRate <= 115200 ) {
        if( baudRate % 1200 == 0 ) vaild = true;
    }
    return vaild;
}

/// Get all available ports
inline QJsonArray getSerialPortsMeta()
{
    QList<QSerialPortInfo> list = QSerialPortInfo().availablePorts();
    QJsonArray arr;
    for (QSerialPortInfo &port : list) {
        QJsonObject infoTable;
        if( port.hasProductIdentifier() ){
             infoTable.insert("productId", port.productIdentifier());
        }
        if( port.hasVendorIdentifier() ){
             infoTable.insert("vendorId", port.vendorIdentifier());
        }
        infoTable.insert("port", port.portName());
        infoTable.insert("despcription", port.description());
        infoTable.insert("serialNumber", port.serialNumber());
        infoTable.insert("manufacturer", port.manufacturer());
        infoTable.insert("systemLocation", port.systemLocation());
        arr.append(infoTable);
    }
    return arr;
}

/**
 * @brief findSerialPort: Search for available port based on the various fields in JsonObject
 * @param portJ: json object in exthw.json
 * @return available port
 */
inline QSerialPortInfo findSerialPort(const QJsonObject &portJ){
    for (QSerialPortInfo &port : QSerialPortInfo().availablePorts()) {
        if( portJ.contains("serialNumber") ){
            if(port.serialNumber() == portJ.value("serialNumber").toString()){
                return port;
            }
        }
        if( port.hasProductIdentifier() && portJ.contains("productId") ){
            if(port.productIdentifier() == portJ.value("productId").toVariant().toInt()){
                return port;
            }
        }
        if( port.hasVendorIdentifier() && portJ.contains("vendorId") ){
            if(port.vendorIdentifier() == portJ.value("vendorId").toVariant().toInt()){
                return port;
            }
        }
        if( portJ.contains("port") ){
            if(port.portName() == portJ.value("port").toString()){
                return port;
            }
        }
    }
    return QSerialPortInfo();
}

inline QString socketErrorToStr(quint8 error){
    QString errorMsg;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorMsg = "ConnectionRefusedError";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorMsg = "RemoteHostClosedError";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorMsg = "HostNotFoundError";
        break;
    case QAbstractSocket::SocketAccessError:
        errorMsg = "SocketAccessError";
        break;
    case QAbstractSocket::SocketResourceError:
        errorMsg = "SocketResourceError";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorMsg = "SocketTimeoutError";
        break;
    case QAbstractSocket::DatagramTooLargeError:
        errorMsg = "DatagramTooLargeError";
        break;
    case QAbstractSocket::NetworkError:
        errorMsg = "NetworkError";
        break;
    case QAbstractSocket::AddressInUseError:
        errorMsg = "AddressInUseError";
        break;
    case QAbstractSocket::SocketAddressNotAvailableError:
        errorMsg = "SocketAddressNotAvailableError";
        break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        errorMsg = "UnsupportedSocketOperationError";
        break;
    case QAbstractSocket::UnfinishedSocketOperationError:
        errorMsg = "UnfinishedSocketOperationError";
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        errorMsg = "ProxyAuthenticationRequiredError";
        break;
    case QAbstractSocket::SslHandshakeFailedError:
        errorMsg = "SslHandshakeFailedError";
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        errorMsg = "ProxyConnectionRefusedError";
        break;
    case QAbstractSocket::ProxyConnectionClosedError:
        errorMsg = "ProxyConnectionClosedError";
        break;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        errorMsg = "ProxyConnectionTimeoutError";
        break;
    case QAbstractSocket::ProxyNotFoundError:
        errorMsg = "ProxyNotFoundError";
        break;
    case QAbstractSocket::ProxyProtocolError:
        errorMsg = "ProxyProtocolError";
        break;
    case QAbstractSocket::OperationError:
        errorMsg = "OperationError";
        break;
    case QAbstractSocket::SslInternalError:
        errorMsg = "SslInternalError";
        break;
    case QAbstractSocket::TemporaryError:
        errorMsg = "TemporaryError";
        break;
    case QAbstractSocket::UnknownSocketError:
        errorMsg = "UnknownSocketError";
        break;
    }
    return errorMsg;
}
}

#endif
