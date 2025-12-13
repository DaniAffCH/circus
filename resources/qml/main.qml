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
                text: qsTr("&Save Config")
                onTriggered: saveConfig()
            }
        }

        Menu {
            title: qsTr("&Simulation")
            Action {
                text: qsTr("&Run")
                // onTriggered: fileDialog.open() TODO: change with placeholder run simulation
            }
            MenuSeparator{}
            Action {
                text: qsTr("&Pause")
                // onTriggered: fileDialog.open() TODO: change with placeholder run simulation
            }
            MenuSeparator{}
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

    Platform.FileDialog {
        id: saveFileDialog
        title: "Save Config As..."
        folder: "file://" + appWindow.projectRoot + "/resources/scenes"
        nameFilters: ["YAML Files (*.yaml)", "All Files (*)"]
        fileMode: Platform.FileDialog.SaveFile
        onAccepted: {
            var path = file.toString()
            path = path.replace(/^(file:\/{2})/, "")
            path = decodeURIComponent(path)
            performSave(path)
        }
    }

    Platform.MessageDialog {
        id: overwriteDialog
        title: "Save Configuration"
        text: "Overwrite existing scene file?"
        buttons: Platform.MessageDialog.Yes | Platform.MessageDialog.No
        onYesClicked: {
            var currentPath = appWindow.getCurrentScenePath()
            performSave(currentPath)
        }
        onNoClicked: {
            saveFileDialog.open()
        }
    }

    // Apply grid configuration from scene YAML
    Connections {
        target: appWindow
        function onGuiConfigChanged() {
            var config = appWindow.guiConfig
            if (config.numRows && config.numColumns) {
                applyGridConfig(config.numRows, config.numColumns)
            }
        }
    }

    function applyGridConfig(rows, columns) {
        toolsPanel.numRows = rows
        toolsPanel.numColumns = columns

        // Rebuild column widths array with equal distribution
        var newColumnWidths = []
        for (var i = 0; i < columns; i++) {
            newColumnWidths.push(1.0 / columns)
        }
        toolsPanel.columnWidths = newColumnWidths

        // Rebuild row heights array with equal distribution
        var newRowHeights = []
        for (var j = 0; j < rows; j++) {
            newRowHeights.push(1.0 / rows)
        }
        toolsPanel.rowHeights = newRowHeights

        // Apply cell data from configuration
        var config = appWindow.guiConfig
        if (config.cellData && config.cellData.length > 0) {
            toolsPanel.applyCellData(config.cellData)
        }
    }

    function saveConfig() {
        var currentPath = appWindow.getCurrentScenePath()
        if (currentPath && currentPath !== "") {
            // File exists, ask to overwrite
            overwriteDialog.open()
        } else {
            // No file loaded, open save dialog
            saveFileDialog.open()
        }
    }

    function performSave(filePath) {
        // Collect cell data from ToolsPanel
        var cellData = toolsPanel.collectCellData()

        // Call C++ function to save with current grid dimensions
        appWindow.saveGuiConfig(filePath, cellData, toolsPanel.numRows, toolsPanel.numColumns)
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
