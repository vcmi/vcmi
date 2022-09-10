#include "mapcontroller.h"

#include "../lib/GameConstants.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/Terrain.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/rmg/ObstaclePlacer.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "mapview.h"
#include "scenelayer.h"
#include "maphandler.h"
#include "mainwindow.h"
#include "inspector.h"


MapController::MapController(MainWindow * m): main(m)
{
	_scenes[0].reset(new MapScene(0));
	_scenes[1].reset(new MapScene(1));
	_miniscenes[0].reset(new MinimapScene(0));
	_miniscenes[1].reset(new MinimapScene(1));
	connectScenes();
}

void MapController::connectScenes()
{
	for (int level = 0; level <= 1; level++)
	{
		//selections for both layers will be handled separately
		QObject::connect(_scenes[level].get(), &MapScene::selected, [this, level](bool anythingSelected)
		{
			main->onSelectionMade(level, anythingSelected);
		});
	}
}

MapController::~MapController()
{
}

const std::unique_ptr<CMap> & MapController::getMapUniquePtr() const
{
	return _map;
}

CMap * MapController::map()
{
	return _map.get();
}

MapHandler * MapController::mapHandler()
{
	return _mapHandler.get();
}

MapScene * MapController::scene(int level)
{
	return _scenes[level].get();
}

MinimapScene * MapController::miniScene(int level)
{
	return _miniscenes[level].get();
}

void MapController::repairMap()
{
	//fix owners for objects
	for(auto obj : _map->objects)
		if(obj->getOwner() == PlayerColor::UNFLAGGABLE)
			if(dynamic_cast<CGMine*>(obj.get()) ||
			   dynamic_cast<CGDwelling*>(obj.get()) ||
			   dynamic_cast<CGTownInstance*>(obj.get()) ||
			   dynamic_cast<CGGarrison*>(obj.get()) ||
			   dynamic_cast<CGShipyard*>(obj.get()) ||
			   dynamic_cast<CGHeroInstance*>(obj.get()))
				obj->tempOwner = PlayerColor::NEUTRAL;
	
	//there might be extra skills, arts and spells not imported from map
	if(VLC->skillh->getDefaultAllowed().size() > map()->allowedAbilities.size())
	{
		for(int i = map()->allowedAbilities.size(); i < VLC->skillh->getDefaultAllowed().size(); ++i)
			map()->allowedAbilities.push_back(false);
	}
	if(VLC->arth->getDefaultAllowed().size() > map()->allowedArtifact.size())
	{
		for(int i = map()->allowedArtifact.size(); i < VLC->arth->getDefaultAllowed().size(); ++i)
			map()->allowedArtifact.push_back(false);
	}
	if(VLC->spellh->getDefaultAllowed().size() > map()->allowedSpell.size())
	{
		for(int i = map()->allowedSpell.size(); i < VLC->spellh->getDefaultAllowed().size(); ++i)
			map()->allowedSpell.push_back(false);
	}
}

void MapController::setMap(std::unique_ptr<CMap> cmap)
{
	_map = std::move(cmap);
	
	repairMap();
	
	_scenes[0].reset(new MapScene(0));
	_scenes[1].reset(new MapScene(1));
	_miniscenes[0].reset(new MinimapScene(0));
	_miniscenes[1].reset(new MinimapScene(1));
	resetMapHandler();
	sceneForceUpdate();

	connectScenes();

	_map->getEditManager()->getUndoManager().setUndoCallback([this](bool allowUndo, bool allowRedo)
		{
			main->enableUndo(allowUndo);
			main->enableRedo(allowRedo);
		}
	);
}

void MapController::sceneForceUpdate()
{
	_scenes[0]->updateViews();
	_miniscenes[0]->updateViews();
	if(_map->twoLevel)
	{
		_scenes[1]->updateViews();
		_miniscenes[1]->updateViews();
	}
}

void MapController::sceneForceUpdate(int level)
{
	_scenes[level]->updateViews();
	_miniscenes[level]->updateViews();
}

void MapController::resetMapHandler()
{
	if(!_mapHandler)
		_mapHandler.reset(new MapHandler());
	_mapHandler->reset(map());
	_scenes[0]->initialize(*this);
	_scenes[1]->initialize(*this);
	_miniscenes[0]->initialize(*this);
	_miniscenes[1]->initialize(*this);
}

