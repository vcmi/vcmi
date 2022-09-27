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


#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../VCMI_Lib.h"
#include "CDrawRoadsOperation.h"
#include "../mapping/CMap.h"
#include "CMapOperation.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapUndoManager::CMapUndoManager() :
	undoRedoLimit(100000), //not sure if we ever need to bother about undo limit
	undoCallback([](bool, bool) {})
{
}

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
	undoCallback = functor;
	onUndoRedo(); //inform immediately
}

CMapEditManager::CMapEditManager(CMap * map)
	: map(map), terrainSel(map), objectSel(map)
{

}

CMap * CMapEditManager::getMap()
{
	return map;
}

void CMapEditManager::clearTerrain(CRandomGenerator * gen)
{
	execute(make_unique<CClearTerrainOperation>(map, gen ? gen : &(this->gen)));
}

void CMapEditManager::drawTerrain(TTerrainId terType, CRandomGenerator * gen)
{
	execute(make_unique<CDrawTerrainOperation>(map, terrainSel, terType, gen ? gen : &(this->gen)));
	terrainSel.clearSelection();
}

void CMapEditManager::drawRoad(TRoadId roadType, CRandomGenerator* gen)
{
	execute(make_unique<CDrawRoadsOperation>(map, terrainSel, roadType, gen ? gen : &(this->gen)));
	terrainSel.clearSelection();
}

void CMapEditManager::drawRiver(TRiverId riverType, CRandomGenerator* gen)
{
	execute(make_unique<CDrawRiversOperation>(map, terrainSel, riverType, gen ? gen : &(this->gen)));
	terrainSel.clearSelection();
}



void CMapEditManager::insertObject(CGObjectInstance * obj)
{
	execute(make_unique<CInsertObjectOperation>(map, obj));
}

void CMapEditManager::insertObjects(std::set<CGObjectInstance*>& objects)
{
	auto composedOperation = make_unique<CComposedOperation>(map);
	for (auto obj : objects)
	{
		composedOperation->addOperation(make_unique<CInsertObjectOperation>(map, obj));
	}
	execute(std::move(composedOperation));
}

void CMapEditManager::moveObject(CGObjectInstance * obj, const int3 & pos)
{
	execute(make_unique<CMoveObjectOperation>(map, obj, pos));
}

void CMapEditManager::removeObject(CGObjectInstance * obj)
{
	execute(make_unique<CRemoveObjectOperation>(map, obj));
}

void CMapEditManager::removeObjects(std::set<CGObjectInstance*> & objects)
{
	auto composedOperation = make_unique<CComposedOperation>(map);
	for (auto obj : objects)
	{
		composedOperation->addOperation(make_unique<CRemoveObjectOperation>(map, obj));
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
