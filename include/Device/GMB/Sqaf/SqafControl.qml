// SqafControl —— 视强云台的「设备控制」分区：**只有本台特有的**——九宫格点动 + 速度 + 云台指令。
//
// 角度目标、前往云台系 / 世界系、立即停止、安装标定都在族层那块「状态与定位」
// （GimbalSection.qml）——每台云台都有的东西不该两边各写一份。
// 九宫格：按下发、松开停。
// 嵌套组件（inline component / delegate）直接引用外层 id：Bound 把"绑定到定义处的作用域"写明，qmllint 才不报
// 未限定访问。代价：delegate 的模型数据一律走 required property（本文件已如此），不再有隐式注入的 index / modelData。
pragma ComponentBehavior: Bound
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
    title: qsTr("设备控制")

    // 一个方向键。按住持续走，松手停。
    // **格子要方的**（用户定）：宽高都由 pad.cell 定死，不交给布局去拉——
    // GridLayout 里 fillWidth 会把格子按右栏宽度摊平，高度却只有 minimumHeight，于是成了扁矩形。
    component DirButton: Button {
        id: direction
        property var press: null
        width: pad.cell
        height: pad.cell
        // 不 flat：方格看得见才叫方方正正
        font.pointSize: 20
        font.bold: true
        onPressed: {
            if (direction.press && direction.press())
                root.settings.logMessage(qsTr("已下发：方向运动 %1").arg(direction.text))
        }
        onReleased: {
            if (root.device.issueStop())
                root.settings.logMessage(qsTr("已下发：停止"))
        }
    }

    Label { text: qsTr("方向点动"); font.bold: true }

    // Grid（不是 GridLayout）：格子尺寸自己说了算，整块居中摆
    Item {
        Layout.fillWidth: true
        implicitHeight: pad.implicitHeight

        Grid {
            id: pad
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 3
            spacing: 6
            // 按右栏宽度均分，但封顶——右栏一宽就把按钮撑成大方块不好看
            property real cell: Math.min(64, (parent.width - 2 * spacing) / 3)

            DirButton { text: "↖"; press: () => root.device.moveLeftUp(root.settings.hSpeed, root.settings.vSpeed) }
            DirButton { text: "↑"; press: () => root.device.moveUp(root.settings.vSpeed) }
            DirButton { text: "↗"; press: () => root.device.moveRightUp(root.settings.hSpeed, root.settings.vSpeed) }
            DirButton { text: "←"; press: () => root.device.moveLeft(root.settings.hSpeed) }
            DirButton { text: "■" } // 中心：停。不给 press——松手那一下本来就发停止，按下再发一次是多余的
            DirButton { text: "→"; press: () => root.device.moveRight(root.settings.hSpeed) }
            DirButton { text: "↙"; press: () => root.device.moveLeftDown(root.settings.hSpeed, root.settings.vSpeed) }
            DirButton { text: "↓"; press: () => root.device.moveDown(root.settings.vSpeed) }
            DirButton { text: "↘"; press: () => root.device.moveRightDown(root.settings.hSpeed, root.settings.vSpeed) }
        }
    }

    // 范围由型号创建参数交入，控件只编辑官邸持有的整数输入。
    FieldRow {
        label: qsTr("水平速度")
        SliderField { Layout.fillWidth: true
                      from: root.parameters.speedMin; to: root.parameters.speedMax
                      value: root.settings.hSpeed
                      onCommitted: function (v) {
                          if (v === root.settings.hSpeed) return
                          root.settings.hSpeed = v
                          root.settings.logMessage(qsTr("水平速度：%1").arg(v))
                      } }
    }
    FieldRow {
        label: qsTr("俯仰速度")
        SliderField { Layout.fillWidth: true
                      from: root.parameters.speedMin; to: root.parameters.speedMax
                      value: root.settings.vSpeed
                      onCommitted: function (v) {
                          if (v === root.settings.vSpeed) return
                          root.settings.vSpeed = v
                          root.settings.logMessage(qsTr("俯仰速度：%1").arg(v))
                      } }
    }

    Label { text: qsTr("云台指令"); font.bold: true; Layout.topMargin: 6 }

    FieldRow {
        label: qsTr("预置位")
        IntField { Layout.fillWidth: true; Layout.preferredWidth: 80
                   minimum: root.parameters.presetMin; maximum: root.parameters.presetMax
                   value: root.settings.preset
                   onCommitted: function (v) {
                          if (v === root.settings.preset) return
                          root.settings.preset = v
                          root.settings.logMessage(qsTr("预置位：%1").arg(v))
                      } }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 6
        rowSpacing: 6

        Button { Layout.fillWidth: true; text: qsTr("设置预置位")
                 onClicked: if (root.device.storePreset(root.settings.preset))
                     root.settings.logMessage(qsTr("已下发：设置预置位 %1").arg(root.settings.preset)) }
        Button { Layout.fillWidth: true; text: qsTr("调用预置位")
                 onClicked: if (root.device.recallPreset(root.settings.preset))
                     root.settings.logMessage(qsTr("已下发：调用预置位 %1").arg(root.settings.preset)) }
        Button { Layout.fillWidth: true; text: qsTr("开启自检")
                 onClicked: if (root.device.enableSelfCheck()) root.settings.logMessage(qsTr("已下发：开启自检")) }
        Button { Layout.fillWidth: true; text: qsTr("关闭自检")
                 onClicked: if (root.device.disableSelfCheck()) root.settings.logMessage(qsTr("已下发：关闭自检")) }
        Button { Layout.fillWidth: true; text: qsTr("角度实时回传")
                 onClicked: if (root.device.enableAngleRealtime()) root.settings.logMessage(qsTr("已下发：开启角度实时回传")) }
        Button { Layout.fillWidth: true; text: qsTr("角度手动查询")
                 onClicked: if (root.device.setAngleQueryMode()) root.settings.logMessage(qsTr("已下发：角度改为手动查询")) }
        Button { Layout.fillWidth: true; text: qsTr("查询水平角")
                 onClicked: if (root.device.queryPan()) root.settings.logMessage(qsTr("已下发：查询水平角")) }
        Button { Layout.fillWidth: true; text: qsTr("查询俯仰角")
                 onClicked: if (root.device.queryTilt()) root.settings.logMessage(qsTr("已下发：查询俯仰角")) }
    }
}
