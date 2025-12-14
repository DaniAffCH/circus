import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

// Component to display 2D plot data with a single chart and toggle controls
// Can be used for IMU data, position, velocity, or any 3-axis data
Rectangle {
    id: root
    color: "#2a2a2a"

    // Properties for data
    property string title: "2D Plot"
    property int maxDataPoints: 50  // Maximum number of points to display
    property real minValue: -20.0
    property real maxValue: 20.0

    // Properties for axis labels
    property string xAxisLabel: "X"
    property string yAxisLabel: "Y"
    property string zAxisLabel: "Z"

    // Visibility toggles for each axis
    property bool showX: true
    property bool showY: true
    property bool showZ: true

    // Arrays to store historical data for each axis
    property var xData: []
    property var yData: []
    property var zData: []
    property var timeData: []
    property real currentTime: 0

    // Function to add new data point
    function addDataPoint(x, y, z) {
        // Add new data
        xData.push(x)
        yData.push(y)
        zData.push(z)
        timeData.push(currentTime)
        currentTime += 0.1  // Increment time (adjust based on your update rate)

        // Remove old data if we exceed maxDataPoints
        if (xData.length > maxDataPoints) {
            xData.shift()
            yData.shift()
            zData.shift()
            timeData.shift()
        }

        // Update all series
        updateSeries()
    }

    // Function to update all chart series
    function updateSeries() {
        // Clear all series
        seriesX.clear()
        seriesY.clear()
        seriesZ.clear()

        // Add all points if visible
        for (var i = 0; i < xData.length; i++) {
            if (showX) seriesX.append(timeData[i], xData[i])
            if (showY) seriesY.append(timeData[i], yData[i])
            if (showZ) seriesZ.append(timeData[i], zData[i])
        }

        // Update axis ranges
        if (timeData.length > 0) {
            var minTime = timeData[0]
            var maxTime = timeData[timeData.length - 1]
            axisTime.min = minTime
            axisTime.max = maxTime
        }
    }

    // Function to clear all data
    function clearData() {
        xData = []
        yData = []
        zData = []
        timeData = []
        currentTime = 0
        updateSeries()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        // Main chart
        ChartView {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: "#3a3a3a"
            legend.visible: false
            antialiasing: true
            margins.top: 10
            margins.bottom: 10
            margins.left: 10
            margins.right: 10

            ValueAxis {
                id: axisTime
                titleText: "Time (s)"
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 9
            }

            ValueAxis {
                id: axisValue
                min: root.minValue
                max: root.maxValue
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 9
            }

            LineSeries {
                id: seriesX
                name: root.xAxisLabel
                color: "#ff5555"
                width: 2
                axisX: axisTime
                axisY: axisValue
                visible: root.showX
            }

            LineSeries {
                id: seriesY
                name: root.yAxisLabel
                color: "#55ff55"
                width: 2
                axisX: axisTime
                axisY: axisValue
                visible: root.showY
            }

            LineSeries {
                id: seriesZ
                name: root.zAxisLabel
                color: "#5555ff"
                width: 2
                axisX: axisTime
                axisY: axisValue
                visible: root.showZ
            }
        }

        // Control panel with checkboxes in column
        Rectangle {
            Layout.preferredWidth: 60
            Layout.fillHeight: true
            color: "#3a3a3a"
            border.color: "#555555"
            border.width: 1
            radius: 3

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15

                Item {
                    Layout.fillHeight: true
                }

                // X-axis checkbox
                CheckBox {
                    id: checkboxX
                    text: root.xAxisLabel
                    checked: root.showX
                    Layout.alignment: Qt.AlignLeft
                    onCheckedChanged: {
                        root.showX = checked
                        updateSeries()
                    }

                    contentItem: Text {
                        text: checkboxX.text
                        font.pixelSize: 11
                        color: "#cccccc"
                        leftPadding: checkboxX.indicator.width + checkboxX.spacing
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        x: checkboxX.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#ff5555"
                        border.width: 2
                        color: checkboxX.checked ? "#ff5555" : "transparent"

                        Rectangle {
                            width: 8
                            height: 8
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: checkboxX.checked
                        }
                    }
                }

                // Y-axis checkbox
                CheckBox {
                    id: checkboxY
                    text: root.yAxisLabel
                    checked: root.showY
                    Layout.alignment: Qt.AlignLeft
                    onCheckedChanged: {
                        root.showY = checked
                        updateSeries()
                    }

                    contentItem: Text {
                        text: checkboxY.text
                        font.pixelSize: 11
                        color: "#cccccc"
                        leftPadding: checkboxY.indicator.width + checkboxY.spacing
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        x: checkboxY.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#55ff55"
                        border.width: 2
                        color: checkboxY.checked ? "#55ff55" : "transparent"

                        Rectangle {
                            width: 8
                            height: 8
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: checkboxY.checked
                        }
                    }
                }

                // Z-axis checkbox
                CheckBox {
                    id: checkboxZ
                    text: root.zAxisLabel
                    checked: root.showZ
                    Layout.alignment: Qt.AlignLeft
                    onCheckedChanged: {
                        root.showZ = checked
                        updateSeries()
                    }

                    contentItem: Text {
                        text: checkboxZ.text
                        font.pixelSize: 11
                        color: "#cccccc"
                        leftPadding: checkboxZ.indicator.width + checkboxZ.spacing
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        x: checkboxZ.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#5555ff"
                        border.width: 2
                        color: checkboxZ.checked ? "#5555ff" : "transparent"

                        Rectangle {
                            width: 8
                            height: 8
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: checkboxZ.checked
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
