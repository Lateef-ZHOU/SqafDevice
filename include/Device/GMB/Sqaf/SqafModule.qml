pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property GMBDeviceBase family: internals.device
    sections: [connectionSection, positioningSection, modelControlSection].filter(item => item !== null)
    function worldHistory(begin, end) { return angles ? angles.worldHistory(begin, end) : undefined }
    function moveParameters() { return ({}) }
    // 官邸独持输入；分区只编辑这份状态，连接和发令时一次提交。
    property int serialPortIndex: internals.connectionParameters.serialPortDefault
    property int baudRateIndex: internals.connectionParameters.baudRateDefault
    property int dataBitsIndex: internals.connectionParameters.dataBitsDefault
    property int stopBitsIndex: internals.connectionParameters.stopBitsDefault
    property int parityIndex: internals.connectionParameters.parityDefault
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
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder

    QtObject {
        id: internals
        readonly property var positioningParameters: device.positioningParameters()
        readonly property SqafDevice device: root.SqafDevice
        readonly property var connectionParameters: device.connectionParameters()
        readonly property var controlParameters: device.controlParameters()
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
