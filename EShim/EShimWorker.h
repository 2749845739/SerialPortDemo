/** Copyright (C) Fuqing Medical and USTC BMEC RFLab - All Rights Reserved.
 ** Unauthorized copying of this file, via any medium is strictly prohibited.
 ** Proprietary and confidential.
 ** Created on 20230804, by yue.wang.
 ** brief:  This class is used for asynchronous serial communication with the EShim-1D1S Power Supply
 **/
#ifndef ESHIMWORKER_H
#define ESHIMWORKER_H

#include <QByteArray>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QMutex>
#include "propSetGet.h"
#include "serialworker.h"
#pragma execution_character_set("utf-8")

class QSerialPort;
class EShimWorker : public SerialWorker
{
    Q_OBJECT
    LAB_PROP_RS(busVoltage, int, BusVoltage);
    LAB_PROP_RS(maxOutVoltageLimit, int, MaxOutVoltageLimit);
    LAB_PROP_RS(maxOutPowerLimit, int, MaxOutPowerLimit);
    LAB_PROP_RS(maxTotalPowerLimit, int, MaxTotalPowerLimit);
    LAB_PROP_RS(chSetCurrent, QVector<int>, ChSetCurrent);
    LAB_PROP_RS(chActCurrent, QVector<int>, ChActCurrent);
    LAB_PROP_RS(chVoltage, QVector<int>, ChVoltage);
    LAB_PROP_RS(chPower, QVector<int>, ChPower);
    LAB_PROP_RS(chError, QVector<QString>, ChError);
    LAB_PROP_RS(deviceStatus, QVector<QString>, DeviceStatus);

public:
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
    QMap<QString, int> voltageAddrMap{
        {"16", 1},
        {"17", 2},
        {"18", 3},
        {"19", 4},
        {"1A", 5},
        {"1B", 6}
    };

    explicit EShimWorker(QObject *parent = nullptr);

    /// Get all useful data pack in json,
    /// the method should be able to call from another thread.
    QJsonObject getAll();

protected:
    /// send instruction data
    void sendAction(const QByteArray &data);

    /// parse the entire read buffer
    void parseBuffer();

    /// parse the chanel error infomation
    void parseChError(int chNumber, const QString &strData);

    /// parse the device status
    void parseStatus(const QString &strData);

    /// parse other value  of query
    void parseValue(const QString &strAddr, int value);

    /// convet int to hex of little endian
    QByteArray int2LittleEndianHex(int value);

    /// convet QByteArray to format hex string
    QString formatHex(const QByteArray &value);

    /// convert hex string to int32_t
    int32_t hex2Int32(const QString &strData);

    /**
     * @brief addDefaultData: Add default data for instruction
     * @param command         instruction
     * @param number          the number of default data
     */
    void addDefaultData(QByteArray &command, int number);

    /**
     * @brief checkVector: Check if the array is out of bounds
     * @param strValue     Number in string form
     * @param index        Required parameters
     * @param length       The length of the array
     * @param resIndex     Valid parameter values
     * @return bool        True: is vaild
     */
    bool checkVector(QString strValue, int index, int length, int& resIndex);

Q_SIGNALS:
    LAB_PROP_SIGNAL(busVoltage, int);
    LAB_PROP_SIGNAL(maxOutVoltageLimit, int);
    LAB_PROP_SIGNAL(maxOutPowerLimit, int);
    LAB_PROP_SIGNAL(maxTotalPowerLimit, int);
    LAB_PROP_SIGNAL(chSetCurrent, QVector<int>);
    LAB_PROP_SIGNAL(chActCurrent, QVector<int>);
    LAB_PROP_SIGNAL(chVoltage, QVector<int>);
    LAB_PROP_SIGNAL(chPower, QVector<int>);
    LAB_PROP_SIGNAL(chError, QVector<QString>);
    LAB_PROP_SIGNAL(deviceStatus, QVector<QString>);

protected Q_SLOTS:
    /// when serial port is ready to read
    void readData() override;

public Q_SLOTS:
    void init() override;

    /// Read config file and open serial port.
    void open(const bool closeAndOpen = true) override;

    void close() override;

    /// Let the internal power supply of the device start outputting
    void setBoot();

    /// Let the internal power supply of the device stop outputting
    void setShut();

    /// Initialize the device: set all current to 0
    void initChannel();

    /**
     * @brief setChannelCurrent: Set the current value of the channel
     * @param channelNumber:     The number of channel, from 1 to 6
     * @param current:           [-5000, 5000]
     */
    void setChannelCurrent(int channelNumber, int current);

    /// Set the maximum output voltage
    void setMaxOutVoltage(int voltage);

    /// Set the maximum output power
    void setMaxOutPower(int power);

    /// Set the maximum total power
    void setMaxTotalPower(int power);

    /**
     * @brief queryChannelSetCurrent: Get the current setting value of the channel
     * @param channelNumber:          The number of channel, from 1 to 6
     */
    void queryChannelSetCurrent(int channelNumber);

    /**
     * @brief queryChannelActCurrent: Get the current actual value of the channel
     * @param channelNumber:          The number of channel, from 1 to 6
     */
    void queryChannelActCurrent(int channelNumber);

    /**
     * @brief queryChannelVoltage: Get the voltage actual value of the channel
     * @param channelNumber:       The number of channel, from 1 to 6
     */
    void queryChannelVoltage(int channelNumber);

    /// Get the bus voltage value
    void queryBusVoltage();

    /// Get the maximum output voltage value
    void queryMaxOutVoltage();

    /**
     * @brief queryChannelPower: Get the power actual value of the channel
     * @param channelNumber:     The number of channel, from 1 to 6
     */
    void queryChannelPower(int channelNumber);

    /// Get the maximum output power value
    void queryMaxOutPower();

    /// Get the maximum total power value
    void queryMaxTotalPower();

    /**
     * @brief queryChannelErr: Get the error infomation of the channel
     * @param channelNumber: The number of channel, from 1 to 6
     */
    void queryChannelErr(int channelNumber);

    /// query the device status infomation
    void queryStatus();

    ///  query all data from devices
    void queryAll();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastCmdTime;
    QMutex m_mutex;
};

#endif // ESHIMWORKER_H
