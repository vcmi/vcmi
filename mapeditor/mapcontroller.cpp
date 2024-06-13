/*
 * mapcontroller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapcontroller.h"

#include "../lib/ArtifactUtils.h"
#include "../lib/GameConstants.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/ObstacleProxy.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/CModInfo.h"
#include "../lib/TerrainHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "mapview.h"
#include "scenelayer.h"
#include "maphandler.h"
#include "mainwindow.h"
#include "inspector/inspector.h"
#include "VCMI_Lib.h"

MapController::MapController(MainWindow * m): main(m)
{
	for(int i : {0, 1})
	{
		_scenes[i].reset(new MapScene(i));
		_miniscenes[i].reset(new MinimapScene(i));
	}
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
	main = nullptr;
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
	repairMap(map());
}

void MapController::repairMap(CMap * map) const
{
	if(!map)
		return;
	
	//make sure events/rumors has name to have proper identifiers
	int emptyNameId = 1;
	for(auto & e : map->events)
		if(e.name.empty())
			e.name = "event_" + std::to_string(emptyNameId++);
	emptyNameId = 1;
	for(auto & e : map->rumors)
		if(e.name.empty())
			e.name = "rumor_" + std::to_string(emptyNameId++);
	
	//fix owners for objects
	auto allImpactedObjects(map->objects);
	allImpactedObjects.insert(allImpactedObjects.end(), map->predefinedHeroes.begin(), map->predefinedHeroes.end());
	for(auto obj : allImpactedObjects)
	{
		//setup proper names (hero name will be fixed later
		if(obj->ID != Obj::HERO && obj->ID != Obj::PRISON && (obj->typeName.empty() || obj->subTypeName.empty()))
		{
			auto handler = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
			obj->typeName = handler->getTypeName();
			obj->subTypeName = handler->getSubTypeName();
		}
		//fix flags
		if(obj->getOwner() == PlayerColor::UNFLAGGABLE)
		{
			if(dynamic_cast<CGMine*>(obj.get()) ||
			   dynamic_cast<CGDwelling*>(obj.get()) ||
			   dynamic_cast<CGTownInstance*>(obj.get()) ||
			   dynamic_cast<CGGarrison*>(obj.get()) ||
			   dynamic_cast<CGShipyard*>(obj.get()) ||
			   dynamic_cast<CGLighthouse*>(obj.get()) ||
			   dynamic_cast<CGHeroInstance*>(obj.get()))
				obj->tempOwner = PlayerColor::NEUTRAL;
		}
		//fix hero instance
		if(auto * nih = dynamic_cast<CGHeroInstance*>(obj.get()))
		{
			// All heroes present on map or in prisons need to be allowed to rehire them after they are defeated

			// FIXME: How about custom scenarios where defeated hero cannot be hired again?

			map->allowedHeroes.insert(nih->getHeroType());

			auto type = VLC->heroh->objects[nih->subID];
			assert(type->heroClass);
			//TODO: find a way to get proper type name
			if(obj->ID == Obj::HERO)
			{
				nih->typeName = "hero";
				nih->subTypeName = type->heroClass->getJsonKey();
			}
			if(obj->ID == Obj::PRISON)
			{
				nih->typeName = "prison";
				nih->subTypeName = "prison";
				nih->subID = 0;
			}
			
			if(obj->ID != Obj::RANDOM_HERO)
				nih->type = type.get();
			
			if(nih->ID == Obj::HERO) //not prison
				nih->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, type->heroClass->getIndex())->getTemplates().front();
			//fix spellbook
			if(nih->spellbookContainsSpell(SpellID::SPELLBOOK_PRESET))
			{
				nih->removeSpellFromSpellbook(SpellID::SPELLBOOK_PRESET);
				if(!nih->getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook)
					nih->putArtifact(ArtifactPosition::SPELLBOOK, ArtifactUtils::createNewArtifactInstance(ArtifactID::SPELLBOOK));
			}
			
		}
		//fix town instance
		if(auto * tnh = dynamic_cast<CGTownInstance*>(obj.get()))
		{
			if(tnh->getTown())
			{
				vstd::erase_if(tnh->builtBuildings, [tnh](BuildingID bid)
				{
					return !tnh->getTown()->buildings.count(bid);
				});
				vstd::erase_if(tnh->forbiddenBuildings, [tnh](BuildingID bid)
				{
					return !tnh->getTown()->buildings.count(bid);
				});
			}
		}
		//fix spell scrolls
		if(auto * art = dynamic_cast<CGArtifact*>(obj.get()))
		{
			if(art->ID == Obj::SPELL_SCROLL && !art->storedArtifact)
			{
				std::vector<SpellID> out;
				for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
				{
					//if(map->isAllowedSpell(spell->id))
					{
						out.push_back(spell->id);
					}
				}
				auto a = ArtifactUtils::createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
				art->storedArtifact = a;
			}
		}
		//fix mines 
		if(auto * mine = dynamic_cast<CGMine*>(obj.get()))
		{
			if(!mine->isAbandoned())
			{
				mine->producedResource = GameResID(mine->subID);
				mine->producedQuantity = mine->defaultResProduction();
			}
		}
	}
}

void MapController::setMap(std::unique_ptr<CMap> cmap)
{
	_map = std::move(cmap);
	
	repairMap();
	
	for(int i : {0, 1})
	{
		_scenes[i].reset(new MapScene(i));
		_miniscenes[i].reset(new MinimapScene(i));
	}
	resetMapHandler();
	sceneForceUpdate();

	connectScenes();

	_map->getEditManager()->getUndoManager().setUndoCallback([this](bool allowUndo, bool allowRedo)
		{
			if(!main)
				return;
			main->enableUndo(allowUndo);
			main->enableRedo(allowRedo);
		}
	);
	_map->getEditManager()->getUndoManager().clearAll();

	initObstaclePainters(_map.get());
}

void MapController::initObstaclePainters(CMap * map)
{
	for (auto terrain : VLC->terrainTypeHandler->objects)
	{
		auto terrainId = terrain->getId();
		_obstaclePainters[terrainId] = std::make_unique<EditorObstaclePlacer>(map);
		_obstaclePainters[terrainId]->collectPossibleObstacles(terrainId);
	}
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
	for(int i : {0, 1})
	{
		_scenes[i]->initialize(*this);
		_miniscenes[i]->initialize(*this);
	}
}

void MapController::commitTerrainChange(int level, const TerrainId & terrain)
{
	static const int terrainDecorationPercentageLevel = 10;

	std::vector<int3> v(_scenes[level]->selectionTerrainView.selection().begin(),
						_scenes[level]->selectionTerrainView.selection().end());
	if(v.empty())
		return;
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionTerrainView.draw();
	
	_map->getEditManager()->getTerrainSelection().setSelection(v);
	_map->getEditManager()->drawTerrain(terrain, terrainDecorationPercentageLevel, &CRandomGenerator::getDefault());
	
	for(auto & t : v)
		_scenes[level]->terrainView.setDirty(t);
	_scenes[level]->terrainView.draw();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitRoadOrRiverChange(int level, ui8 type, bool isRoad)
{
	std::vector<int3> v(_scenes[level]->selectionTerrainView.selection().begin(),
						_scenes[level]->selectionTerrainView.selection().end());
	if(v.empty())
		return;
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionTerrainView.draw();
	
	_map->getEditManager()->getTerrainSelection().setSelection(v);
	if(isRoad)
		_map->getEditManager()->drawRoad(RoadId(type), &CRandomGenerator::getDefault());
	else
		_map->getEditManager()->drawRiver(RiverId(type), &CRandomGenerator::getDefault());
	
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

	for (auto & obj : selectedObjects)
	{
		//invalidate tiles under objects
		_mapHandler->removeObject(obj);
		_scenes[level]->objectsView.setDirty(obj);
	}

	_scenes[level]->selectionObjectsView.clear();
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::copyToClipboard(int level)
{
	_clipboard.clear();
	_clipboardShiftIndex = 0;
	auto selectedObjects = _scenes[level]->selectionObjectsView.getSelection();
	for(auto * obj : selectedObjects)
	{
		assert(obj->pos.z == level);
		_clipboard.push_back(CMemorySerializer::deepCopy(*obj));
	}
}

void MapController::pasteFromClipboard(int level)
{
	_scenes[level]->selectionObjectsView.clear();
	
	auto shift = int3::getDirs()[_clipboardShiftIndex++];
	if(_clipboardShiftIndex == int3::getDirs().size())
		_clipboardShiftIndex = 0;
	
	for(auto & objUniquePtr : _clipboard)
	{
		auto * obj = CMemorySerializer::deepCopy(*objUniquePtr).release();
		auto newPos = objUniquePtr->pos + shift;
		if(_map->isInTheMap(newPos))
			obj->pos = newPos;
		obj->pos.z = level;
		
		Initializer init(obj, defaultPlayer);
		_map->getEditManager()->insertObject(obj);
		_scenes[level]->selectionObjectsView.selectObject(obj);
		_mapHandler->invalidate(obj);
	}
	
	_scenes[level]->objectsView.draw();
	_scenes[level]->passabilityView.update();
	_scenes[level]->selectionObjectsView.draw();
	
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
		_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
		_scenes[level]->selectionObjectsView.draw();
		return true;
	}
	return false;
}

void MapController::createObject(int level, CGObjectInstance * obj) const
{
	_scenes[level]->selectionObjectsView.newObject = obj;
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
	_scenes[level]->selectionObjectsView.draw();
}

void MapController::commitObstacleFill(int level)
{
	auto selection = _scenes[level]->selectionTerrainView.selection();
	if(selection.empty())
		return;
	
	//split by zones
	for (auto & painter : _obstaclePainters)
	{
		painter.second->clearBlockedArea();
	}
	
	for(auto & t : selection)
	{
		auto tl = _map->getTile(t);
		if(tl.blocked || tl.visitable)
			continue;
		
		auto terrain = tl.terType->getId();
		_obstaclePainters[terrain]->addBlockedTile(t);
	}
	
	for(auto & sel : _obstaclePainters)
	{
		for(auto * o : sel.second->placeObstacles(CRandomGenerator::getDefault()))
		{
			_mapHandler->invalidate(o);
			_scenes[level]->objectsView.setDirty(o);
		}
	}
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionTerrainView.draw();
	_scenes[level]->objectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

void MapController::commitObjectChange(int level)
{	
	for( auto * o : _scenes[level]->selectionObjectsView.getSelection())
		_scenes[level]->objectsView.setDirty(o);
	
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

void MapController::commitObjectShift(int level)
{
	auto shift = _scenes[level]->selectionObjectsView.shift;
	bool makeShift = !shift.isNull();
	if(makeShift)
	{
		for(auto * obj : _scenes[level]->selectionObjectsView.getSelection())
		{
			int3 pos = obj->pos;
			pos.z = level;
			pos.x += shift.x(); pos.y += shift.y();
			
			_scenes[level]->objectsView.setDirty(obj); //set dirty before movement
			_map->getEditManager()->moveObject(obj, pos);
			_mapHandler->invalidate(obj);
		}
	}
	
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
	
	if(makeShift)
	{
		_scenes[level]->objectsView.draw();
		_scenes[level]->selectionObjectsView.draw();
		_scenes[level]->passabilityView.update();
		
		_miniscenes[level]->updateViews();
		main->mapChanged();
	}
}

void MapController::commitObjectCreate(int level)
{
	auto * newObj = _scenes[level]->selectionObjectsView.newObject;
	if(!newObj)
		return;
	
	auto shift = _scenes[level]->selectionObjectsView.shift;
	
	int3 pos = newObj->pos;
	pos.z = level;
	pos.x += shift.x(); pos.y += shift.y();
	
	newObj->pos = pos;
	
	Initializer init(newObj, defaultPlayer);
	
	_map->getEditManager()->insertObject(newObj);
	_mapHandler->invalidate(newObj);
	_scenes[level]->objectsView.setDirty(newObj);
	
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

bool MapController::canPlaceObject(int level, CGObjectInstance * newObj, QString & error) const
{
	//find all objects of such type
	int objCounter = 0;
	for(auto o : _map->objects)
	{
		if(o->ID == newObj->ID && o->subID == newObj->subID)
		{
			++objCounter;
		}
	}
	
	if(newObj->ID == Obj::GRAIL && objCounter >= 1) //special case for grail
	{
		auto typeName = QString::fromStdString(newObj->typeName);
		auto subTypeName = QString::fromStdString(newObj->subTypeName);
		error = QString("There can be only one grail object on the map");
		return false; //maplimit reached
	}
	
	if(defaultPlayer == PlayerColor::NEUTRAL && (newObj->ID == Obj::HERO || newObj->ID == Obj::RANDOM_HERO))
	{
		error = "Hero cannot be created as NEUTRAL";
		return false;
	}
	
	return true;
}

void MapController::undo()
{
	_map->getEditManager()->getUndoManager().undo();
	resetMapHandler();
	sceneForceUpdate(); //TODO: use smart invalidation (setDirty)
	main->mapChanged();
}

void MapController::redo()
{
	_map->getEditManager()->getUndoManager().redo();
	resetMapHandler();
	sceneForceUpdate(); //TODO: use smart invalidation (setDirty)
	main->mapChanged();
}

ModCompatibilityInfo MapController::modAssessmentAll()
{
	ModCompatibilityInfo result;
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			auto modName = QString::fromStdString(handler->getJsonKey()).split(":").at(0).toStdString();
			if(modName != "core")
				result[modName] = VLC->modh->getModInfo(modName).getVerificationInfo();
		}
	}
	return result;
}

ModCompatibilityInfo MapController::modAssessmentMap(const CMap & map)
{
	ModCompatibilityInfo result;
	for(auto obj : map.objects)
	{
		if(obj->ID == Obj::HERO)
			continue; //stub! 
		
		auto handler = obj->getObjectHandler();
		auto modName = QString::fromStdString(handler->getJsonKey()).split(":").at(0).toStdString();
		if(modName != "core")
			result[modName] = VLC->modh->getModInfo(modName).getVerificationInfo();
	}
	//TODO: terrains?
	return result;
}
