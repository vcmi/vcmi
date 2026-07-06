Semi-Automatic Map Generation Mode for VCMI Editor
Context
The user wants to add a new mode to the VCMI editor that enables semi-automatic map generation via a dialog. This streamlines map creation by letting users select key metaparameters (size, player count, water content, difficulty, levels) and generate a playable map that can then be further edited in the editor.

Requirements
Exposed parameters: Map dimensions (width/height), player count, water content, difficulty/monster strength, levels (surface/underground)
UI entry point: File → Generate Map menu item
Workflow: Dialog offers choice at generation time to either replace current map or open in new editor window
Incremental commits: Separate each functional piece into distinct commits
High-Level Approach
Phase 1: Create Generation Dialog UI (Commit 1)
File: mapeditor/GenerateMapDialog.h/.cpp, mapeditor/GenerateMapDialog.ui

Create a new dialog following existing editor patterns (mirror WindowNewMap structure):

Core parameters:

Map width (range: 36-144, step 4)
Map height (range: 36-144, step 4)
Player count (1-8, with "random" option)
Water content (combo: None, Normal, Islands)
Monster strength (combo: enum from CMapGenOptions)
Levels (spinbox: 1-2, representing surface and underground)
Controls:

Combo boxes for fixed selections (water, difficulty)
Spin boxes for dimensions and counts
Checkboxes for level options
Generate & Cancel buttons
Settings persistence: Use QSettings to save last-used parameter values (follows editor conventions in WindowNewMap)

Phase 2: Integrate RMG Generation (Commit 2)
Files: mapeditor/GenerateMapDialog.cpp (extend), new utility if needed

Implement RMG invocation:

Convert dialog inputs → CMapGenOptions structure (per lib/rmg/CMapGenOptions.h)
Call CMapGenerator with options to generate std::unique_ptr<CMap>
Reuse existing: CMapGenerator and related classes already exist; just instantiate and call generate()
Handle potential RMG failures gracefully (validation, error messages)
Show progress feedback (RMG can take time on larger maps)
Phase 3: Workflow Dialog (Commit 3)
File: mapeditor/GenerateMapDialog.cpp (extend)

After map generation, show a follow-up dialog offering:

Option A: Replace current map (with unsaved changes warning if applicable)
Option B: Open generated map in new editor window
Cancel: Discard generated map, return to editor
Implement via a simple QMessageBox or lightweight custom dialog.

Phase 4: Menu Integration (Commit 4)
Files: mapeditor/mainwindow.h/.cpp, mapeditor/mainwindow.ui

Add "Generate Map..." menu item to File menu
Wire signal → slot that opens GenerateMapDialog
Handle dialog result: proceed with workflow from Phase 3
Phase 5: Test & Refinement (Commit 5+)
Build and test dialog launches correctly from menu
Test parameter input validation (e.g., width bounds, player counts)
Generate a small map (36×36, 2 players) and verify it loads
Test replace-map workflow (verify undo/redo context)
Test new-window workflow (verify independent editor windows open)
Test larger/complex map generation (catch any RMG issues)
Critical Files & Patterns
Reuse these existing patterns:

Dialog pattern: Mirror WindowNewMap (mapeditor/windownewmap.h/cpp)

Uses .ui file for layout
Loads/saves settings with QSettings
Returns accepted/rejected dialog state
RMG integration: Reference /client/lobby/RandomMapTab.h

Shows how to build CMapGenOptions and call RMG
Error handling for invalid parameters
Map loading: mainwindow.cpp:openMap() (lines 591-625)

Shows how to integrate a generated map into editor
controller.setMap(mapData) and initializeMap() flow
Settings base class: mapeditor/mapsettings/AbstractSettings

If dialog grows into a tabbed settings panel later, reuse this pattern
New Files to Create
mapeditor/GenerateMapDialog.h (~100 lines)
mapeditor/GenerateMapDialog.cpp (~250 lines, includes RMG invocation)
mapeditor/GenerateMapDialog.ui (~150 lines, Qt Designer)
CMake Changes
Update mapeditor/CMakeLists.txt to include new source files in the target
Verification Plan
Build: cmake --build --preset=linux-gcc-release from mapeditor directory
Launch editor: Run ./build/bin/vcmiclient (editor), verify File menu has "Generate Map..." option
Generate small map: Open dialog, set 36×36, 2 players, normal water, normal difficulty, 1 level; click Generate
Verify output: Confirm generated map loads, displays in editor, can be edited and saved
Test workflows: Verify both "replace current" and "new window" paths work
Edge cases: Try invalid values (0 players, 256 width), verify graceful error handling
Incremental Commits Summary
GenerateMapDialog UI & Settings: Dialog structure, parameter widgets, settings persistence
RMG Integration: Connect dialog inputs to CMapGenOptions, call CMapGenerator
Workflow Dialog: Add post-generation choice dialog (replace vs new window)
Menu Integration: Add File → Generate Map menu item, wire signal
Testing & Polish: Validate, test all paths, refine error messages
Why this approach:

Leverages existing editor patterns (dialogs, settings, map loading) → minimal new code
Integrates cleanly with RMG system (already exposed via CMapGenOptions & CMapGenerator)
Incremental commits allow testing at each stage
Low blast radius: new dialog doesn't touch core editor logic
