# RoboCup Circus - GUI Configuration System Guide

This guide provides a comprehensive explanation of the GUI configuration system implemented for the RoboCup Circus application, covering YAML-based configuration, dynamic grid management, and the complete data flow from file to UI.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [YAML Configuration Format](#yaml-configuration-format)
4. [Data Flow: YAML to QML](#data-flow-yaml-to-qml)
5. [Component Details](#component-details)
6. [User Interface Features](#user-interface-features)
7. [Saving Configuration](#saving-configuration)
8. [Code Examples](#code-examples)
9. [Future Enhancements](#future-enhancements)

---

## Overview

The GUI configuration system allows users to:
- Define grid layout (rows and columns) for the Tools Panel via YAML files
- Assign specific data streams to individual cells
- Dynamically modify the grid during runtime (add/remove rows and columns)
- Save the current configuration back to YAML files
- Use context menus for quick grid modifications

### Key Features

- **YAML-driven configuration**: Scene files define the initial grid layout and cell data
- **Dynamic updates**: Grid can be modified at runtime without restarting
- **Persistence**: Current state can be saved to existing or new YAML files
- **Flexible cell assignment**: Each cell can display any available data stream
- **Default behavior**: Cells automatically select the first available stream if not configured

---

## Architecture

### Component Layers

```
┌─────────────────────────────────────────┐
│         YAML Scene File                 │
│  (resources/scenes/*.yaml)              │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         SceneParser (C++)               │
│  Parses YAML → SceneSpec struct         │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         AppWindow (C++)                 │
│  Converts to QVariantMap/QVariantList   │
│  Exposes via Q_PROPERTY                 │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         main.qml (QML)                  │
│  Receives signal, applies configuration │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         ToolsPanel.qml (QML)            │
│  Renders grid, manages cell data        │
└─────────────────────────────────────────┘
```

### File Structure

```
circus/
├── include/
│   ├── SceneParser.h          # GuiConfig and CellData structs
│   └── AppWindow.h            # Q_PROPERTY declarations
├── src/
│   ├── SceneParser.cpp        # YAML parsing logic
│   └── AppWindow.cpp          # Data conversion and save logic
├── resources/
│   ├── qml/
│   │   ├── main.qml           # Signal handling and toolbar
│   │   └── ToolsPanel.qml     # Grid rendering and cell management
│   └── scenes/
│       └── *.yaml             # Scene configuration files
```

---

## YAML Configuration Format

### Basic Structure

```yaml
gui_config:
  - tools_panel: [rows, columns]
  - cell_data:
      - cell: [row, col]
        stream: "Stream Name"
      - cell: [row, col]
        stream: "Stream Name"
```

### Example: 2x3 Grid with Cell Assignments

```yaml
gui_config:
  - tools_panel: [2, 3]
  - cell_data:
      - cell: [0, 0]
        stream: "Team 1 - Robot 1 - Pose Position"
      - cell: [0, 1]
        stream: "Team 1 - Robot 1 - IMU"
      - cell: [0, 2]
        stream: "Team 1 - Robot 2 - Pose Position"
      - cell: [1, 0]
        stream: "Team 2 - Robot 1 - Pose Position"
      - cell: [1, 1]
        stream: "Ball Position"
      - cell: [1, 2]
        stream: "Team 2 - Robot 2 - IMU"
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `tools_panel` | Array[int, int] | Grid dimensions [rows, columns] |
| `cell_data` | Array of objects | List of cell configurations |
| `cell` | Array[int, int] | Cell coordinates [row, column] (0-indexed) |
| `stream` | String | Name of the data stream to display |

### Default Values

If `gui_config` is not specified:
- **Grid**: 1 row × 4 columns
- **Cells**: First available stream for each cell

---

## Data Flow: YAML to QML

### Step-by-Step Data Conversion

#### 1. YAML Parsing (SceneParser.cpp)

**Location:** `src/SceneParser.cpp:55-64`

```cpp
// Parse cell_data from gui_config
if (guiConfigNode[1] && guiConfigNode[1]["cell_data"]) {
    for (const auto& cellNode : guiConfigNode[1]["cell_data"]) {
        CellData cellData;
        if (cellNode["cell"] && cellNode["cell"].IsSequence() && cellNode["cell"].size() >= 2) {
            cellData.row = cellNode["cell"][0].as<int>();
            cellData.column = cellNode["cell"][1].as<int>();
        }
        cellData.stream = cellNode["stream"].as<string>();
        sceneInfo.guiConfig.cellData.push_back(cellData);
    }
}
```

**Result:** `vector<CellData>` stored in `sceneInfo.guiConfig.cellData`

**Data Structure:**
```cpp
struct CellData {
    int row;
    int column;
    string stream;
};
```

#### 2. C++ to Qt Conversion (AppWindow.cpp)

**Location:** `src/AppWindow.cpp:131-140`

```cpp
// Expose cell data to QML
QVariantList cellDataList;
for (const auto& cellData : sceneInfo.guiConfig.cellData) {
    QVariantMap cellMap;
    cellMap["row"] = cellData.row;
    cellMap["column"] = cellData.column;
    cellMap["stream"] = QString::fromStdString(cellData.stream);
    cellDataList.append(cellMap);
}
currentGuiConfig_["cellData"] = cellDataList;
```

**Result:** `QVariantList` of `QVariantMap` objects

**Data Structure:**
```javascript
[
    {row: 0, column: 0, stream: "Team 1 - Robot 1 - Pose Position"},
    {row: 0, column: 1, stream: "Team 1 - Robot 1 - IMU"},
    ...
]
```

#### 3. Signal Emission (AppWindow.cpp)

**Location:** `src/AppWindow.cpp:200-201`

```cpp
emit guiConfigChanged();
```

**Property Definition:** `include/AppWindow.h:20`

```cpp
Q_PROPERTY(QVariantMap guiConfig READ getGuiConfig NOTIFY guiConfigChanged)
```

#### 4. Signal Reception (main.qml)

**Location:** `resources/qml/main.qml:56-68`

```javascript
Connections {
    target: appWindow
    function onGuiConfigChanged() {
        var config = appWindow.guiConfig

        if (config.rows !== undefined && config.columns !== undefined) {
            applyGridConfig(config.rows, config.columns)
        }

        // Apply cell data if present
        if (config.cellData !== undefined) {
            toolsPanel.applyCellData(config.cellData)
        }
    }
}
```

#### 5. Data Processing (ToolsPanel.qml)

**Location:** `resources/qml/ToolsPanel.qml:31-44`

```javascript
function applyCellData(cellDataList) {
    var newMap = {}
    for (var i = 0; i < cellDataList.length; i++) {
        var cellData = cellDataList[i]
        var key = cellData.row + "_" + cellData.column
        newMap[key] = cellData.stream
    }
    cellDataMap = newMap
}
```

**Result:** JavaScript object mapping cell coordinates to streams

**Data Structure:**
```javascript
{
    "0_0": "Team 1 - Robot 1 - Pose Position",
    "0_1": "Team 1 - Robot 1 - IMU",
    "1_0": "Team 2 - Robot 1 - Pose Position"
}
```

#### 6. ComboBox Application (ToolsPanel.qml)

**Location:** `resources/qml/ToolsPanel.qml:477-504`

```javascript
function updateFromConfig() {
    // Check if there's a configured stream for this cell
    var configuredStream = toolsPanel.cellDataMap[cellKey]

    if (configuredStream) {
        // Find the stream in the model
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
        // No configuration, use first available stream
        if (model.length > 0) {
            currentIndex = 0
        }
    }
}
```

### Type Conversions Summary

| Layer | C++ Type | Qt Type | JavaScript Type |
|-------|----------|---------|-----------------|
| SceneParser | `vector<CellData>` | - | - |
| AppWindow | - | `QVariantList` | - |
| QML | - | - | `Array` |
| Cell Entry | `CellData` | `QVariantMap` | `Object` |
| String | `std::string` | `QString` | `string` |
| Integer | `int` | `int` | `number` |

---

## Component Details

### SceneParser

**Files:** `include/SceneParser.h`, `src/SceneParser.cpp`

#### Key Structures

```cpp
struct GuiConfig {
    int rows = 1;
    int columns = 4;
    vector<CellData> cellData;
};

struct CellData {
    int row;
    int column;
    string stream;
};
```

#### Parsing Logic

1. Reads `gui_config` section from YAML
2. Extracts `tools_panel` array → `GuiConfig.rows`, `GuiConfig.columns`
3. Iterates through `cell_data` entries
4. For each entry, creates `CellData` object
5. Stores in `GuiConfig.cellData` vector

### AppWindow

**Files:** `include/AppWindow.h`, `src/AppWindow.cpp`

#### Q_PROPERTY

```cpp
Q_PROPERTY(QVariantMap guiConfig READ getGuiConfig NOTIFY guiConfigChanged)
```

Exposes `currentGuiConfig_` to QML as a readable property.

#### Key Methods

| Method | Purpose |
|--------|---------|
| `getGuiConfig()` | Returns `currentGuiConfig_` QVariantMap |
| `loadScene(QString)` | Parses scene file, populates config, emits signal |
| `saveGuiConfig(QString, QVariantList, int, int)` | Saves current config to YAML |
| `getCurrentScenePath()` | Returns path of currently loaded scene |

#### Member Variables

```cpp
QVariantMap currentGuiConfig_;  // Stores current GUI configuration
QString currentScenePath_;      // Tracks loaded scene file path
```

### ToolsPanel.qml

**File:** `resources/qml/ToolsPanel.qml`

#### Key Properties

```javascript
property int numColumns: 4
property int numRows: 1
property var columnWidths: [0.25, 0.25, 0.25, 0.25]
property var rowHeights: [1.0]
property var cellDataMap: ({})        // Maps "row_col" → stream name
property var comboBoxRefs: ({})       // Maps "row_col" → ComboBox object
```

#### Key Functions

| Function | Purpose |
|----------|---------|
| `applyCellData(cellDataList)` | Converts array to cellDataMap |
| `collectCellData()` | Gathers current cell states for saving |
| `buildDataStreamOptions()` | Builds available stream list from teams |

#### ComboBox Management

Each ComboBox:
1. Registers itself in `comboBoxRefs` on creation
2. Watches `cellDataMapChanged` signal
3. Calls `updateFromConfig()` when signal fires
4. Unregisters on destruction

---

## User Interface Features

### Toolbar Buttons

#### Add/Remove Row

**Location:** `resources/qml/ToolsPanel.qml:228-282`

- **+ Add Row**: Adds a new row, recalculates heights
- **- Remove Row**: Removes last row (disabled when numRows = 1)

```javascript
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
```

#### Add/Remove Column

**Location:** `resources/qml/ToolsPanel.qml:284-338`

- **+ Add Column**: Adds a new column, recalculates widths
- **- Remove Column**: Removes last column (disabled when numColumns = 1)

Similar logic to rows but operates on `numColumns` and `columnWidths`.

### Context Menu

**Location:** `resources/qml/ToolsPanel.qml:412-457`

Right-click on any cell to access:
- **Split Horizontally**: Adds a row to the entire grid
- **Split Vertically**: Adds a column to the entire grid

```javascript
Menu {
    id: contextMenu
    property var panel: toolsPanel

    MenuItem {
        text: "Split Horizontally"
        onTriggered: {
            var panel = contextMenu.panel
            panel.numRows++
            // ... normalization logic
        }
    }

    MenuItem {
        text: "Split Vertically"
        onTriggered: {
            var panel = contextMenu.panel
            panel.numColumns++
            // ... normalization logic
        }
    }
}
```

### Grid Display

**Location:** `resources/qml/ToolsPanel.qml:351-530`

- Outer `Repeater` iterates over rows
- Inner `Repeater` iterates over columns
- Each cell contains:
  - Label showing coordinates `[row, col]`
  - ComboBox for stream selection
  - MouseArea for context menu
  - Menu with split options

---

## Saving Configuration

### User Workflow

1. **Trigger Save**: Click "Save Config" button in toolbar
2. **Check File Existence**:
   - If current scene exists → Show overwrite confirmation dialog
   - If no current scene → Show file selection dialog
3. **Collect Data**: `collectCellData()` gathers all ComboBox values
4. **Write YAML**: C++ method writes configuration to file

### Save Logic Flow

#### Step 1: User Clicks Save (main.qml)

**Location:** `resources/qml/main.qml:136-142`

```javascript
function saveConfig() {
    var currentPath = appWindow.getCurrentScenePath()
    if (currentPath) {
        saveConfigDialog.open()
    } else {
        saveFileDialog.open()
    }
}
```

#### Step 2: Confirmation Dialog (main.qml)

**Location:** `resources/qml/main.qml:70-82`

```javascript
Platform.MessageDialog {
    id: saveConfigDialog
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
```

#### Step 3: Collect Cell Data (ToolsPanel.qml)

**Location:** `resources/qml/ToolsPanel.qml:46-68`

```javascript
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
```

#### Step 4: Perform Save (main.qml)

**Location:** `resources/qml/main.qml:144-147`

```javascript
function performSave(filePath) {
    var cellData = toolsPanel.collectCellData()
    appWindow.saveGuiConfig(filePath, cellData, toolsPanel.numRows, toolsPanel.numColumns)
}
```

#### Step 5: Write YAML (AppWindow.cpp)

**Location:** `src/AppWindow.cpp:242-316`

```cpp
void AppWindow::saveGuiConfig(const QString& yamlFile, const QVariantList& cellData,
                               int numRows, int numColumns) {
    YAML::Node root;

    // Check if file exists
    std::ifstream fin(yamlFile.toStdString());
    bool fileExists = fin.good();
    fin.close();

    if (fileExists) {
        // Load existing file
        root = YAML::LoadFile(yamlFile.toStdString());
    } else {
        // Copy from current scene (preserves robots, ball, field)
        if (!currentScenePath_.isEmpty()) {
            root = YAML::LoadFile(currentScenePath_.toStdString());
        } else {
            // Create minimal structure
            root["field"] = "fieldRCAP";
            root["ball"]["position"] = /* ... */;
            root["teams"] = YAML::Node(YAML::NodeType::Map);
        }
    }

    // Update tools_panel dimensions
    YAML::Node toolsPanelDims(YAML::NodeType::Sequence);
    toolsPanelDims.push_back(numRows);
    toolsPanelDims.push_back(numColumns);
    root["gui_config"][0]["tools_panel"] = toolsPanelDims;

    // Update cell_data
    YAML::Node cellDataNode(YAML::NodeType::Sequence);
    for (const QVariant& cellVariant : cellData) {
        QVariantMap cellMap = cellVariant.toMap();
        YAML::Node cellNode;
        YAML::Node cellCoords(YAML::NodeType::Sequence);
        cellCoords.push_back(cellMap["row"].toInt());
        cellCoords.push_back(cellMap["column"].toInt());
        cellNode["cell"] = cellCoords;
        cellNode["stream"] = cellMap["stream"].toString().toStdString();
        cellDataNode.push_back(cellNode);
    }
    root["gui_config"][1]["cell_data"] = cellDataNode;

    // Write to file
    std::ofstream fout(yamlFile.toStdString());
    fout << root;
    fout.close();
}
```

### Save Behavior

| Scenario | Behavior |
|----------|----------|
| Saving to existing file | Updates only `gui_config` section |
| Saving to new file | Copies entire scene structure from current file |
| No current scene | Creates minimal YAML structure |

### What Gets Saved

- Grid dimensions (`tools_panel: [rows, columns]`)
- Cell data for all non-empty cells
- When creating new file: field, ball, teams, robots from current scene

### What Doesn't Get Saved (Yet)

- Custom column widths and row heights (always uses equal distribution)
- Current robot positions if modified during runtime (TODO)

---

## Code Examples

### Example 1: Adding a New Data Stream to a Cell

**Scenario:** You want to programmatically set cell [0, 2] to display "Custom Stream"

```javascript
// In ToolsPanel.qml
var newMap = toolsPanel.cellDataMap
newMap["0_2"] = "Custom Stream"
toolsPanel.cellDataMap = newMap  // Triggers cellDataMapChanged signal
```

The ComboBox at [0, 2] will automatically update via its `Connections` block.

### Example 2: Creating a 3x3 Grid Programmatically

```javascript
// In main.qml
function createThreeByThreeGrid() {
    toolsPanel.numRows = 3
    toolsPanel.numColumns = 3

    // Equal distribution
    toolsPanel.rowHeights = [0.333, 0.333, 0.334]
    toolsPanel.columnWidths = [0.333, 0.333, 0.334]
}
```

### Example 3: Accessing Cell Data from C++

```cpp
// In AppWindow.cpp
QVariantMap config = currentGuiConfig_;
QVariantList cellData = config["cellData"].toList();

for (const QVariant& cellVariant : cellData) {
    QVariantMap cellMap = cellVariant.toMap();
    int row = cellMap["row"].toInt();
    int col = cellMap["column"].toInt();
    QString stream = cellMap["stream"].toString();

    qDebug() << "Cell [" << row << "," << col << "] → " << stream;
}
```

### Example 4: Custom YAML Scene with Full Configuration

```yaml
# my_custom_scene.yaml
field: "fieldRCAP"

ball:
  position: [0.0, 0.0, 0.12]

teams:
  - name: "Team 1"
    robots:
      - number: 1
        type: "nao"
        position: [1.0, 2.0, 0.0]
        orientation: [0.0, 0.0, 1.57]

gui_config:
  - tools_panel: [2, 4]
  - cell_data:
      - cell: [0, 0]
        stream: "Team 1 - Robot 1 - Pose Position"
      - cell: [0, 1]
        stream: "Team 1 - Robot 1 - IMU"
      - cell: [0, 2]
        stream: "Team 1 - Robot 1 - Odometry"
      - cell: [0, 3]
        stream: "Ball Position"
      - cell: [1, 0]
        stream: "Team 1 - Robot 1 - Camera Front"
      - cell: [1, 1]
        stream: "Team 1 - Robot 1 - Camera Bottom"
      - cell: [1, 2]
        stream: "Team 1 - Robot 1 - Sonar"
      - cell: [1, 3]
        stream: "Field State"
```

---

## Future Enhancements

### Planned Features

1. **Subcell Splitting**
   - Allow individual cells to be split into subcells
   - Each cell maintains its own internal grid
   - Preserves main grid structure
   - YAML format TBD

2. **Custom Column/Row Sizes**
   - Save and load custom width/height ratios
   - YAML format:
     ```yaml
     tools_panel:
       rows: [0.3, 0.7]
       columns: [0.25, 0.25, 0.5]
     ```

3. **Dynamic Robot State Capture**
   - Save current robot positions/orientations during runtime
   - Update YAML with live robot data instead of copying from source
   - Requires robot manipulation features to be implemented first

4. **Cell Merging**
   - Merge adjacent cells into a single larger cell
   - Opposite of splitting
   - UI: Select multiple cells and merge

5. **Layout Templates**
   - Predefined grid layouts for common use cases
   - Quick-switch between layouts
   - Save custom layouts as templates

6. **Drag-and-Drop Stream Assignment**
   - Drag stream from a list and drop onto cell
   - Visual feedback during drag operation

### Known Limitations

- Context menu split operations add rows/columns to the entire grid (subcell splitting not yet implemented)
- Cell widths and heights are always evenly distributed
- No undo/redo functionality for grid modifications
- Saving to a new file copies the original scene's robot states, not the current runtime states

---

## Troubleshooting

### Issue: Cell data not applying after scene load

**Cause:** `guiConfigChanged` signal not emitted or not connected

**Solution:**
1. Check that `emit guiConfigChanged()` is called in `AppWindow::loadScene()`
2. Verify `Connections` block in `main.qml` has correct target (`appWindow`)

### Issue: ComboBox shows wrong stream

**Cause:** `cellDataMap` key format mismatch

**Solution:**
- Keys must be in format `"row_column"` (e.g., `"0_1"`)
- Check that `applyCellData()` creates keys correctly
- Verify ComboBox `cellKey` property matches format

### Issue: Save creates empty or invalid YAML

**Cause:** `collectCellData()` returning empty array

**Solution:**
- Ensure ComboBoxes are registered in `comboBoxRefs`
- Check `Component.onCompleted` in ComboBox registers itself
- Verify `Component.onDestruction` doesn't remove refs prematurely

### Issue: Context menu doesn't appear

**Cause:** MouseArea not capturing right-click events

**Solution:**
- Verify `acceptedButtons: Qt.RightButton` is set
- Check that MouseArea `anchors.fill: parent` is correct
- Ensure MouseArea is not obscured by other components

---

## Glossary

| Term | Definition |
|------|------------|
| **Cell** | A single grid position containing a ComboBox for stream selection |
| **Cell Data** | Configuration specifying which stream a cell should display |
| **GUI Config** | Overall configuration including grid dimensions and cell data |
| **Stream** | A data source (e.g., robot sensor, ball position) that can be displayed |
| **Q_PROPERTY** | Qt mechanism for exposing C++ properties to QML |
| **QVariantMap** | Qt type representing a dictionary/map, converted to JavaScript object in QML |
| **QVariantList** | Qt type representing a list/array, converted to JavaScript array in QML |
| **Signal/Slot** | Qt's event notification system for communication between objects |

---

## References

### File Locations

- **SceneParser**: `include/SceneParser.h`, `src/SceneParser.cpp`
- **AppWindow**: `include/AppWindow.h`, `src/AppWindow.cpp`
- **Main QML**: `resources/qml/main.qml`
- **ToolsPanel**: `resources/qml/ToolsPanel.qml`
- **Scenes**: `resources/scenes/*.yaml`

### Key Code Sections

- YAML parsing: `SceneParser.cpp:55-64`
- C++ to Qt conversion: `AppWindow.cpp:131-140`
- Signal handling: `main.qml:56-68`
- Cell data application: `ToolsPanel.qml:31-44`
- Save logic: `AppWindow.cpp:242-316`
- ComboBox update: `ToolsPanel.qml:477-504`

---

## Conclusion

This guide covers the complete implementation of the GUI configuration system, from YAML parsing to UI rendering and persistence. The architecture uses Qt's signal/slot mechanism and property system to create a reactive, data-driven interface that seamlessly integrates C++ backend logic with QML frontend components.

For questions or contributions, please refer to the main project repository.
