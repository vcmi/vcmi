/*
 * CMapEditManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapEditManager.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../GameLibrary.h"
#include "CDrawRoadsOperation.h"
#include "CMapOperation.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

CMapUndoManager::CMapUndoManager() :
	undoRedoLimit(100000), //not sure if we ever need to bother about undo limit
	undoCallback([](bool, bool) {})
{
}

CMapUndoManager::~CMapUndoManager() = default;

void CMapUndoManager::undo()
{
	doOperation(undoStack, redoStack, true);

}

void CMapUndoManager::redo()
{
	doOperation(redoStack, undoStack, false);
}

void CMapUndoManager::clearAll()
{
	//FIXME: Will crash if an object was added twice to actions
	undoStack.clear();
	redoStack.clear();
	onUndoRedo();
}

int CMapUndoManager::getUndoRedoLimit() const
{
	return undoRedoLimit;
}

void CMapUndoManager::setUndoRedoLimit(int value)
{
	assert(value >= 0);
	undoStack.resize(std::min(undoStack.size(), static_cast<TStack::size_type>(value)));
	redoStack.resize(std::min(redoStack.size(), static_cast<TStack::size_type>(value)));
	onUndoRedo();
}

const CMapOperation * CMapUndoManager::peekRedo() const
{
	return peek(redoStack);
}

const CMapOperation * CMapUndoManager::peekUndo() const
{
	return peek(undoStack);
}

void CMapUndoManager::addOperation(std::unique_ptr<CMapOperation> && operation)
{
	undoStack.push_front(std::move(operation));
	if(undoStack.size() > undoRedoLimit) undoStack.pop_back();
	redoStack.clear();
	onUndoRedo();
}

void CMapUndoManager::doOperation(TStack & fromStack, TStack & toStack, bool doUndo)
{
	if(fromStack.empty()) return;
	auto & operation = fromStack.front();
	if(doUndo)
	{
		operation->undo();
	}
	else
	{
		operation->redo();
	}
	toStack.push_front(std::move(operation));
	fromStack.pop_front();
	onUndoRedo();
}

const CMapOperation * CMapUndoManager::peek(const TStack & stack) const
{
	if(stack.empty()) return nullptr;
	return stack.front().get();
}

void CMapUndoManager::onUndoRedo()
{
	//true if there's anything on the stack
	undoCallback((bool)peekUndo(), (bool)peekRedo());
}

void CMapUndoManager::setUndoCallback(std::function<void(bool, bool)> functor)
{
	undoCallback = std::move(functor);
	onUndoRedo(); //inform immediately
}

CMapEditManager::CMapEditManager(CMap * map)
	: map(map), terrainSel(map), objectSel(map)
{
}

CMapEditManager::~CMapEditManager() = default;

CMap * CMapEditManager::getMap()
{
	return map;
}

void CMapEditManager::clearTerrain(vstd::RNG * customGen)
{
	execute(std::make_unique<CClearTerrainOperation>(map, customGen ? customGen : gen.get()));
}

void CMapEditManager::drawTerrain(TerrainId terType, int decorationsPercentage, vstd::RNG * customGen)
{
	execute(std::make_unique<CDrawTerrainOperation>(map, terrainSel, terType, decorationsPercentage, customGen ? customGen : gen.get()));
	terrainSel.clearSelection();
}

void CMapEditManager::drawRoad(RoadId roadType, vstd::RNG* customGen)
{
	execute(std::make_unique<CDrawRoadsOperation>(map, terrainSel, roadType, customGen ? customGen : gen.get()));
	terrainSel.clearSelection();
}

void CMapEditManager::drawRiver(RiverId riverType, vstd::RNG* customGen)
{
	execute(std::make_unique<CDrawRiversOperation>(map, terrainSel, riverType, customGen ? customGen : gen.get()));
	terrainSel.clearSelection();
}

void CMapEditManager::insertObject(std::shared_ptr<CGObjectInstance> obj)
{
	execute(std::make_unique<CInsertObjectOperation>(map, obj));
}

void CMapEditManager::insertObjects(const std::set<std::shared_ptr<CGObjectInstance>>& objects)
{
	auto composedOperation = std::make_unique<CComposedOperation>(map);
	for(auto obj : objects)
	{
		composedOperation->addOperation(std::make_unique<CInsertObjectOperation>(map, obj));
	}
	execute(std::move(composedOperation));
}

void CMapEditManager::moveObject(CGObjectInstance * obj, const int3 & pos)
{
	execute(std::make_unique<CMoveObjectOperation>(map, obj, pos));
}

void CMapEditManager::removeObject(CGObjectInstance * obj)
{
	execute(std::make_unique<CRemoveObjectOperation>(map, obj));
}

void CMapEditManager::removeObjects(std::set<CGObjectInstance *> & objects)
{
	auto composedOperation = std::make_unique<CComposedOperation>(map);
	for(auto obj : objects)
	{
		composedOperation->addOperation(std::make_unique<CRemoveObjectOperation>(map, obj));
	}
	execute(std::move(composedOperation));
}

void CMapEditManager::execute(std::unique_ptr<CMapOperation> && operation)
{
	operation->execute();
	undoManager.addOperation(std::move(operation));
}

CTerrainSelection & CMapEditManager::getTerrainSelection()
{
	return terrainSel;
}

CObjectSelection & CMapEditManager::getObjectSelection()
{
	return objectSel;
}

CMapUndoManager & CMapEditManager::getUndoManager()
{
	return undoManager;
}

VCMI_LIB_NAMESPACE_END
