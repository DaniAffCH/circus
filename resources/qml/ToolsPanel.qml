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

    // Grid properties
    property int numColumns: 4
    property int numRows: 1
    property var columnWidths: [0.25, 0.25, 0.25, 0.25]
    property var rowHeights: [1.0]

    // Teams data
    property var teams: []
    property var dataStreamOptions: buildDataStreamOptions()

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

                                    // Cell Content
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: 2
                                        color: "#3a3a3a"
                                        border.color: "#5c8dbd"
                                        border.width: 1
                                        radius: 2

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 5
                                            spacing: 5

                                            Label {
                                                Layout.alignment: Qt.AlignHCenter
                                                text: "Cell [" + rowContainer.rowIndex + "," + cellContainer.colIndex + "]"
                                                font.pixelSize: 10
                                                color: "#888888"
                                            }

                                            ComboBox {
                                                id: dataStreamCombo
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 30
                                                Layout.alignment: Qt.AlignVCenter
                                                editable: true

                                                model: toolsPanel.dataStreamOptions

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

                                            Item {
                                                Layout.fillHeight: true
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
