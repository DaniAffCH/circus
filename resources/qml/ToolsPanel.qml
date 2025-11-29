import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: toolsPanel

    property bool isExpanded: false
    property int minHeight: 40
    property int maxHeight: Screen.height - 100
    property int expandedHeight: 200
    property int collapsedHeight: 40

    Layout.fillWidth: true
    Layout.preferredHeight: collapsedHeight
    color: "#262525"

    Behavior on Layout.preferredHeight {
        enabled: !resizeMouseArea.pressed
        NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
    }

    // Top Border (Draggable Resize Handle)
    Rectangle {
        id: resizeHandle
        width: parent.width
        height: 6
        color: resizeMouseArea.containsMouse || resizeMouseArea.pressed ? "#5c8dbd" : "#464545"
        anchors.top: parent.top
        anchors.topMargin: -2
        z: 100

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        MouseArea {
            id: resizeMouseArea
            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            hoverEnabled: true
            preventStealing: true
            enabled: toolsPanel.isExpanded

            property real startMouseY: 0
            property real startPanelHeight: 0

            onPressed: (mouse) => {
                // If panel is collapsed, expand it first
                if (!toolsPanel.isExpanded) {
                    toolsPanel.isExpanded = true
                    toolsPanel.Layout.preferredHeight = toolsPanel.expandedHeight
                    return
                }
                
                startMouseY = mouse.y + resizeHandle.y
                startPanelHeight = toolsPanel.Layout.preferredHeight
                mouse.accepted = true
            }

            onPositionChanged: (mouse) => {
                if (pressed && toolsPanel.isExpanded) {
                    var currentMouseY = mouse.y + resizeHandle.y
                    var deltaY = startMouseY - currentMouseY
                    var newHeight = Math.max(toolsPanel.minHeight, Math.min(toolsPanel.maxHeight, startPanelHeight + deltaY))
                    
                    toolsPanel.Layout.preferredHeight = newHeight
                    toolsPanel.expandedHeight = newHeight
                }
            }

            onReleased: {
                if (toolsPanel.isExpanded) {
                    // Snap to collapsed if close to minHeight
                    if (toolsPanel.Layout.preferredHeight < toolsPanel.minHeight + 30) {
                        toolsPanel.isExpanded = false
                        toolsPanel.Layout.preferredHeight = toolsPanel.collapsedHeight
                    } else {
                        toolsPanel.expandedHeight = toolsPanel.Layout.preferredHeight
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        // Toggle Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            background: Rectangle {
                color: "#464545"
                radius: 2
            }

            contentItem: Label {
                text: toolsPanel.isExpanded ? "Tools Panel ▼" : "Tools Panel ▲"
                font.pixelSize: 12
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                toolsPanel.isExpanded = !toolsPanel.isExpanded
                if (toolsPanel.isExpanded) {
                    toolsPanel.Layout.preferredHeight = toolsPanel.expandedHeight
                } else {
                    toolsPanel.Layout.preferredHeight = toolsPanel.collapsedHeight
                }
            }
        }

        // Content Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: toolsPanel.isExpanded

            Rectangle {
                width: toolsPanel.width - 10
                height: Math.max(100, toolsPanel.height - 45)
                color: "#5c5b5b"
                radius: 3

                Label {
                    anchors.centerIn: parent
                    text: "Tools Panel Content"
                    font.pixelSize: 14
                    color: "#ecf0f1"
                }
            }
        }
    }
}
