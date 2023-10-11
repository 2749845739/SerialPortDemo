/** Copyright (C) Fuqing Medical and USTC BMEC RFLab - All Rights Reserved.
 ** Unauthorized copying of this file, via any medium is strictly prohibited.
 ** Proprietary and confidential.
 ** Created on 20230727, by yue.wang.
 ** brief:  This class is used for asynchronous serial communication with the EShim-1D1S Power Supply
 **/

#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QTextStream>
#include <QMap>
#pragma execution_character_set("utf-8")

class SerialWorker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames WRITE setPortNames NOTIFY portNamesChanged)

public:
    const quint8 DEFAULT_DATA = 0x00;
    enum Header{
        First = 0xA5,
        Second = 0x5A
    };
    enum  Operation{
        Read = 0x20,
        Write = 0x30,
        Feedback = 0X40
    };
    enum Address{
        Ch1SetCurrent = 0x01,
        Ch2SetCurrent = 0x02,
        Ch3SetCurrent = 0x03,
        Ch4SetCurrent = 0x04,
        Ch5SetCurrent = 0x05,
        Ch6SetCurrent = 0x06,
        Ch1ActCurrent = 0x07,
        Ch2ActCurrent = 0x08,
        Ch3ActCurrent = 0x09,
        Ch4ActCurrent = 0x0A,
        Ch5ActCurrent = 0x0B,
        Ch6ActCurrent = 0x0C,
        Ch1Error = 0x10,
        Ch2Error = 0x11,
        Ch3Error = 0x12,
        Ch4Error = 0x13,
        Ch5Error = 0x14,
        Ch6Error = 0x15,
        Ch1GetVoltage = 0x16,
        Ch2GetVoltage = 0x17,
        Ch3GetVoltage = 0x18,
        Ch4GetVoltage = 0x19,
        Ch5GetVoltage = 0x1A,
        Ch6GetVoltage = 0x1B,
        Ch1Power = 0x1C,
        Ch2Power = 0x1D,
        Ch3Power = 0x1E,
        Ch4Power = 0x1F,
        Ch5Power = 0x20,
        Ch6Power = 0x21,
        BusVoltage = 0x22,
        On = 0x23,
        Clear = 0x24,
        Status01 = 0x25,
        MaxOutVoltage = 0x27,
        MaxOutPower = 0x28,
        MaxTotalPower = 0x29
    };
    enum OnData{
        Shut = 0x00,
        Boot = 0x01
    };

    QMap<QString, QString> ChanelErrorNameMap{
        {"10", "通道1"},
        {"11", "通道2"},
        {"12", "通道3"},
        {"13", "通道4"},
        {"14", "通道5"},
        {"15", "通道6"}
    };
    QMap<int, QString> StatusInfoMap{
        {0, "运行"},
        {1, "模块故障"},
        {2, "主电源故障"},
        {3, "直流母线禁止输出"},
        {4, "通道1输出电压故障"},
        {5, "超总功率故障"},
        {6, "通道1超功率故障"},
        {7, "直流母线电压故障"},
        {8, "通道1指令电流更新"},
        {9, "通道2指令电流更新"},
        {10, "通道3指令电流更新"},
        {11, "通道4指令电流更新"},
        {12, "通道5指令电流更新"},
        {13, "线束故障"},
        {15, "过温故障"},
        {16, "电流设置未生效"},
        {17, "电流故障"},
        {18, "电源故障"},
        {24, "通道2输出电压故障"},
        {25, "通道3输出电压故障"},
        {26, "通道4输出电压故障"},
        {27, "通道5输出电压故障"},
        {28, "通道2超功率故障"},
        {29, "通道3超功率故障"},
        {29, "通道3超功率电源故障"},
        {30, "通道4超功率故障"},
        {31, "通道5超功率故障"}
    };
    QMap<QString, QString> CoilStatusMap{
        {"00", "Resistive"},
        {"01", "Idle"},
        {"10", "Supercon"},
        {"11", "No connection"}
    };

    explicit SerialWorker(QObject *parent = nullptr);
    SerialWorker(const SerialWorker &) = delete;
    SerialWorker(SerialWorker &&) = delete;
    SerialWorker &operator=(const SerialWorker &) = delete;
    SerialWorker &operator=(SerialWorker &&) = delete;

    Q_INVOKABLE void initPort(const QString &portName, const int &baudRate);
    Q_INVOKABLE void setBoot();
    Q_INVOKABLE void setShut();
    Q_INVOKABLE void initChanel();
    /**
     * @brief setChannelCurrent: Set the current value of the channel
     * @param channelNumber
     * @param current
     */
    Q_INVOKABLE void setChannelCurrent(const int &channelNumber, const QString &current);
    /**
     * @brief setMaxOutVoltage: Set the maximum output voltage
     * @param voltage
     */
    Q_INVOKABLE void setMaxOutVoltage(const QString &voltage);
    /**
     * @brief setMaxOutPower: Set the maximum output power
     * @param power
     */
    Q_INVOKABLE void setMaxOutPower(const QString &power);
    /**
     * @brief setMaxTotalPower: Set the maximum total power
     * @param power
     */
    Q_INVOKABLE void setMaxTotalPower(const QString &power);
    Q_INVOKABLE void getChannelSetCurrent(const int &channelNumber);
    Q_INVOKABLE void getChannelActCurrent(const int &channelNumber);
    Q_INVOKABLE void getChannelVoltage(const int &channelNumber);
    Q_INVOKABLE void getBusVoltage();
    Q_INVOKABLE void getMaxOutVoltage();
    Q_INVOKABLE void getChannelPower(const int &channelNumber);
    Q_INVOKABLE void getMaxOutPower();
    Q_INVOKABLE void getMaxTotalPower();
    Q_INVOKABLE void getChannelErr(const int &channelNumber);
    Q_INVOKABLE void getStatus();
    /// just for test
    Q_INVOKABLE void clearReadData();

    QStringList portNames() const;
    void setPortNames(const QStringList &newPortNames);

signals:
    void portNamesChanged();


private:
    void getDevicePorts();
    void write(const QByteArray &writeData);
    void parseData();
    void parseChError(const QString &chNumber, const QString &strData);
    void parseStatus(const QString &strData);
    QByteArray str2LittleEndianHex(const QString &strData);
    QString formatHex(const QByteArray &value);
    int32_t hex2Int32(const QString &strData);

private slots:
    void handleBytesWritten(qint64 bytes);
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort = nullptr;
    QByteArray m_writeData;
    QByteArray m_readData;
    QTextStream m_standardOutput;
    qint64 m_bytesWritten = 0;
    bool   m_isOpen;
    QStringList m_portNames;
    QString m_selectPort;
};

#endif // SERIALWORKER_H
