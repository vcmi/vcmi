/*
 * CMapEditManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../CRandomGenerator.h"
#include "MapEditUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapOperation;

/// The CMapUndoManager provides the functionality to save operations and undo/redo them.
class DLL_LINKAGE CMapUndoManager : boost::noncopyable
{
public:
	CMapUndoManager();
	~CMapUndoManager();

	void undo();
	void redo();
	void clearAll();

	/// The undo redo limit is a number which says how many undo/redo items can be saved. The default
	/// value is 10. If the value is 0, no undo/redo history will be maintained.
	
	/// FIXME: unlimited undo please
	int getUndoRedoLimit() const;
	void setUndoRedoLimit(int value);

	const CMapOperation * peekRedo() const;
	const CMapOperation * peekUndo() const;

	void addOperation(std::unique_ptr<CMapOperation> && operation); /// Client code does not need to call this method.

	//poor man's signal
	void setUndoCallback(std::function<void(bool, bool)> functor);

private:
	using TStack = std::list<std::unique_ptr<CMapOperation>>;

	void doOperation(TStack & fromStack, TStack & toStack, bool doUndo);
	const CMapOperation * peek(const TStack & stack) const;

	TStack undoStack;
	TStack redoStack;
	int undoRedoLimit;

	void onUndoRedo();
	std::function<void(bool allowUndo, bool allowRedo)> undoCallback;
};

/// The map edit manager provides functionality for drawing terrain and placing
/// objects on the map.
class DLL_LINKAGE CMapEditManager : boost::noncopyable
{
public:
	CMapEditManager(CMap * map);
	CMap * getMap();

	/// Clears the terrain. The free level is filled with water and the underground level with rock.
	void clearTerrain(CRandomGenerator * gen = nullptr);

	/// Draws terrain at the current terrain selection. The selection will be cleared automatically.
	void drawTerrain(TerrainId terType, CRandomGenerator * gen = nullptr);

	/// Draws roads at the current terrain selection. The selection will be cleared automatically.
	void drawRoad(RoadId roadType, CRandomGenerator * gen = nullptr);
	
	/// Draws rivers at the current terrain selection. The selection will be cleared automatically.
	void drawRiver(RiverId riverType, CRandomGenerator * gen = nullptr);

	void insertObject(CGObjectInstance * obj);
	void insertObjects(std::set<CGObjectInstance *> & objects);
	void moveObject(CGObjectInstance * obj, const int3 & pos);
	void removeObject(CGObjectInstance * obj);
	void removeObjects(std::set<CGObjectInstance *> & objects);

	CTerrainSelection & getTerrainSelection();
	CObjectSelection & getObjectSelection();

	CMapUndoManager & getUndoManager();

private:
	void execute(std::unique_ptr<CMapOperation> && operation);

	CMap * map;
	CMapUndoManager undoManager;
	CRandomGenerator gen;
	CTerrainSelection terrainSel;
	CObjectSelection objectSel;
};

VCMI_LIB_NAMESPACE_END
