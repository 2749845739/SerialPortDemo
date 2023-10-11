/** Copyright (C) Fuqing Medical and USTC BMEC RFLab - All Rights Reserved.
 ** Unauthorized copying of this file, via any medium is strictly prohibited.
 ** Proprietary and confidential.
 ** Created on 20230727, by yue.wang.
 ** brief:  This class is used for asynchronous serial communication with the EShim-1D1S Power Supply
 **/

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import MyLab 1.0

Window {
    width: 1000
    height: 1000
    visible: true

    property int buttonWidth: 100
    property int buttonHeight: 30
    property int rowSpace: 50
    property int titleSize: 16
    property int titleWidth: 100

    ColumnLayout{
        id: layout
        anchors {
            fill: parent
            left: parent.left
            leftMargin: 20
        }
        RowLayout{
            spacing: rowSpace
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            ComboBox {
                id: portBox
                model: serialWorker.portNames
                onModelChanged: {
                    console.log("onModelChanged:", model)
                }
            }
            ComboBox {
                id: baudBox
                model: [9600, 19200, 38400, 115200]
            }
            Button{
                Layout.preferredWidth: buttonWidth * 1.5
                text: "Run"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        // 若内部是int值，获取currentText 自动转为int？？？
                        serialWorker.initPort(portBox.currentText, baudBox.currentText)
                    }
                }
            }
            Button{
                text: "Clear Read"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.clearReadData()
                    }
                }
            }

        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Device")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "boot"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setBoot()
                    }
                }
            }
            Button{
                text: "shut"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setShut()
                    }
                }
            }

        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Setting Current")
                    font.pixelSize: titleSize
                }
            }
            ComboBox {
                id: currentChanelBox
                Layout.preferredWidth: buttonWidth
                model: ["chanel01", "chanel02", "chanel03", "chanel04", "chanel05", "chanel06"]

            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getChannelSetCurrent(currentChanelBox.currentIndex + 1)
                    }
                }
            }
            TextField {
                id: currentInput
                placeholderText: qsTr("Enter value")
                Layout.preferredWidth: buttonWidth
            }
            Button{
                text: "set"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setChannelCurrent(currentChanelBox.currentIndex + 1, currentInput.text)
                    }
                }
            }
            Button{
                text: "init"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.initChanel()
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Actual Current")
                    font.pixelSize: titleSize
                }
            }
            ComboBox {
                id: actCurrentChanelBox
                Layout.preferredWidth: buttonWidth
                model: ["chanel01", "chanel02", "chanel03", "chanel04", "chanel05", "chanel06"]

            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getChannelActCurrent(actCurrentChanelBox.currentIndex + 1)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Voltage")
                    font.pixelSize: titleSize
                }
            }
            ComboBox {
                id: volChanelBox
                Layout.preferredWidth: buttonWidth
                model: ["chanel01", "chanel02", "chanel03", "chanel04", "chanel05", "chanel06"]
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getChannelVoltage(volChanelBox.currentIndex + 1)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Bus Voltage")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getBusVoltage()
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Max Output Voltage")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getMaxOutVoltage()
                    }
                }
            }
            TextField {
                id: macOutVolInput
                placeholderText: qsTr("Enter value")
                Layout.preferredWidth: buttonWidth
            }
            Button{
                text: "set"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setMaxOutVoltage(macOutVolInput.text)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Power")
                    font.pixelSize: titleSize
                }
            }
            ComboBox {
                id: powerChanelBox
                Layout.preferredWidth: buttonWidth
                model: ["chanel01", "chanel02", "chanel03", "chanel04", "chanel05", "chanel06"]
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getChannelPower(powerChanelBox.currentIndex + 1)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Max Output Power")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getMaxOutPower()
                    }
                }
            }
            TextField {
                id: maxOutPowerInput
                placeholderText: qsTr("Enter value")
                Layout.preferredWidth: buttonWidth
            }
            Button{
                text: "set"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setMaxOutPower(maxOutPowerInput.text)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Total Power")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getMaxTotalPower()
                    }
                }
            }
            TextField {
                id: totalPowerInput
                placeholderText: qsTr("Enter value")
                Layout.preferredWidth: buttonWidth
            }
            Button{
                text: "set"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.setMaxTotalPower(totalPowerInput.text)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Error")
                    font.pixelSize: titleSize
                }
            }
            ComboBox {
                id: errorChanelBox
                Layout.preferredWidth: buttonWidth
                model: ["chanel01", "chanel02", "chanel03", "chanel04", "chanel05", "chanel06"]
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getChannelErr(errorChanelBox.currentIndex + 1)
                    }
                }
            }
        }

        RowLayout{
            spacing: rowSpace
            Rectangle{
                width: titleWidth
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Status")
                    font.pixelSize: titleSize
                }
            }
            Button{
                text: "get"
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        serialWorker.getStatus()
                    }
                }
            }
        }
    }

    LabSerial{
        id: serialWorker
    }

}

