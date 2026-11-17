pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property QtObject family: internals.device // 云台族基不是 QObject，交出设备对象本身
    sections: [connectionSection, positioningSection, modelControlSection].filter(item => item !== null)
    function worldHistory(begin, end) { return angles ? angles.worldHistory(begin, end) : undefined }
    // 本型号定位不带速度：页面交 0，型号 paramsFor 忽略。
    readonly property real panSpeed: 0
    readonly property real tiltSpeed: 0
    // 官邸独持输入；分区只编辑这份状态，连接和发令时一次提交。
    property int serialPortIndex: internals.connectionParameters.serialPortDefault
    property int hSpeed: internals.controlParameters.speedDefault
    property int vSpeed: internals.controlParameters.speedDefault
    property int preset: internals.controlParameters.presetDefault
    mainPages: [Page {
        title: qsTr("角度")
        contentItem: GimbalCharts {
            id: angles
            device: internals.device
            configuration: internals.device.chartParameters().configuration
        }
    }]
    readonly property Section connectionSection: SqafConnection {
        device: internals.device
        settings: root
        parameters: internals.connectionParameters
    }
    readonly property Section positioningSection: GimbalSection {
        device: internals.device
        settings: root
        panMin: internals.positioningParameters.panMin
        panMax: internals.positioningParameters.panMax
        tiltMin: internals.positioningParameters.tiltMin
        tiltMax: internals.positioningParameters.tiltMax
    }
    readonly property Section modelControlSection: SqafControl {
        device: internals.device
        settings: root
        parameters: internals.controlParameters
    }
    objectName: "SqafModule"
    readonly property string name: String(internals.device.displayName)
    readonly property string subtitle: String(internals.device.subtitle)
    // 借用公共只读资源接口（即设备对象本身），不另存状态。
    readonly property var deviceState: internals.device

    QtObject {
        id: internals
        readonly property var positioningParameters: device.positioningParameters()
        readonly property SqafDevice device: root.SqafDevice
        readonly property var connectionParameters: device.connectionParameters()
        readonly property var controlParameters: device.controlParameters()
    }
    Connections {
        target: internals.device
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
