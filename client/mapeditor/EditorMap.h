/*
 * EditorMap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Translator.h"

#include "../../lib/callback/EditorCallback.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/Point.h"
#include "../../lib/int3.h"
#include "../../lib/modding/ModVerificationInfo.h"

class CMap;
class CGObjectInstance;
class EditorObstaclePlacer;

using ModCompatibilityInfo = std::map<std::string, ModVerificationInfo>;

/// Receives notifications about the changes of the edited map
class IEditorMapListener
{
public:
	virtual ~IEditorMapListener() = default;

	/// Content of the map has changed, so it differs from the saved one
	virtual void onMapModified() = 0;
	/// Another map was loaded or the whole map may have changed, e.g. after undo
	virtual void onMapReloaded() = 0;
	/// Tiles or objects have changed, so anything derived from them, e.g. the minimap, is outdated
	virtual void onContentChanged() = 0;
	/// Object is about to leave its tiles, e.g. it is deleted or moved
	virtual void onObjectRemoving(const CGObjectInstance * object) = 0;
	/// Object has appeared on its tiles
	virtual void onObjectAdded(const CGObjectInstance * object) = 0;
	virtual void onSelectionChanged() = 0;
	virtual void onUndoStateChanged(bool canUndo, bool canRedo) = 0;
};

/// Map that is opened in the editor, together with the selection and the clipboard
class EditorMap
{
public:
	/// Levels cannot be added past this number
	static constexpr int MAX_LEVELS = 10;

	/// What has to be confirmed by the user before an object can be put on the map
	struct PlacementCheck
	{
		std::string error;
		std::vector<ModVerificationInfo> missingMods;
		bool needsPlayer = false;
	};

private:
	struct LevelState
	{
		std::set<int3> terrainSelection;
		std::set<CGObjectInstance *> objectSelection;
		std::set<const CGObjectInstance *> lockedObjects;
	};

	IEditorMapListener * listener = nullptr;
	std::unique_ptr<EditorCallback> callback;
	std::unique_ptr<CMap> map;
	std::optional<TranslatorOverlay> textsOverlay;
	std::vector<LevelState> levelStates;
	std::vector<std::unique_ptr<CGObjectInstance>> clipboard;
	int clipboardShiftIndex = 0;
	std::map<TerrainId, std::unique_ptr<EditorObstaclePlacer>> obstaclePainters;

	void initObstaclePainters();
	void notifyModified();
	void notifySelection();
	void dropDeadPointers();

public:
	/// Player that becomes the owner of the objects placed on the map
	PlayerColor defaultPlayer;

	/// Lets the user click an object on the map. Set by the window that shows the map.
	/// The callback gets nullptr if the user clicks something else
	std::function<void(const std::function<bool(const CGObjectInstance *)> &, const std::function<void(const CGObjectInstance *)> &)> pickObject;

	EditorMap();
	~EditorMap();

	void setListener(IEditorMapListener * newListener);

	EditorCallback * getCallback();
	bool hasMap() const;
	CMap * getMap();
	const std::unique_ptr<CMap> & getMapUniquePtr() const;
	int levels() const;

	/// Makes the map the edited one, repairs it and resets the history
	void setMap(std::unique_ptr<CMap> newMap);

	static void repairMap(CMap * map);

	void addLevel(const MapLayerId & layerType);
	bool canAddLevel() const;

	// selection
	const std::set<int3> & getTerrainSelection(int level) const;
	const std::set<CGObjectInstance *> & getObjectSelection(int level) const;
	bool isSelected(int level, const CGObjectInstance * object) const;
	bool isLocked(int level, const CGObjectInstance * object) const;
	void selectTiles(int level, const std::vector<int3> & tiles);
	void deselectTiles(int level, const std::vector<int3> & tiles);
	void clearTerrainSelection(int level);
	void selectObject(int level, CGObjectInstance * object);
	void deselectObject(int level, CGObjectInstance * object);
	void selectObjects(int level, const std::set<CGObjectInstance *> & objects);
	void clearObjectSelection(int level);
	void lockObjects(int level, const std::set<CGObjectInstance *> & objects);
	void lockAll(int level);
	void unlockAll(int level);
	const std::set<const CGObjectInstance *> & getLockedObjects(int level) const;

	/// Object under the tile, visitable objects first
	CGObjectInstance * findObjectAt(const int3 & tile, const CGObjectInstance * ignore = nullptr) const;
	/// Objects that cover any tile of the rectangle
	std::set<CGObjectInstance *> findObjectsIn(int level, const int3 & corner1, const int3 & corner2) const;

	// changes of the map, undoable ones go through CMapEditManager
	void commitTerrainChange(int level, const TerrainId & terrain);
	void commitRoadOrRiverChange(int level, ui8 type, bool isRoad);
	void commitObjectErase(int level);
	void commitObstacleFill(int level);
	void commitObjectShift(int level, const Point & shiftTiles);
	/// The bottom right tile of the object is placed at anchorTile
	void commitObjectCreate(int level, const std::shared_ptr<CGObjectInstance> & object, const int3 & anchorTile);
	/// Object has been edited in place, e.g. in the inspector
	void commitObjectChange(int level, CGObjectInstance * object);
	void commitChangeWithoutRedraw();

	/// Replaces the appearance of the selected objects with the one that fits the terrain below
	/// Returns the number of objects that could not be updated
	int updateAppearance(int level);

	void copyToClipboard(int level);
	/// Returns the errors of the objects that could not be pasted
	std::vector<std::string> pasteFromClipboard(int level);

	PlacementCheck checkPlacement(const CGObjectInstance * object) const;
	void registerRequiredMods(const std::vector<ModVerificationInfo> & mods);

	void undo();
	void redo();

	static void modAssessmentObject(const CGObjectInstance * obj, ModCompatibilityInfo & result);
	static ModCompatibilityInfo modAssessmentAll();
	static ModCompatibilityInfo modAssessmentMap(const CMap & map);
	/// Text that tells why the mod is needed by the map
	static std::string modMissingMessage(const ModVerificationInfo & info);

	/// Object gets the defaults the editor gives to every new object
	void initializeObject(CGObjectInstance * object);
};
