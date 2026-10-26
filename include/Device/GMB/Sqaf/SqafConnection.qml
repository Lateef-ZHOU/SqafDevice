// SqafConnection —— 视强云台的「设备连接」分区（串口 · Pelco-D）。官邸自己的 QML。
//
// 选项随创建参数交入，选中项仅存在 QML 官邸；连接时交 C++ 校验并转换为强类型。
import QtQuick
import Lidar.UI
import QtQuick.Controls
import QtQuick.Layouts

Section {
    id: root
    required property SqafDevice device
    // 表单由官邸持有；控件不反向依赖包含自己的具体页面类型。
    required property var settings
    required property var parameters
    title: qsTr("设备连接")

    // C++ 提供显示表与默认下标；Qt 控件提交后编辑官邸，不回写后端。
    FieldRow {
        label: qsTr("串口")
        ChoiceField { Layout.fillWidth: true
                      model: root.parameters.serialPortOptions
                      tips: root.parameters.serialPortTips
                      value: root.settings.serialPortIndex
                      onCommitted: function (i) {
                          if (i === root.settings.serialPortIndex) return
                          root.settings.serialPortIndex = i
                          root.settings.logMessage(qsTr("串口：%1").arg(root.parameters.serialPortOptions[i]))
                      } }
    }
    FieldRow {
        label: qsTr("波特率")
        ChoiceField { Layout.fillWidth: true
                      model: root.parameters.baudRateOptions
                      value: root.settings.baudRateIndex
                      onCommitted: function (i) {
                          if (i === root.settings.baudRateIndex) return
                          root.settings.baudRateIndex = i
                          root.settings.logMessage(qsTr("波特率：%1").arg(root.parameters.baudRateOptions[i]))
                      } }
    }
    FieldRow {
        label: qsTr("数据位")
        ChoiceField { Layout.fillWidth: true
                      model: root.parameters.dataBitsOptions
                      value: root.settings.dataBitsIndex
                      onCommitted: function (i) {
                          if (i === root.settings.dataBitsIndex) return
                          root.settings.dataBitsIndex = i
                          root.settings.logMessage(qsTr("数据位：%1").arg(root.parameters.dataBitsOptions[i]))
                      } }
    }
    FieldRow {
        label: qsTr("停止位")
        ChoiceField { Layout.fillWidth: true
                      model: root.parameters.stopBitsOptions
                      value: root.settings.stopBitsIndex
                      onCommitted: function (i) {
                          if (i === root.settings.stopBitsIndex) return
                          root.settings.stopBitsIndex = i
                          root.settings.logMessage(qsTr("停止位：%1").arg(root.parameters.stopBitsOptions[i]))
                      } }
    }
    FieldRow {
        label: qsTr("校验位")
        ChoiceField { Layout.fillWidth: true
                      model: root.parameters.parityOptions
                      value: root.settings.parityIndex
                      onCommitted: function (i) {
                          if (i === root.settings.parityIndex) return
                          root.settings.parityIndex = i
                          root.settings.logMessage(qsTr("校验位：%1").arg(root.parameters.parityOptions[i]))
                      } }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        Button {
            Layout.fillWidth: true
            text: qsTr("连接")
            onClicked: root.device.open(root.settings.serialPortIndex, root.settings.baudRateIndex,
                                        root.settings.dataBitsIndex, root.settings.stopBitsIndex, root.settings.parityIndex)
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("断开")
            onClicked: root.device.close()
        }
    }
}