void MapController::commitTerrainChange(int level, const Terrain & terrain)
{
	std::vector<int3> v(_scenes[level]->selectionTerrainView.selection().begin(),
						_scenes[level]->selectionTerrainView.selection().end());
	if(v.empty())
		return;
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionTerrainView.draw();
	
	_map->getEditManager()->getTerrainSelection().setSelection(v);
	_map->getEditManager()->drawTerrain(terrain, &CRandomGenerator::getDefault());
	
	for(auto & t : v)
		_scenes[level]->terrainView.setDirty(t);
	_scenes[level]->terrainView.draw();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectErase(int level)
{
	auto selectedObjects = _scenes[level]->selectionObjectsView.getSelection();
	if (selectedObjects.size() > 1)
	{
		//mass erase => undo in one operation
		_map->getEditManager()->removeObjects(selectedObjects);
	}
	else if (selectedObjects.size() == 1)
	{
		_map->getEditManager()->removeObject(*selectedObjects.begin());
	}
	else //nothing to erase - shouldn't be here
	{
		return;
	}

	for (auto obj : selectedObjects)
	{
		//invalidate tiles under objects
		_mapHandler->invalidate(_mapHandler->getTilesUnderObject(obj));
	}

	_scenes[level]->selectionObjectsView.clear();
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

bool MapController::discardObject(int level) const
{
	_scenes[level]->selectionObjectsView.clear();
	if(_scenes[level]->selectionObjectsView.newObject)
	{
		delete _scenes[level]->selectionObjectsView.newObject;
		_scenes[level]->selectionObjectsView.newObject = nullptr;
		_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
		_scenes[level]->selectionObjectsView.selectionMode = 0;
		_scenes[level]->selectionObjectsView.draw();
		return true;
	}
	return false;
}

void MapController::createObject(int level, CGObjectInstance * obj) const
{
	_scenes[level]->selectionObjectsView.newObject = obj;
	_scenes[level]->selectionObjectsView.selectionMode = 2;
	_scenes[level]->selectionObjectsView.draw();
}

void MapController::commitObstacleFill(int level)
{
	auto selection = _scenes[level]->selectionTerrainView.selection();
	if(selection.empty())
		return;
	
	//split by zones
	std::map<Terrain, ObstacleProxy> terrainSelected;
	for(auto & t : selection)
	{
		auto tl = _map->getTile(t);
		if(tl.blocked || tl.visitable)
			continue;
		
		terrainSelected[tl.terType].blockedArea.add(t);
	}
	
	for(auto & sel : terrainSelected)
	{
		sel.second.collectPossibleObstacles(sel.first);
		sel.second.placeObstacles(_map.get(), CRandomGenerator::getDefault());
	}

	_mapHandler->invalidateObjects();
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionTerrainView.draw();
	_scenes[level]->objectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectChange(int level)
{	
	//for( auto * o : _scenes[level]->selectionObjectsView.getSelection())
		//_mapHandler->invalidate(o);
	
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}


void MapController::commitChangeWithoutRedraw()
{
	//DO NOT REDRAW
	main->mapChanged();
}

void MapController::commitObjectShiftOrCreate(int level)
{
	auto shift = _scenes[level]->selectionObjectsView.shift;
	if(shift.isNull())
		return;
	
	for(auto * obj : _scenes[level]->selectionObjectsView.getSelection())
	{
		int3 pos = obj->pos;
		pos.z = level;
		pos.x += shift.x(); pos.y += shift.y();
		
		if(obj == _scenes[level]->selectionObjectsView.newObject)
		{
			_scenes[level]->selectionObjectsView.newObject->pos = pos;
			commitObjectCreate(level);
		}
		else
		{
			auto prevPositions = _mapHandler->getTilesUnderObject(obj);
			_map->getEditManager()->moveObject(obj, pos);
			_mapHandler->invalidate(prevPositions);
			_mapHandler->invalidate(obj);
		}
	}
		
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = 0;
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectCreate(int level)
{
	auto * newObj = _scenes[level]->selectionObjectsView.newObject;
	if(!newObj)
		return;
	Initializer init(map(), newObj);
	
	_map->getEditManager()->insertObject(newObj);
	_mapHandler->invalidate(newObj);
	main->mapChanged();
}

void MapController::undo()
{
	_map->getEditManager()->getUndoManager().undo();
	resetMapHandler();
	sceneForceUpdate();
	main->mapChanged();
}

void MapController::redo()
{
	_map->getEditManager()->getUndoManager().redo();
	resetMapHandler();
	sceneForceUpdate();
	main->mapChanged();
}
