# Changelog

## [Unreleased] - 2025-12-07

### Added
- **Dynamic ToolsPanel Grid Configuration from Scene YAML**
  - Added `GuiConfig` struct in `SceneParser` to store grid configuration (rows, columns)
  - Scene YAML files can now define tools panel layout using `gui_config` section:
    ```yaml
    gui_config:
      - tools_panel: [rows, columns]
    ```
  - Example: `tools_panel: [1, 6]` creates a 1 row × 6 columns grid

- **Automatic Grid Application on Scene Load**
  - `AppWindow` now exposes `guiConfig` Q_PROPERTY that emits `guiConfigChanged` signal
  - QML automatically applies grid configuration when a scene is loaded
  - Grid cells are distributed equally across rows and columns

### Modified
- **SceneParser** (`include/SceneParser.h`, `src/SceneParser.cpp`)
  - Added `GuiConfig` struct with default values (1 row, 4 columns)
  - Added `guiConfig` field to `SceneSpec`
  - Parser reads `gui_config` section from YAML and extracts `tools_panel` array

- **AppWindow** (`include/AppWindow.h`, `src/AppWindow.cpp`)
  - Added `Q_PROPERTY(QVariantMap guiConfig READ getGuiConfig NOTIFY guiConfigChanged)`
  - Added `getGuiConfig()` method to expose configuration to QML
  - Added `guiConfigChanged()` signal
  - `loadScene()` extracts GUI config from parsed scene and emits signal
  - Added private member `currentGuiConfig_` to store configuration

- **main.qml** (`resources/qml/main.qml`)
  - Added `Connections` block to listen for `guiConfigChanged` signal
  - Added `applyGridConfig(rows, columns)` function to update ToolsPanel
  - Function rebuilds column widths and row heights arrays with equal distribution

- **Scene Files** (`resources/scenes/mixed_scene.yaml`)
  - Added example `gui_config` section with `tools_panel: [1, 6]`

### Technical Details

#### File Changes
1. **include/SceneParser.h:23-30**
   - New `GuiConfig` struct with `rows` and `columns` fields

2. **src/SceneParser.cpp:37-49**
   - Parse `gui_config` section from YAML
   - Support array format: `tools_panel: [rows, columns]`
   - Validates sequence structure before reading values

3. **include/AppWindow.h:20,33,37,51**
   - New Q_PROPERTY for `guiConfig`
   - New signal `guiConfigChanged()`
   - New getter method `getGuiConfig()`
   - New member variable `currentGuiConfig_`

4. **src/AppWindow.cpp:46-49,123-128,200-201**
   - Implementation of `getGuiConfig()` method
   - Extract GUI config from SceneParser in `loadScene()`
   - Emit `guiConfigChanged()` after scene loads

5. **resources/qml/main.qml:56-84**
   - Connections to handle `guiConfigChanged` signal
   - `applyGridConfig()` helper function
   - Dynamic array building for column widths and row heights

### Usage Example

To configure a tools panel with 2 rows and 3 columns, add to your scene YAML:

```yaml
gui_config:
  - tools_panel: [2, 3]
```

When this scene is loaded, the ToolsPanel will automatically reconfigure to a 2×3 grid.

### Default Behavior

If no `gui_config` is specified in the scene YAML:
- Default: 1 row × 4 columns (as defined in `GuiConfig` defaults)

### Future Enhancements
- Support for subcell splitting (split individual cells horizontally/vertically)

---

## [Update] - 2025-12-07

### Added
- **Cell Data Stream Configuration from YAML**
  - Extended `gui_config` section to support `cell_data` entries
  - Each cell can now specify which data stream to display
  - YAML format:
    ```yaml
    gui_config:
      - tools_panel: [rows, columns]
      - cell_data:
          - cell: [row, col]
            stream: "Stream Name"
    ```
  - Parser reads both multi-line and inline formats: `cell: [0, 0]` or separate `row`/`column` fields

- **Cell Data Application in QML**
  - `ToolsPanel` now maintains `cellDataMap` property mapping cell coordinates to stream names
  - ComboBoxes are registered in `comboBoxRefs` map for dynamic access
  - `applyCellData()` function processes cell data list from YAML
  - `Connections` watches `cellDataMapChanged` signal to update ComboBoxes
  - Default behavior: selects first available stream if no configuration exists

- **Save Configuration Feature**
  - Added "Save Config" button in main.qml toolbar
  - `saveGuiConfig()` C++ method writes current grid and cell configuration to YAML
  - Support for both "Save" (overwrite) and "Save As" (new file) workflows
  - Native confirmation dialog using `Qt.labs.platform.MessageDialog`
  - File selection dialog for "Save As" functionality
  - Automatically saves grid dimensions (rows/columns) when modified

- **Dynamic Row and Column Management**
  - Added "+ Add Row" and "- Remove Row" buttons to toolbar
  - Added "+ Add Column" and "- Remove Column" buttons to toolbar
  - Buttons dynamically update grid dimensions and normalize heights/widths
  - Remove buttons disabled when only 1 row/column remains
  - Grid size label shows current dimensions (e.g., "Grid: 2×3")

