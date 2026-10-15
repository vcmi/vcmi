# Map editor port to the client

Status: written, **not compiled** (nothing was built or run). The old Qt `mapeditor/` is untouched.

## Entry points

* Main menu: text button `Map editor` at the top left of the main tab (`config/mainmenu.json`, shortcut `E`, command `editor`). The main menu config now accepts buttons with `text`, `width`, `height` instead of `name`.
* File menu of the editor: `Campaign editor`, `Template editor`, `Convert H3M maps`, `Convert H3C campaign`, `Convert extracted resources`.

## Layout

| Directory | Content |
| --- | --- |
| `client/widgets/` | Generic controls: `CTextButton`, `CCheckBox`, `CTextField`, `CNumberInput`, `CDropDown`, `CTabBar`, `CMenuBar`, `CPanel`, `CScrollPanel`, `CFormView`, `CSelectList`, `CListEditor`, `CMultiLineEdit`, `CResourceEditor`, `CPlayerSetEditor`, `CSetPicker`, `CItemValuePicker`, `UiTheme.h` |
| `client/windows/` | `CDialogWindow`, `CPopupMenu`, `CPromptWindows` (message, confirmation, text and number prompts), `CChoiceDialog`, `CFileDialog` |
| `client/mapeditor/` | Editor core (`EditorMap`, `EditorMapView`, `EditorMinimap`, `EditorObjectCatalog`, `EditorValidator`, `EditorFiles`, ...), `EditorWindow`, `EditorInspector`, new map, player settings, map settings, translations, converters, campaign window, template window |
| `client/mapeditor/mapsettings/` | Tabs of the map settings: general, abilities, spells and artifacts, heroes, victory, loss, events, rumors, mods |
| `client/mapeditor/inspector/` | Dialogs of the inspector: army, hero skills, spells and artifacts, portrait, town buildings, spells and events, rewards, quests, witch hut, scholar, shrine |
| `client/mapeditor/campaign/` | Campaign properties, scenario properties, starting bonus |
| `client/mapeditor/templates/` | Template diagram, selector dialogs, layout algorithm |
| `Mods/vcmi/Content/Sprites/editor/` | Toolbar icons copied from `mapeditor/icons` |

## Changes outside of the new files

* `lib/campaign/CampaignState.h`, `lib/campaign/CampaignRegions.h`, `lib/rmg/CRmgTemplate.h`, `lib/rmg/ObjectConfig.h`: friend declarations for `EditorCampaignWindow`, `EditorCampaignPropertiesDialog`, `EditorScenarioDialog`, `EditorTemplateWindow`, `EditorTemplateObjectDialog` (the Qt classes are friends only when `ENABLE_EDITOR` / `ENABLE_TEMPLATE_EDITOR` is defined).
* `client/mapView/`: `IMapRendererContext` got `debugOverlaysAllowed()` and `grailPosition()`, `MapRendererContextState` can be created from a `CMap`, so the game map renderer draws the edited map.
* `client/widgets/CTextInput`: `CFocusable::isAnyFocused()` (plain keys are not used as editor shortcuts while a field has the focus).
* `client/gui/Shortcut.h`, `ShortcutHandler.cpp`, `config/keyBindingsConfig.json`: editor shortcuts and `mainMenuEditor`.
* `client/mainmenu/CMainMenu.*`: command `editor`, text buttons.
* `client/CMakeLists.txt`: all new sources and headers.
* `Mods/vcmi/Content/config/translations/english.json`: keys `vcmi.editor.*`, `vcmi.ui.*`, `vcmi.mainMenu.editor`, `vcmi.keyBindings.keyBinding.editor*`.

## What is ported

Everything the Qt editor offers except the items below: new, open, recent, save, save as, image export, undo and redo, cut, copy, paste, erase, lock and unlock, fill with obstacles, grid and passability overlays, levels (add, layer, switch), update appearance, validation, translations, map settings, player settings, terrain, road and river painting with all tools (select, pan, brushes, area, lasso, line, fill), object catalog and placing, inspector with all property editors, campaign editor, template editor, H3M and H3C converters, resource conversion (split def files, PCX to PNG).

## Differences to the Qt editor

* The Qt scene layers are replaced by the renderer of the game, so the objects look as in the game.
* The Qt translation files (`.ts`) are not used. Editor texts are in `english.json`, other languages come through Weblate.
* Not ported: opening files by drag and drop, a map file as command line argument, the extraction options on the command line (the launcher keeps using the old editor for that; the new menu entry does the same conversion).
* Template editor: the card is drawn with primitives (colors, resource icons, numbers, three marks for the monster strength) instead of the SVG with gradients and icons.
* Bugs of the Qt code that were not copied: the spells limiter of rewards was loaded into the artifact list, the label position of campaign regions was never assigned, opening a template file was blocked by the validation of the previous content.
* Hidden state kept by the Qt dialogs (for example `CMapEvent` fields that the dialogs do not show) is preserved when an event is edited.

## Things to check at the first build

* The code was only checked by reading. Expect compile errors in the places that use lib API that was not run through a compiler.
* `CSelectList` with two check columns (town buildings and spells) has no column headers, a hint line explains them.
* `ENGINE->windows().createAndPushWindow<...>` with lambdas as arguments relies on the conversions to `std::function` in the constructors.
