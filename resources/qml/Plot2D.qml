import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

// Component to display 2D plot data with 3 subplots for X, Y, Z axes
// Can be used for IMU data, position, velocity, or any 3-axis data
Rectangle {
    id: root
    color: "#2a2a2a"

    // Properties for data
    property string title: "2D Plot"
    property int maxDataPoints: 50  // Maximum number of points to display (reduced for smaller window)
    property real minValue: -20.0
    property real maxValue: 20.0

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

        // Add all points
        for (var i = 0; i < xData.length; i++) {
            seriesX.append(timeData[i], xData[i])
            seriesY.append(timeData[i], yData[i])
            seriesZ.append(timeData[i], zData[i])
        }

        // Update axis ranges
        if (timeData.length > 0) {
            var minTime = timeData[0]
            var maxTime = timeData[timeData.length - 1]
            axisXTime.min = minTime
            axisXTime.max = maxTime
            axisYTime.min = minTime
            axisYTime.max = maxTime
            axisZTime.min = minTime
            axisZTime.max = maxTime
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
        spacing: 3

        // X-axis chart
        ChartView {
            id: chartX
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: "#3a3a3a"
            legend.visible: false
            antialiasing: true
            margins.top: 5
            margins.bottom: 5
            margins.left: 5
            margins.right: 5
            title: "X"
            titleColor: "#ff5555"
            titleFont.pixelSize: 11
            titleFont.bold: true

            ValueAxis {
                id: axisXTime
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            ValueAxis {
                id: axisXValue
                min: root.minValue
                max: root.maxValue
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            LineSeries {
                id: seriesX
                name: "X"
                color: "#ff5555"
                width: 2
                axisX: axisXTime
                axisY: axisXValue
            }
        }

        // Y-axis chart
        ChartView {
            id: chartY
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: "#3a3a3a"
            legend.visible: false
            antialiasing: true
            margins.top: 5
            margins.bottom: 5
            margins.left: 5
            margins.right: 5
            title: "Y"
            titleColor: "#55ff55"
            titleFont.pixelSize: 11
            titleFont.bold: true

            ValueAxis {
                id: axisYTime
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            ValueAxis {
                id: axisYValue
                min: root.minValue
                max: root.maxValue
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            LineSeries {
                id: seriesY
                name: "Y"
                color: "#55ff55"
                width: 2
                axisX: axisYTime
                axisY: axisYValue
            }
        }

        // Z-axis chart
        ChartView {
            id: chartZ
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: "#3a3a3a"
            legend.visible: false
            antialiasing: true
            margins.top: 5
            margins.bottom: 5
            margins.left: 5
            margins.right: 5
            title: "Z"
            titleColor: "#5555ff"
            titleFont.pixelSize: 11
            titleFont.bold: true

            ValueAxis {
                id: axisZTime
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            ValueAxis {
                id: axisZValue
                min: root.minValue
                max: root.maxValue
                labelFormat: "%.1f"
                labelsColor: "#aaaaaa"
                gridLineColor: "#555555"
                color: "#666666"
                labelsFont.pixelSize: 8
            }

            LineSeries {
                id: seriesZ
                name: "Z"
                color: "#5555ff"
                width: 2
                axisX: axisZTime
                axisY: axisZValue
            }
        }
    }
}
