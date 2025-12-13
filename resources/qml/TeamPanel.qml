import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: teamPanel

    // Properties
    property string side: "left"  // "left" or "right"
    property string teamName: "Team"
    property var teamRobots: []
    property bool isExpanded: teamRobots.length > 0

    readonly property bool isLeftSide: side === "left"

    Layout.preferredWidth: isExpanded ? 300 : 60
    Layout.fillHeight: true
    color: "#262525"

    Behavior on Layout.preferredWidth {
        NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
    }

    // Border (left side for left panel, right side for right panel)
    Rectangle {
        width: 2
        height: parent.height
        color: "#464545"
        anchors.left: isLeftSide ? parent.right : undefined
        anchors.right: isLeftSide ? undefined : parent.left
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        // Toggle Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            background: Rectangle {
                color: "#464545"
                radius: 2
            }

            contentItem: Item {
                anchors.fill: parent

                Label {
                    text: {
                        if (isExpanded) {
                            return (isLeftSide ? "Team 1 <<" : "Team 2 >>")
                        } else {
                            return (isLeftSide ? "T1 >>" : "T2 <<")
                        }
                    }
                    font.pixelSize: 15
                    font.bold: true
                    color: "#ffffff"
                    anchors.centerIn: parent
                }
            }

            onClicked: isExpanded = !isExpanded
        }

        // Content Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: teamPanel.width - 10
                spacing: 5

                Repeater {
                    model: teamRobots

                    delegate: Rectangle {
                        Layout.preferredWidth: parent.width
                        Layout.preferredHeight: isExpanded ? (robotExpander.expanded ? robotContent.height + robotHeader.height : robotHeader.height) : 40
                        Layout.alignment: Qt.AlignHCenter
                        color: "#5c5b5b"
                        radius: 3

                        Behavior on Layout.preferredHeight {
                            NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                        }

                        Column {
                            anchors.fill: parent

                            // Robot Header (Always visible)
                            Rectangle {
                                id: robotHeader
                                width: parent.width
                                height: 40
                                color: "#5c5b5b"
                                radius: 3

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 5

                                    // Left side: show image first (for left panel)
                                    Image {
                                        source: robotExpander.expanded ? "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M7 10l5 5 5-5z'/%3E%3C/svg%3E"
                                                                      : "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M10 17l5-5-5-5z'/%3E%3C/svg%3E"
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        visible: isExpanded
                                    }

                                    Label {
                                        text: isExpanded ?
                                            (modelData.name + " (R" + modelData.number + ")") :
                                            "R" + modelData.number
                                        font.pixelSize: isExpanded ? 12 : 14
                                        font.bold: true
                                        color: "#ecf0f1"
                                        Layout.fillWidth: true
                                        horizontalAlignment: isExpanded ? Text.AlignLeft : Text.AlignHCenter
                                    }
                                }

                                MouseArea {
                                    id: robotExpander
                                    anchors.fill: parent
                                    property bool expanded: false
                                    onClicked: {
                                        if (isExpanded) {
                                            expanded = !expanded
                                        } else {
                                            isExpanded = true
                                            expanded = true
                                        }
                                    }
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            // Robot Details (Expandable)
                            Column {
                                id: robotContent
                                width: parent.width * 0.9
                                visible: isExpanded && robotExpander.expanded
                                opacity: (isExpanded && robotExpander.expanded) ? 1 : 0
                                padding: 10
                                spacing: 8

                                Behavior on opacity {
                                    NumberAnimation { duration: 200 }
                                }

                                // Label {
                                //     text: modelData.type
                                //     font.pixelSize: 10
                                //     color: "#bdc3c7"
                                //     width: parent.width - parent.padding * 2
                                // }

                                Label {
                                    text: "Position: [ X: " +
                                          modelData.pose.position[0].toFixed(2) + ", Y: " +
                                          modelData.pose.position[1].toFixed(2) + ", Z: " +
                                          modelData.pose.position[2].toFixed(2) + " ]"
                                    font.pixelSize: 10
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width - parent.padding * 2
                                }

                                Label {
                                    text: "Orientation: [ R: " +
                                          modelData.pose.orientation[0].toFixed(2) + ", P: " +
                                          modelData.pose.orientation[1].toFixed(2) + ", Y: " +
                                          modelData.pose.orientation[2].toFixed(2) + "]"
                                    font.pixelSize: 10
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width - parent.padding * 2
                                }

                                Label {
                                    text: modelData.imu && modelData.imu.linearAcceleration ?
                                            "Linear Acceleration: [ X: " +
                                            modelData.imu.linearAcceleration[0].toFixed(3) + ", Y: " +
                                            modelData.imu.linearAcceleration[1].toFixed(3) + ", Z: " +
                                            modelData.imu.linearAcceleration[2].toFixed(3) + "]"
                                            : "    [N/A]"
                                    font.pixelSize: 10
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }

                                Label {
                                    text: modelData.imu && modelData.imu.angularVelocity ?
                                            "Angular Velocity: [ X: " +
                                            modelData.imu.angularVelocity[0].toFixed(3) + ", Y: " +
                                            modelData.imu.angularVelocity[1].toFixed(3) + ", Z: " +
                                            modelData.imu.angularVelocity[2].toFixed(3) + "]"
                                            : "    [N/A]"
                                    font.pixelSize: 10
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
