# Semi-Automatic Map Generation — Implementation Plan

## Project Goal

Add a "Generate Map" dialog to the VCMI map editor that lets the user configure RMG parameters, generate a map, and load it into the editor (replacing the current map or opening in a new window).

---

## Status Overview

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Dialog UI & settings persistence | ✅ Done (commit `739d12ab`) |
| 2 | RMG integration & progress bar | ✅ Done (commit `16b35bd2`) |
| 3 | Menu integration & map loading | ✅ Done (commit `3b172582a`) |
| 4 | Code quality & minor fixes | ✅ Done (commit `658b327cb`) |
| 5 | End-to-end testing | ⬜ TODO (blocked on game data) |
| — | **Extra: Object Density control** | ✅ Done (commit `9fc990614`) |
| — | **Extra: Connectivity validation** | ✅ Done (commit `9fc990614`) |

---

## Phase 1 — Dialog UI & Settings (DONE)

**Files:** `GenerateMapDialog.h`, `GenerateMapDialog.cpp`, `GenerateMapDialog.ui`

All parameter widgets are implemented: width/height spinboxes (36–144, step 4), player count, water content combo, monster strength combo, levels spinbox (1–2), and a "Replace Current Map" checkbox. Settings are saved/loaded via `QSettings` using `CLauncherDirs::getSettings`.

---

## Phase 2 — RMG Integration (DONE)

**Files:** `GenerateMapDialog.cpp`

`on_generateButton_clicked()` converts dialog inputs to `CMapGenOptions`, validates that at least one template matches (`getPossibleTemplates()`), runs generation on a background thread via `std::async`, and shows `GeneratorProgress` (which calls `qApp->processEvents()` in a loop to keep the UI responsive). On success, calls `accept()`; on failure, shows `QMessageBox::critical` and restores the dialog.

---

## Phase 3 — Menu Integration & Map Loading (DONE)

**Files to modify:** `mainwindow.ui`, `mainwindow.h`, `mainwindow.cpp`

This is the missing piece that makes the feature accessible from the editor.

### Step 3a — Add menu action to `mainwindow.ui`

In Qt Designer (or directly in the XML), add a new action to the File menu after the `actionNew` entry:

```xml
<addaction name="actionGenerateMap"/>
```

And declare the action:

```xml
<widget class="QAction" name="actionGenerateMap">
 <property name="text"><string>Generate Map...</string></property>
 <property name="shortcut"><string>Ctrl+Shift+G</string></property>
</widget>
```

### Step 3b — Declare the slot in `mainwindow.h`

```cpp
private slots:
    void on_actionGenerateMap_triggered();
```

### Step 3c — Implement the handler in `mainwindow.cpp`

Pattern mirrors `on_actionNew_triggered()` but uses `GenerateMapDialog`:

```cpp
void EditorMainWindow::on_actionGenerateMap_triggered()
{
    auto * dialog = new GenerateMapDialog(this);
    connect(dialog, &QDialog::accepted, this, [this, dialog]()
    {
        auto generatedMap = dialog->getGeneratedMap();
        if(dialog->shouldReplaceCurrentMap())
        {
            if(!getAnswerAboutUnsavedChanges())
                return;
            controller.setMap(std::move(generatedMap));
            initializeMap(true);
        }
        else
        {
            auto * newWindow = new EditorMainWindow();
            newWindow->controller.setMap(std::move(generatedMap));
            newWindow->initializeMap(true);
            newWindow->show();
        }
    });
}
```

> **Note:** `GenerateMapDialog` already calls `setAttribute(Qt::WA_DeleteOnClose)`, so the lambda must capture `dialog` only to call its getters — the object is still alive during the `accepted()` signal because `WA_DeleteOnClose` uses `deleteLater()`.

> **Note on new window:** Verify that `EditorMainWindow` has a no-argument constructor and that `initializeMap` can be called without a prior `openMap`. Check `mainwindow.cpp:286` constructor.

**Commit message:**
```
Add File > Generate Map menu item and wire GenerateMapDialog to editor

Adds actionGenerateMap to the File menu (Ctrl+G shortcut).
The slot opens GenerateMapDialog and on acceptance either replaces
the current map (with unsaved-changes guard) or loads the result
into a new editor window, based on the dialog checkbox.
```

**Testing protocol (Phase 3):**
1. Build: `cmake --build build-debug --target vcmieditor -j$(nproc)`
2. Launch editor with game data present
3. Verify "Generate Map..." appears in File menu with Ctrl+G shortcut
4. Open dialog: confirm all widgets are present and populated with saved defaults
5. Cancel: confirm no change to current map
6. Generate (replace): confirm unsaved-changes warning fires if map is dirty, then map is replaced
7. Generate (new window): confirm a second editor window opens with the generated map

