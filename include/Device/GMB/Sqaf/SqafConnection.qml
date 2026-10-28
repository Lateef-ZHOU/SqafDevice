// SqafConnection —— 视强云台的「设备连接」分区（串口 · Pelco-D）。官邸自己的 QML。
//
// 端口选项随创建参数交入；线路规格由型号固定，连接时只提交端口下标。
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
    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        opacity: 0.6
        text: root.parameters.description
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        Button {
            Layout.fillWidth: true
            text: qsTr("连接")
            onClicked: root.device.open(root.settings.serialPortIndex)
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("断开")
            onClicked: root.device.close()
        }
    }
}
