#include "mapcontroller.h"

#include "../lib/GameConstants.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/Terrain.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/rmg/ObstaclePlacer.h"
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
}

MapController::~MapController()
{
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

void MapController::setMap(std::unique_ptr<CMap> cmap)
{
	_map = std::move(cmap);
	_scenes[0].reset(new MapScene(0));
	_scenes[1].reset(new MapScene(1));
	_miniscenes[0].reset(new MinimapScene(0));
	_miniscenes[1].reset(new MinimapScene(1));
	resetMapHandler();
	sceneForceUpdate();
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
	_mapHandler.reset(new MapHandler(_map.get()));
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
	for(auto * obj : _scenes[level]->selectionObjectsView.getSelection())
	{
		_map->getEditManager()->removeObject(obj);
		delete obj;
	}
	_scenes[level]->selectionObjectsView.clear();
	resetMapHandler();
	_scenes[level]->updateViews();
	
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

	resetMapHandler();
	_scenes[level]->updateViews();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectChange(int level)
{
	resetMapHandler();
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	
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
			_map->getEditManager()->moveObject(obj, pos);
		}
	}
		
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = 0;
	
	resetMapHandler();
	_scenes[level]->updateViews();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectCreate(int level)
{
	auto * newObj = _scenes[level]->selectionObjectsView.newObject;
	if(!newObj)
		return;
	_map->getEditManager()->insertObject(newObj);
	Initializer init(newObj);
	
	main->mapChanged();
}
