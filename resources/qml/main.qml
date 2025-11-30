import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

ApplicationWindow {
    id: root
    visible: true
    visibility: Window.Maximized
    width: 1280
    height: 720
    title: "SPQR Circus"
    color: "#262525"

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open Scene...")
                onTriggered: fileDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                onTriggered: Qt.quit()
            }
        }
    }

    Platform.FileDialog {
        id: fileDialog
        title: "Open Scene File"
        folder: "file://" + appWindow.projectRoot + "/resources/scenes"
        nameFilters: ["YAML Files (*.yaml)", "All Files (*)"]
        onAccepted: {
            var path = file.toString()
            path = path.replace(/^(file:\/{2})/, "")
            path = decodeURIComponent(path)
            appWindow.loadScene(path)
        }
    }

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Top Row with Team Panels and Viewport
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Left Team Panel
                TeamPanel {
                    id: leftPanel
                    side: "left"
                    teamName: "Team 1"
                    teamRobots: appWindow.teams.length > 0 ? appWindow.teams[0].robots : []
                    z: 1
                }

                // Center - Viewport Container (lowest z-order)
                Item {
                    id: viewportContainer
                    objectName: "viewportContainer"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    z: 0
                }

                // Right Team Panel
                TeamPanel {
                    id: rightPanel
                    side: "right"
                    teamName: "Team 2"
                    teamRobots: appWindow.teams.length > 1 ? appWindow.teams[1].robots : []
                    z: 1
                }
            }

            // Bottom Tools Panel
            ToolsPanel {
                id: toolsPanel
                teams: appWindow.teams
                z: 2
            }
        }
    }
}