---

## Phase 4 — Code Quality Fixes (DONE)

Small issues to clean up before the PR.

### Fix 1 — `getGeneratedMap()` should be non-`const`

`getGeneratedMap()` is declared `const` but uses `const_cast` + `std::move` to extract the stored map. This is misleading and unsafe. Change it to non-`const`:

**`GenerateMapDialog.h`:** remove `const` from declaration  
**`GenerateMapDialog.cpp`:** remove `const_cast`

```cpp
// before
std::unique_ptr<CMap> GenerateMapDialog::getGeneratedMap() const
{
    return std::move(const_cast<GenerateMapDialog*>(this)->generatedMap);
}

// after
std::unique_ptr<CMap> GenerateMapDialog::getGeneratedMap()
{
    return std::move(generatedMap);
}
```

**Commit message:**
```
Fix GenerateMapDialog::getGeneratedMap() const-correctness

The method moves data out of the object, which is inherently
a mutating operation. Remove const qualifier and const_cast.
```

**Testing protocol (Phase 4):** build compiles without warnings; no behavioral change.

---

## Extra Features (DONE, commit `9fc990614`)

Two features added beyond the original plan.

### Object Density control

New **Object Density** spinbox (1–5, default 3) in the dialog, with tooltip. Stored as `objectDensity` in `CMapGenOptions` (serialization version `RMG_OBJECT_DENSITY`). Applied in `TreasurePlacer`: both the object count and minimum spacing scale linearly with the multiplier — denser means more objects placed closer together.

**Files:** `GenerateMapDialog.ui`, `GenerateMapDialog.cpp`, `GenerateMapDialog.h`, `CMapGenOptions.h`, `CMapGenOptions.cpp`, `TreasurePlacer.cpp`, `ESerializationVersion.h`

### Connectivity validation

`CMapGenerator::validateConnectivity()` called after `fillZones()`. Runs a BFS from the first player town using `RmgMap` tile states (`FREE`/`USED` = walkable). If any other player town is unreachable, logs a warning:
```
RMG: player town at (x,y,z) is unreachable from the starting town.
RMG: 1 player town(s) unreachable — consider regenerating with a different seed.
```
Generation does not abort — the map is still returned — but the problem is surfaced in the log.

**Files:** `CMapGenerator.h`, `CMapGenerator.cpp`

---

## Phase 5 — End-to-End Testing (TODO, blocked on game data)

Requires HoMM3 data files in `~/.local/share/vcmi/`. See `mapeditor/README.md` for setup.

**Test matrix:**

| Scenario | Parameters | Expected result |
|----------|-----------|-----------------|
| Minimal map | 36×36, 2 players, no water, 1 level | Generates quickly (<5s), loads in editor |
| Underground | 72×72, 4 players, normal water, 2 levels | Both level tabs appear in editor |
| Replace current | Any, checkbox checked, map has unsaved changes | Unsaved-changes warning shown |
| New window | Any, checkbox unchecked | Second editor window opens independently |
| No matching template | Unusual combination (e.g., 8 players, 36×36) | Warning dialog, stays on Generate dialog |
| Settings persistence | Change all fields, cancel, reopen | Previous values restored |
| Large map | 144×144, 8 players, islands, 2 levels | Generation completes, no crash |

**Commit message (after all tests pass):**
```
Test and stabilize semi-automatic map generation

Verified full generate → load workflow for replace and new-window
paths. Confirmed settings persistence, unsaved-changes guard, and
graceful error on no matching template.
```

---

## Suggested Improvements (Future)

These are out of scope for the initial PR but worth tracking:

1. **Template selector** — expose the list of matching RMG templates (`getPossibleTemplates()`) in a combo box so the user can pick a specific one rather than getting a random selection.
2. **Seed control** — add an optional spin box for the RNG seed (default: random) to allow reproducible map generation.
3. **Player type per slot** — currently sets `humanOrCpuPlayerCount`; a future version could let the user specify which slots are human vs CPU, mirroring the lobby's random map tab (`client/lobby/RandomMapTab`).
4. **Async progress with cancel** — the current `std::async` + `processEvents()` loop blocks the main thread waiting for `.get()`. A proper Qt worker thread with a cancel button would be more robust for large maps.
5. **Map name field** — let the user give the generated map a name before loading it.
