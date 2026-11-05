// SqafConnection —— 视强云台的「设备连接」分区（串口 · Pelco-D）。官邸自己的 QML。
//
// 端口选项随创建参数交入；线路规格由型号固定，连接时只提交端口下标。
import QtQuick
import Lidar.UI
import QtQuick.Controls
import QtQuick.Layouts

ConnectionSection {
    id: root
    required property SqafDevice device
    // 表单由官邸持有；控件不反向依赖包含自己的具体页面类型。
    required property var settings
    required property var parameters
    title: qsTr("设备连接")

    // C++ 提供显示表与默认下标；Qt 控件提交后编辑官邸，不回写后端。

    label: qsTr("串口")
    ChoiceField {
        Layout.fillWidth: true
        model: root.parameters.serialPortOptions
        tips: root.parameters.serialPortTips
        value: root.settings.serialPortIndex
        onCommitted: function (i) {
            if (i === root.settings.serialPortIndex)
                return;
            root.settings.serialPortIndex = i;
            root.settings.logMessage(qsTr("串口：%1").arg(root.parameters.serialPortOptions[i]));
        }
    }

    actionTopMargin: 4
    firstAction: Action {
        text: qsTr("连接")
        onTriggered: root.device.open(root.settings.serialPortIndex)
    }
    secondAction: Action {
        text: qsTr("断开")
        onTriggered: root.device.close()
    }
}