- **Context Menu for Cells**
  - Right-click on any cell to open context menu
  - "Split Horizontally" option adds a new row to entire grid
  - "Split Vertically" option adds a new column to entire grid

### Modified
- **SceneParser** (`src/SceneParser.cpp:55-64`)
  - Fixed cell coordinate parsing to use `cell: [row, col]` array format
  - Added parsing logic for `cell_data` entries in `gui_config` section
  - Populates `CellData` struct with row, column, and stream values

- **AppWindow** (`include/AppWindow.h`, `src/AppWindow.cpp`)
  - Added `saveGuiConfig(yamlFile, cellData, numRows, numColumns)` Q_INVOKABLE method
  - Added `getCurrentScenePath()` method to track currently loaded scene file
  - Added `currentScenePath_` member variable to store active scene path
  - Extended `currentGuiConfig_` to include `cellData` as QVariantList
  - Each cell entry is a QVariantMap with "row", "column", and "stream" properties
  - Save logic copies entire scene structure (field, ball, teams, robots) when creating new files
  - Updates only `gui_config` section when saving to existing files

- **ToolsPanel.qml** (`resources/qml/ToolsPanel.qml`)
  - Added `cellDataMap` property to store cell configurations
  - Added `comboBoxRefs` property to track ComboBox instances
  - Added `applyCellData(cellDataList)` function to process YAML configuration
  - Added `collectCellData()` function to gather current cell states for saving
  - ComboBox `updateFromConfig()` applies configured stream or defaults to first available
  - ComboBox registers itself in `comboBoxRefs` on creation
  - Added toolbar with row/column management buttons
  - Added context menu with split options for each cell
  - Context menu uses `property var panel: toolsPanel` to access parent scope

- **main.qml** (`resources/qml/main.qml`)
  - Added "Save Config" button to toolbar
  - Added `saveConfigDialog` (Platform.MessageDialog) for overwrite confirmation
  - Added `saveFileDialog` (Platform.FileDialog) for "Save As" functionality
  - Added `saveConfig()` function to initiate save workflow
  - Added `performSave(filePath)` function to collect cell data and call C++ save method
  - Passes `numRows` and `numColumns` to `saveGuiConfig()` for dimension persistence

### Fixed
- **YAML Reference Binding Error**
  - Changed from `YAML::Node&` reference to direct node access
  - Fixed error: "cannot bind non-const lvalue reference to rvalue"

- **MessageDialog Display Issue**
  - Switched from `QtQuick.Dialogs.MessageDialog` to `Qt.labs.platform.MessageDialog`
  - Changed event handlers from `onAccepted/onRejected` to `onYesClicked/onNoClicked`
  - Resolved black screen issue with native platform dialogs

- **Cell Data Validation on Save**
  - Added validation to ensure cell data entries don't exceed grid size
  - Save function now updates `tools_panel` dimensions in YAML
  - Prevents error when grid is expanded during runtime and then saved

### Technical Details

#### File Changes
1. **src/SceneParser.cpp:55-64**
   - Cell parsing using array format: `if (cellNode["cell"] && cellNode["cell"].IsSequence())`
   - Extracts row and column from sequence: `cellData.row = cellNode["cell"][0].as<int>()`

2. **include/AppWindow.h:28,52**
   - New signature: `Q_INVOKABLE void saveGuiConfig(QString, QVariantList, int, int)`
   - New member: `QString currentScenePath_`

3. **src/AppWindow.cpp:131-140,242-316**
   - Expose cell data to QML as QVariantList of QVariantMaps
   - Save implementation with file existence check
   - Copy scene structure from `currentScenePath_` when creating new files
   - Write YAML with updated `tools_panel` dimensions and `cell_data` entries

4. **resources/qml/ToolsPanel.qml:24-68,228-338,412-457**
   - Cell data management functions
   - ComboBox registration and configuration
   - Toolbar buttons for grid management
   - Context menu implementation with MouseArea

5. **resources/qml/main.qml:70-142**
   - Save dialogs and workflow functions
   - Integration with AppWindow save methods

### Usage Example

Configure cell data streams in scene YAML:

```yaml
gui_config:
  - tools_panel: [2, 3]
  - cell_data:
      - cell: [0, 0]
        stream: "Team 1 - Robot 1 - Pose Position"
      - cell: [0, 1]
        stream: "Team 1 - Robot 2 - IMU"
      - cell: [1, 2]
        stream: "Ball Position"
```

### Default Behavior
- If no cell configuration exists, ComboBox defaults to first available stream
- Grid dimensions default to 1 row × 4 columns if not specified
- Save operation preserves all scene data (robots, ball, field) from source file

### Known Limitations
- TODO: When robot addition feature is implemented, save should capture current robot states (position, orientation, type, number) instead of copying from source file
- Context menu split operations currently add rows/columns to entire grid (subcell splitting not yet implemented)
