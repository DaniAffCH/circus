import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Circus 1.0

Rectangle {
    id: toolsPanel

    property bool isExpanded: false
    property int minHeight: 40
    property int maxHeight: Screen.height - 100
    property int expandedHeight: 200
    property int collapsedHeight: 40

    // Grid properties
    property int numColumns: 4
    property int numRows: 1
    property var columnWidths: [0.25, 0.25, 0.25, 0.25]
    property var rowHeights: [1.0]

    // Teams data
    property var teams: []
    property var dataStreamOptions: buildDataStreamOptions()

    // Cell data configuration - maps "row_col" to stream name
    property var cellDataMap: ({})

    // Store references to all ComboBoxes
    property var comboBoxRefs: ({})

    // Component for Plot2D
    Component {
        id: plot2DComponent
        Plot2D {
            id: plot2d
        }
    }

    // Function to apply cell data configuration
    function applyCellData(cellDataList) {
        var newMap = {}
        for (var i = 0; i < cellDataList.length; i++) {
            var cellData = cellDataList[i]
            var key = cellData.row + "_" + cellData.column
            newMap[key] = cellData.stream
        }
        cellDataMap = newMap
    }

    // Function to collect current cell data from all ComboBoxes
    function collectCellData() {
        var cellDataList = []
        for (var key in comboBoxRefs) {
            var combo = comboBoxRefs[key]
            if (combo) {
                var parts = key.split("_")
                var row = parseInt(parts[0])
                var col = parseInt(parts[1])

                var streamValue = ""
                if (combo.currentIndex >= 0 && combo.currentIndex < combo.model.length) {
                    streamValue = combo.model[combo.currentIndex]
                } else {
                    streamValue = combo.editText || ""
                }

                if (streamValue) {
                    cellDataList.push({
                        "row": row,
                        "column": col,
                        "stream": streamValue
                    })
                }
            }
        }
        return cellDataList
    }

    // Build list of available data streams from all robots
    function buildDataStreamOptions() {
        var options = []

        for (var teamIdx = 0; teamIdx < teams.length; teamIdx++) {
            var team = teams[teamIdx]
            if (!team || !team.robots) continue

            var teamName = "Team " + (teamIdx + 1)

            for (var robotIdx = 0; robotIdx < team.robots.length; robotIdx++) {
                var robot = team.robots[robotIdx]
                if (!robot) continue

                var robotPrefix = teamName + " - " + robot.name + " (R" + robot.number + ")"

                // Add IMU data streams if available
                if (robot.imu) {
                    if (robot.imu.linearAcceleration) {
                        options.push(robotPrefix + " - IMU Linear Acceleration")
                    }
                    if (robot.imu.angularVelocity) {
                        options.push(robotPrefix + " - IMU Angular Velocity")
                    }
                }

                // Add Pose data streams
                if (robot.pose) {
                    if (robot.pose.position) {
                        options.push(robotPrefix + " - Pose Position")
                    }
                    if (robot.pose.orientation) {
                        options.push(robotPrefix + " - Pose Orientation")
                    }
                }
            }
        }

        return options
    }

    // Rebuild options when teams change
    onTeamsChanged: {
        dataStreamOptions = buildDataStreamOptions()
    }

    Layout.fillWidth: true
    Layout.preferredHeight: collapsedHeight
    color: "#262525"

    Behavior on Layout.preferredHeight {
        enabled: !resizeMouseArea.pressed
        NumberAnimation { duration: 100; easing.type: Easing.InOutQuad }
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

        // Toolbar with Add Row button
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            visible: toolsPanel.isExpanded
            spacing: 10

            Button {
                text: "+ Add Row"
                Layout.preferredHeight: 25
                background: Rectangle {
                    color: parent.hovered ? "#5c8dbd" : "#464545"
                    radius: 2
                }
                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 11
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    toolsPanel.numRows++
                    var newHeights = toolsPanel.rowHeights.slice()
                    newHeights.push(1.0 / toolsPanel.numRows)
                    // Normalize heights
                    var total = newHeights.reduce((a, b) => a + b, 0)
                    for (var i = 0; i < newHeights.length; i++) {
                        newHeights[i] /= total
                    }
                    toolsPanel.rowHeights = newHeights
                }
            }

            Button {
                text: "- Remove Row"
                Layout.preferredHeight: 25
                enabled: toolsPanel.numRows > 1
                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? "#bd5c5c" : "#464545") : "#3a3a3a"
                    radius: 2
                }
                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 11
                    color: parent.enabled ? "#ffffff" : "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (toolsPanel.numRows > 1) {
                        toolsPanel.numRows--
                        var newHeights = toolsPanel.rowHeights.slice(0, -1)
                        // Normalize heights
                        var total = newHeights.reduce((a, b) => a + b, 0)
                        for (var i = 0; i < newHeights.length; i++) {
                            newHeights[i] /= total
                        }
                        toolsPanel.rowHeights = newHeights
                    }
                }
            }

            Button {
                text: "+ Add Column"
                Layout.preferredHeight: 25
                background: Rectangle {
                    color: parent.hovered ? "#5c8dbd" : "#464545"
                    radius: 2
                }
                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 11
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    toolsPanel.numColumns++
                    var newWidths = toolsPanel.columnWidths.slice()
                    newWidths.push(1.0 / toolsPanel.numColumns)
                    // Normalize widths
                    var total = newWidths.reduce((a, b) => a + b, 0)
                    for (var i = 0; i < newWidths.length; i++) {
                        newWidths[i] /= total
                    }
                    toolsPanel.columnWidths = newWidths
                }
            }

            Button {
                text: "- Remove Column"
                Layout.preferredHeight: 25
                enabled: toolsPanel.numColumns > 1
                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? "#bd5c5c" : "#464545") : "#3a3a3a"
                    radius: 2
                }
                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 11
                    color: parent.enabled ? "#ffffff" : "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (toolsPanel.numColumns > 1) {
                        toolsPanel.numColumns--
                        var newWidths = toolsPanel.columnWidths.slice(0, -1)
                        // Normalize widths
                        var total = newWidths.reduce((a, b) => a + b, 0)
                        for (var i = 0; i < newWidths.length; i++) {
                            newWidths[i] /= total
                        }
                        toolsPanel.columnWidths = newWidths
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "Grid: " + toolsPanel.numRows + "×" + toolsPanel.numColumns
                font.pixelSize: 11
                color: "#aaaaaa"
            }
        }

        // Content Area - Resizable Grid
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: toolsPanel.isExpanded

            Rectangle {
                anchors.fill: parent
                color: "#5c5b5b"
                radius: 3

                // Grid Container
                Item {
                    anchors.fill: parent
                    anchors.margins: 5

                    Repeater {
                        model: toolsPanel.numRows

                        Item {
                            id: rowContainer
                            property int rowIndex: index
                            x: 0
                            y: {
                                var yPos = 0
                                for (var i = 0; i < rowIndex; i++) {
                                    yPos += parent.height * toolsPanel.rowHeights[i]
                                }
                                return yPos
                            }
                            width: parent.width
                            height: parent.height * toolsPanel.rowHeights[rowIndex]

                            // Columns in this row
                            Repeater {
                                model: toolsPanel.numColumns

                                Item {
                                    id: cellContainer
                                    property int colIndex: index
                                    x: {
                                        var xPos = 0
                                        for (var i = 0; i < colIndex; i++) {
                                            xPos += parent.width * toolsPanel.columnWidths[i]
                                        }
                                        return xPos
                                    }
                                    y: 0
                                    width: parent.width * toolsPanel.columnWidths[colIndex]
                                    height: parent.height

                                    // ToolCellWrapper instance for this cell
                                    ToolCellWrapper {
                                        id: cellWrapper
                                    }

                                    // Cell Content
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: 2
                                        color: "#3a3a3a"
                                        border.color: "#5c8dbd"
                                        border.width: 1
                                        radius: 2

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton
                                            onClicked: {
                                                contextMenu.popup()
                                            }
                                        }

                                        Menu {
                                            id: contextMenu
                                            property var panel: toolsPanel

                                            MenuItem {
                                                text: "Split Horizontally"
                                                onTriggered: {
                                                    // Add a new row
                                                    var panel = contextMenu.panel
                                                    panel.numRows++
                                                    var newHeights = panel.rowHeights.slice()
                                                    newHeights.push(1.0 / panel.numRows)
                                                    // Normalize heights
                                                    var total = newHeights.reduce((a, b) => a + b, 0)
                                                    for (var i = 0; i < newHeights.length; i++) {
                                                        newHeights[i] /= total
                                                    }
                                                    panel.rowHeights = newHeights
                                                }
                                            }

                                            MenuItem {
                                                text: "Split Vertically"
                                                onTriggered: {
                                                    // Add a new column
                                                    var panel = contextMenu.panel
                                                    panel.numColumns++
                                                    var newWidths = panel.columnWidths.slice()
                                                    newWidths.push(1.0 / panel.numColumns)
                                                    // Normalize widths
                                                    var total = newWidths.reduce((a, b) => a + b, 0)
                                                    for (var i = 0; i < newWidths.length; i++) {
                                                        newWidths[i] /= total
                                                    }
                                                    panel.columnWidths = newWidths
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 5
                                            spacing: 3

                                            // Header with cell info and stream selector
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                spacing: 5

                                                Label {
                                                    text: "[" + rowContainer.rowIndex + "," + cellContainer.colIndex + "]"
                                                    font.pixelSize: 9
                                                    color: "#888888"
                                                }

                                                CheckBox {
                                                    id: showPlotCheckbox
                                                    text: "Show Plot"
                                                    checked: false
                                                    font.pixelSize: 9

                                                    indicator: Rectangle {
                                                        implicitWidth: 16
                                                        implicitHeight: 16
                                                        x: showPlotCheckbox.leftPadding
                                                        y: parent.height / 2 - height / 2
                                                        radius: 2
                                                        border.color: showPlotCheckbox.checked ? "#5c8dbd" : "#666666"
                                                        border.width: 1
                                                        color: "#3a3a3a"

                                                        Rectangle {
                                                            width: 10
                                                            height: 10
                                                            x: 3
                                                            y: 3
                                                            radius: 1
                                                            color: "#5c8dbd"
                                                            visible: showPlotCheckbox.checked
                                                        }
                                                    }

                                                    contentItem: Text {
                                                        text: showPlotCheckbox.text
                                                        font: showPlotCheckbox.font
                                                        color: "#aaaaaa"
                                                        verticalAlignment: Text.AlignVCenter
                                                        leftPadding: showPlotCheckbox.indicator.width + showPlotCheckbox.spacing
                                                    }
                                                }

                                                ComboBox {
                                                    id: dataStreamCombo
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 25
                                                    editable: true

                                                    model: toolsPanel.dataStreamOptions

                                                    property string cellKey: rowContainer.rowIndex + "_" + cellContainer.colIndex
                                                    property string selectedStream: {
                                                        if (currentIndex >= 0 && currentIndex < model.length) {
                                                            return model[currentIndex]
                                                        }
                                                        return editText || ""
                                                    }

                                                function updateFromConfig() {
                                                    // Check if there's a configured stream for this cell
                                                    var configuredStream = toolsPanel.cellDataMap[cellKey]

                                                    if (configuredStream) {
                                                        // Try to find the configured stream in the model
                                                        var foundIndex = -1
                                                        for (var i = 0; i < model.length; i++) {
                                                            if (model[i] === configuredStream) {
                                                                foundIndex = i
                                                                break
                                                            }
                                                        }

                                                        if (foundIndex >= 0) {
                                                            currentIndex = foundIndex
                                                        } else {
                                                            editText = configuredStream
                                                        }
                                                    } else {
                                                        // No configuration for this cell, use first available stream
                                                        if (model.length > 0) {
                                                            currentIndex = 0
                                                        }
                                                    }
                                                }

                                                Connections {
                                                    target: toolsPanel
                                                    function onCellDataMapChanged() {
                                                        dataStreamCombo.updateFromConfig()
                                                    }
                                                }

                                                Component.onCompleted: {
                                                    // Register this ComboBox
                                                    var newRefs = toolsPanel.comboBoxRefs
                                                    newRefs[cellKey] = dataStreamCombo
                                                    toolsPanel.comboBoxRefs = newRefs

                                                    updateFromConfig()
                                                }

                                                Component.onDestruction: {
                                                    // Unregister this ComboBox
                                                    var newRefs = toolsPanel.comboBoxRefs
                                                    delete newRefs[cellKey]
                                                    toolsPanel.comboBoxRefs = newRefs
                                                }

                                                // Update wrapper when stream selection changes
                                                onSelectedStreamChanged: {
                                                    if (selectedStream) {
                                                        cellWrapper.setStream(selectedStream, toolsPanel.teams)
                                                    }
                                                }

                                                background: Rectangle {
                                                    color: parent.pressed ? "#2a2a2a" : (parent.hovered ? "#4a4a4a" : "#464545")
                                                    border.color: parent.activeFocus ? "#5c8dbd" : "#555555"
                                                    border.width: 1
                                                    radius: 2
                                                }

                                                contentItem: Item {
                                                    Text {
                                                        anchors.fill: parent
                                                        leftPadding: 8
                                                        rightPadding: dataStreamCombo.indicator.width + 8
                                                        text: dataStreamCombo.displayText || "Select or type..."
                                                        font: dataStreamCombo.font
                                                        color: dataStreamCombo.displayText ? "#ffffff" : "#666666"
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignLeft
                                                        elide: Text.ElideRight
                                                        visible: !textInput.activeFocus && !dataStreamCombo.displayText
                                                    }

                                                    TextInput {
                                                        id: textInput
                                                        anchors.fill: parent
                                                        leftPadding: 8
                                                        rightPadding: dataStreamCombo.indicator.width + 8
                                                        text: dataStreamCombo.editable ? dataStreamCombo.editText : dataStreamCombo.displayText
                                                        font: dataStreamCombo.font
                                                        color: "#ffffff"
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignLeft
                                                        selectByMouse: true
                                                        clip: true
                                                    }
                                                }

                                                indicator: Canvas {
                                                    id: canvas
                                                    x: dataStreamCombo.width - width - 5
                                                    y: dataStreamCombo.topPadding + (dataStreamCombo.availableHeight - height) / 2
                                                    width: 12
                                                    height: 8
                                                    contextType: "2d"

                                                    Connections {
                                                        target: dataStreamCombo
                                                        function onPressedChanged() { canvas.requestPaint() }
                                                    }

                                                    onPaint: {
                                                        context.reset()
                                                        context.moveTo(0, 0)
                                                        context.lineTo(width, 0)
                                                        context.lineTo(width / 2, height)
                                                        context.closePath()
                                                        context.fillStyle = dataStreamCombo.enabled ? "#5c8dbd" : "#666666"
                                                        context.fill()
                                                    }
                                                }

                                                popup: Popup {
                                                    y: dataStreamCombo.height
                                                    width: dataStreamCombo.width
                                                    implicitHeight: contentItem.implicitHeight
                                                    padding: 1
                                                    z: 3
                                                    modal: false
                                                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                                                    contentItem: ListView {
                                                        clip: true
                                                        implicitHeight: contentHeight
                                                        model: dataStreamCombo.popup.visible ? dataStreamCombo.delegateModel : null
                                                        currentIndex: dataStreamCombo.highlightedIndex

                                                        ScrollIndicator.vertical: ScrollIndicator { }
                                                    }

                                                    background: Rectangle {
                                                        color: "#3a3a3a"
                                                        border.color: "#5c8dbd"
                                                        border.width: 1
                                                        radius: 2
                                                    }
                                                }

                                                delegate: ItemDelegate {
                                                    width: dataStreamCombo.width
                                                    contentItem: Text {
                                                        text: modelData
                                                        color: "#ffffff"
                                                        font: dataStreamCombo.font
                                                        elide: Text.ElideRight
                                                        verticalAlignment: Text.AlignVCenter
                                                    }
                                                    highlighted: dataStreamCombo.highlightedIndex === index
                                                    background: Rectangle {
                                                        color: highlighted ? "#5c8dbd" : (parent.hovered ? "#4a4a4a" : "#3a3a3a")
                                                    }
                                                }
                                            }
                                            }

                                            // Data display area - always visible below ComboBox
                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: !showPlotCheckbox.checked
                                                Layout.preferredHeight: showPlotCheckbox.checked ? 40 : -1
                                                Layout.minimumHeight: 30
                                                color: "#5c5b5b"
                                                radius: 3
                                                border.color: "#5c8dbd"
                                                border.width: 1

                                                Label {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    font.pixelSize: 11
                                                    font.family: "monospace"
                                                    font.bold: true
                                                    color: "#ffffff"
                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignLeft
                                                    elide: Text.ElideRight
                                                    wrapMode: Text.NoWrap
                                                    text: {
                                                        if (cellWrapper.hasData) {
                                                            var data = cellWrapper.data
                                                            if (data && data.x !== undefined) {
                                                                // Determine labels based on stream type
                                                                var label1 = "X"
                                                                var label2 = "Y"
                                                                var label3 = "Z"
                                                                if (dataStreamCombo.selectedStream.indexOf("Angular Velocity") >= 0 ||
                                                                    dataStreamCombo.selectedStream.indexOf("Orientation") >= 0) {
                                                                    label1 = "R"
                                                                    label2 = "P"
                                                                    label3 = "Y"
                                                                }

                                                                // If height is small, show on one line
                                                                if (parent.height < 55) {
                                                                    return label1 + ":" + data.x.toFixed(3) + " " +
                                                                           label2 + ":" + data.y.toFixed(3) + " " +
                                                                           label3 + ":" + data.z.toFixed(3)
                                                                } else {
                                                                    // Show on three lines when there's space
                                                                    return label1 + ": " + data.x.toFixed(4) + "\n" +
                                                                           label2 + ": " + data.y.toFixed(4) + "\n" +
                                                                           label3 + ": " + data.z.toFixed(4)
                                                                }
                                                            }
                                                        }
                                                        return "No data"
                                                    }
                                                }

                                                // Update data periodically
                                                Timer {
                                                    interval: 50
                                                    running: true
                                                    repeat: true
                                                    onTriggered: {
                                                        // Force update of text binding
                                                    }
                                                }
                                            }

                                            // Plot area - shows Plot2D for data streams
                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                Layout.minimumHeight: 100
                                                visible: showPlotCheckbox.checked
                                                color: "#1a1a1a"
                                                border.color: "#ff0000"
                                                border.width: 2

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: "Plot area - Loading: " + (plotLoader.status === Loader.Loading ? "Yes" : "No") + 
                                                          "\nReady: " + (plotLoader.status === Loader.Ready ? "Yes" : "No") +
                                                          "\nItem: " + (plotLoader.item ? "Loaded" : "Null") +
                                                          "\nCheckbox: " + showPlotCheckbox.checked +
                                                          "\nStream: " + dataStreamCombo.selectedStream +
                                                          "\nHas IMU/Pose: " + (dataStreamCombo.selectedStream && 
                                                                                (dataStreamCombo.selectedStream.indexOf("IMU") >= 0 ||
                                                                                 dataStreamCombo.selectedStream.indexOf("Pose") >= 0))
                                                    color: "#ffffff"
                                                    font.pixelSize: 10
                                                }

                                                Loader {
                                                    id: plotLoader
                                                    anchors.fill: parent
                                                    active: showPlotCheckbox.checked

                                                    sourceComponent: plot2DComponent

                                                    onStatusChanged: {
                                                        console.log("Plot loader status:", status, "item:", item)
                                                    }

                                                    onLoaded: {
                                                        console.log("Plot loaded! Item:", item)
                                                        if (item) {
                                                            item.title = dataStreamCombo.selectedStream
                                                            console.log("Setting plot title:", dataStreamCombo.selectedStream)
                                                            // Adjust min/max and labels based on stream type
                                                            if (dataStreamCombo.selectedStream.indexOf("IMU Linear Acceleration") >= 0) {
                                                                item.minValue = -20.0
                                                                item.maxValue = 20.0
                                                                item.xAxisLabel = "X"
                                                                item.yAxisLabel = "Y"
                                                                item.zAxisLabel = "Z"
                                                            } else if (dataStreamCombo.selectedStream.indexOf("IMU Angular Velocity") >= 0) {
                                                                item.minValue = -10.0
                                                                item.maxValue = 10.0
                                                                item.xAxisLabel = "roll"
                                                                item.yAxisLabel = "pitch"
                                                                item.zAxisLabel = "yaw"
                                                            } else if (dataStreamCombo.selectedStream.indexOf("Pose Position") >= 0) {
                                                                item.minValue = -5.0
                                                                item.maxValue = 5.0
                                                                item.xAxisLabel = "X"
                                                                item.yAxisLabel = "Y"
                                                                item.zAxisLabel = "Z"
                                                            } else if (dataStreamCombo.selectedStream.indexOf("Pose Orientation") >= 0) {
                                                                item.minValue = -3.5
                                                                item.maxValue = 3.5
                                                                item.xAxisLabel = "roll"
                                                                item.yAxisLabel = "pitch"
                                                                item.zAxisLabel = "yaw"
                                                            }
                                                        }
                                                    }

                                                    Connections {
                                                        target: dataStreamCombo
                                                        function onSelectedStreamChanged() {
                                                            if (plotLoader.item) {
                                                                plotLoader.item.title = dataStreamCombo.selectedStream
                                                                // Update labels and ranges based on stream type
                                                                if (dataStreamCombo.selectedStream.indexOf("IMU Linear Acceleration") >= 0) {
                                                                    plotLoader.item.minValue = -20.0
                                                                    plotLoader.item.maxValue = 20.0
                                                                    plotLoader.item.xAxisLabel = "X"
                                                                    plotLoader.item.yAxisLabel = "Y"
                                                                    plotLoader.item.zAxisLabel = "Z"
                                                                } else if (dataStreamCombo.selectedStream.indexOf("IMU Angular Velocity") >= 0) {
                                                                    plotLoader.item.minValue = -10.0
                                                                    plotLoader.item.maxValue = 10.0
                                                                    plotLoader.item.xAxisLabel = "roll"
                                                                    plotLoader.item.yAxisLabel = "pitch"
                                                                    plotLoader.item.zAxisLabel = "yaw"
                                                                } else if (dataStreamCombo.selectedStream.indexOf("Pose Position") >= 0) {
                                                                    plotLoader.item.minValue = -5.0
                                                                    plotLoader.item.maxValue = 5.0
                                                                    plotLoader.item.xAxisLabel = "X"
                                                                    plotLoader.item.yAxisLabel = "Y"
                                                                    plotLoader.item.zAxisLabel = "Z"
                                                                } else if (dataStreamCombo.selectedStream.indexOf("Pose Orientation") >= 0) {
                                                                    plotLoader.item.minValue = -3.5
                                                                    plotLoader.item.maxValue = 3.5
                                                                    plotLoader.item.xAxisLabel = "roll"
                                                                    plotLoader.item.yAxisLabel = "pitch"
                                                                    plotLoader.item.zAxisLabel = "yaw"
                                                                }
                                                                // Clear old data when switching streams
                                                                plotLoader.item.clearData()
                                                            }
                                                        }
                                                    }

                                                    Connections {
                                                        target: showPlotCheckbox
                                                        function onCheckedChanged() {
                                                            plotLoader.sourceComponent = plotLoader.sourceComponent
                                                        }
                                                    }

                                                    // Timer to update plot data
                                                    Timer {
                                                        interval: 100  // Update every 100ms
                                                        running: plotLoader.item !== null
                                                        repeat: true
                                                        onTriggered: {
                                                            if (plotLoader.item && cellWrapper.hasData) {
                                                                var data = cellWrapper.data
                                                                if (data && data.x !== undefined) {
                                                                    plotLoader.item.addDataPoint(data.x, data.y, data.z)
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            Item {
                                                Layout.fillHeight: true
                                                visible: !plotLoader.item
                                            }
                                        }
                                    }

                                    // Right border resize handle (for columns)
                                    Rectangle {
                                        visible: cellContainer.colIndex < toolsPanel.numColumns - 1
                                        x: parent.width - 2
                                        y: 0
                                        width: 4
                                        height: parent.height
                                        color: columnResizeArea.containsMouse || columnResizeArea.pressed ? "#5c8dbd" : "transparent"
                                        z: 10

                                        MouseArea {
                                            id: columnResizeArea
                                            anchors.fill: parent
                                            cursorShape: Qt.SizeHorCursor
                                            hoverEnabled: true
                                            preventStealing: true

                                            property real startMouseX: 0
                                            property real startWidth: 0
                                            property real startNextWidth: 0

                                            onPressed: (mouse) => {
                                                startMouseX = mouseX + parent.x
                                                startWidth = toolsPanel.columnWidths[cellContainer.colIndex]
                                                startNextWidth = toolsPanel.columnWidths[cellContainer.colIndex + 1]
                                            }

                                            onPositionChanged: (mouse) => {
                                                if (pressed) {
                                                    var currentMouseX = mouseX + parent.x
                                                    var deltaX = currentMouseX - startMouseX
                                                    var containerWidth = cellContainer.parent.width
                                                    var deltaRatio = deltaX / containerWidth

                                                    var newWidths = toolsPanel.columnWidths.slice()
                                                    var newCurrent = Math.max(0.05, Math.min(0.95, startWidth + deltaRatio))
                                                    var newNext = Math.max(0.05, startNextWidth - deltaRatio)

                                                    // Ensure we don't go negative
                                                    if (newCurrent >= 0.05 && newNext >= 0.05) {
                                                        newWidths[cellContainer.colIndex] = newCurrent
                                                        newWidths[cellContainer.colIndex + 1] = newNext
                                                        toolsPanel.columnWidths = newWidths
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Bottom border resize handle (for rows)
                            Rectangle {
                                visible: rowContainer.rowIndex < toolsPanel.numRows - 1
                                x: 0
                                y: parent.height - 2
                                width: parent.width
                                height: 4
                                color: rowResizeArea.containsMouse || rowResizeArea.pressed ? "#5c8dbd" : "transparent"
                                z: 10

                                MouseArea {
                                    id: rowResizeArea
                                    anchors.fill: parent
                                    cursorShape: Qt.SizeVerCursor
                                    hoverEnabled: true
                                    preventStealing: true

                                    property real startMouseY: 0
                                    property real startHeight: 0
                                    property real startNextHeight: 0

                                    onPressed: (mouse) => {
                                        startMouseY = mouseY + parent.y
                                        startHeight = toolsPanel.rowHeights[rowContainer.rowIndex]
                                        startNextHeight = toolsPanel.rowHeights[rowContainer.rowIndex + 1]
                                    }

                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            var currentMouseY = mouseY + parent.y
                                            var deltaY = currentMouseY - startMouseY
                                            var containerHeight = rowContainer.parent.height
                                            var deltaRatio = deltaY / containerHeight

                                            var newHeights = toolsPanel.rowHeights.slice()
                                            var newCurrent = Math.max(0.05, Math.min(0.95, startHeight + deltaRatio))
                                            var newNext = Math.max(0.05, startNextHeight - deltaRatio)

                                            // Ensure we don't go negative
                                            if (newCurrent >= 0.05 && newNext >= 0.05) {
                                                newHeights[rowContainer.rowIndex] = newCurrent
                                                newHeights[rowContainer.rowIndex + 1] = newNext
                                                toolsPanel.rowHeights = newHeights
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
