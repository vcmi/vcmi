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

#include "../lib/GameConstants.h"
#include "../lib/entities/artifact/CArtifact.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/ObstacleProxy.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModDescription.h"
#include "../lib/TerrainHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "mapsettings/modsettings.h"
#include "mapview.h"
#include "scenelayer.h"
#include "maphandler.h"
#include "mainwindow.h"
#include "inspector/inspector.h"
#include "GameLibrary.h"
#include "PlayerSelectionDialog.h"

MapController::MapController(QObject * parent)
	: QObject(parent)
{
}

MapController::MapController(MainWindow * m): main(m)
{
	for(int i = 0; i < MAX_LEVELS; i++)
	{
		_scenes[i].reset(new MapScene(i));
		_miniscenes[i].reset(new MinimapScene(i));
	}
	connectScenes();
	_cb = std::make_unique<EditorCallback>(nullptr);
}

void MapController::connectScenes()
{
	for(int i = 0; i < MAX_LEVELS; i++)
	{
		//selections for both layers will be handled separately
		QObject::connect(_scenes[i].get(), &MapScene::selected, [this, i](bool anythingSelected)
		{
			main->onSelectionMade(i, anythingSelected);
		});
	}
}

MapController::~MapController()
{
	main = nullptr;
}

void MapController::setCallback(std::unique_ptr<EditorCallback> cb)
{
	_cb = std::move(cb);
}

EditorCallback * MapController::getCallback()
{
	return _cb.get();
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

void MapController::repairMap(CMap * map)
{
	if(!map)
		return;

	assert(map->cb);
	
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
	std::vector<CGObjectInstance*> allImpactedObjects;

	for (const auto & object : map->objects)
		allImpactedObjects.push_back(object.get());

	for (const auto & hero : map->getHeroesInPool())
		allImpactedObjects.push_back(map->tryGetFromHeroPool(hero));

	for(auto obj : allImpactedObjects)
	{
		if(obj == nullptr)
			continue;

		//fix flags
		if(obj->asOwnable() != nullptr && obj->getOwner() == PlayerColor::UNFLAGGABLE)
		{
			obj->tempOwner = PlayerColor::NEUTRAL;
		}
		//fix hero instance
		if(auto * nih = dynamic_cast<CGHeroInstance*>(obj))
		{
			// All heroes present on map or in prisons need to be allowed to rehire them after they are defeated

			// FIXME: How about custom scenarios where defeated hero cannot be hired again?

			map->allowedHeroes.insert(nih->getHeroTypeID());

			auto const & type = LIBRARY->heroh->objects[nih->subID];
			assert(type->heroClass);

			if(nih->ID == Obj::HERO) //not prison
				nih->appearance = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, type->heroClass->getIndex())->getTemplates().front();
			//fix spellbook
			if(nih->spellbookContainsSpell(SpellID::SPELLBOOK_PRESET))
			{
				nih->removeSpellFromSpellbook(SpellID::SPELLBOOK_PRESET);
				if(!nih->getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook)
					nih->putArtifact(ArtifactPosition::SPELLBOOK, map->createArtifact(ArtifactID::SPELLBOOK));
			}
			
		}
		//fix town instance
		if(auto * tnh = dynamic_cast<CGTownInstance*>(obj))
		{
			if(tnh->getTown())
			{
				for(const auto & building : tnh->getBuildings())
				{
					if(!tnh->getTown()->buildings.count(building))
						tnh->removeBuilding(building);
				}
				vstd::erase_if(tnh->forbiddenBuildings, [tnh](BuildingID bid)
				{
					return !tnh->getTown()->buildings.count(bid);
				});
			}
		}
		//fix spell scrolls
		if(auto * art = dynamic_cast<CGArtifact*>(obj))
		{
			if(art->ID == Obj::SPELL_SCROLL && !art->getArtifactInstance())
			{
				std::vector<SpellID> out;
				for(auto const & spell : LIBRARY->spellh->objects) //spellh size appears to be greater (?)
				{
					//if(map->isAllowedSpell(spell->id))
					{
						out.push_back(spell->id);
					}
				}
				auto a = map->createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
				art->setArtifactInstance(a);
			}
		}
		//fix mines 
		if(auto * mine = dynamic_cast<CGMine*>(obj))
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
	cmap->cb = _cb.get();
	_map = std::move(cmap);
	_cb->setMap(_map.get());
	
	repairMap();
	
	for(int i = 0; i < _map->mapLevels; i++)
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
	for (auto const & terrain : LIBRARY->terrainTypeHandler->objects)
	{
		auto terrainId = terrain->getId();
		_obstaclePainters[terrainId] = std::make_unique<EditorObstaclePlacer>(map);
		_obstaclePainters[terrainId]->collectPossibleObstacles(terrainId);
	}
}

void MapController::sceneForceUpdate()
{
	for(int i = 0; i < _map->mapLevels; i++)
	{
		_scenes[i]->updateViews();
		_miniscenes[i]->updateViews();
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
	for(int i = 0; i < MAX_LEVELS; i++)
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
		_clipboard.push_back(CMemorySerializer::deepCopy(*obj, _cb.get()));
	}
}

void MapController::pasteFromClipboard(int level)
{
	_scenes[level]->selectionObjectsView.clear();
	
	auto shift = int3::getDirs()[_clipboardShiftIndex++];
	if(_clipboardShiftIndex == int3::getDirs().size())
		_clipboardShiftIndex = 0;
	
	QStringList errors;
	for(auto & objUniquePtr : _clipboard)
	{
		auto obj = CMemorySerializer::deepCopyShared(*objUniquePtr, _cb.get());
		QString errorMsg;
		if(!canPlaceObject(obj.get(), errorMsg))
		{
			errors.push_back(std::move(errorMsg));
			continue;
		}
		auto newPos = objUniquePtr->pos + shift;
		if(_map->isInTheMap(newPos))
			obj->pos = newPos;
		obj->pos.z = level;

		obj->id = {};
		Initializer init(*this, obj.get(), defaultPlayer);
		_map->getEditManager()->insertObject(obj);
		_scenes[level]->selectionObjectsView.selectObject(obj.get());
		_mapHandler->invalidate(obj.get());
	}
	if(!errors.isEmpty())
		QMessageBox::warning(main, QObject::tr("Can't place object"), errors.join('\n'));
	
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
		_scenes[level]->selectionObjectsView.newObject.reset();
		_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
		_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
		_scenes[level]->selectionObjectsView.draw();
		return true;
	}
	return false;
}

void MapController::createObject(int level, std::shared_ptr<CGObjectInstance> obj) const
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
		if(tl.blocked() || tl.visitable())
			continue;
		
		auto terrain = tl.getTerrainID();
		_obstaclePainters[terrain]->addBlockedTile(t);
	}
	
	for(auto & sel : _obstaclePainters)
	{
		for(auto o : sel.second->placeObstacles(CRandomGenerator::getDefault()))
		{
			_mapHandler->invalidate(o.get());
			_scenes[level]->objectsView.setDirty(o.get());
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
	auto newObj = _scenes[level]->selectionObjectsView.newObject;
	if(!newObj)
		return;
	
	auto shift = _scenes[level]->selectionObjectsView.shift;
	
	int3 pos = newObj->pos;
	pos.z = level;
	pos.x += shift.x(); pos.y += shift.y();
	
	newObj->pos = pos;
	
	Initializer init(*this, newObj.get(), defaultPlayer);
	
	_map->getEditManager()->insertObject(newObj);
	_mapHandler->invalidate(newObj.get());
	_scenes[level]->objectsView.setDirty(newObj.get());
	
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.shift = QPoint(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
	_scenes[level]->objectsView.draw();
	_scenes[level]->selectionObjectsView.draw();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateViews();
	main->mapChanged();
}

bool MapController::canPlaceObject(const CGObjectInstance * newObj, QString & error) const
{	
	if(newObj->ID == Obj::GRAIL) //special case for grail
		return canPlaceGrail(newObj, error);
	
	if(defaultPlayer == PlayerColor::NEUTRAL && (newObj->ID == Obj::HERO || newObj->ID == Obj::RANDOM_HERO))
		return canPlaceHero(newObj, error);
	
	return checkRequiredMods(newObj, error);
}

bool MapController::canPlaceGrail(const CGObjectInstance * grailObj, QString & error) const
{
	assert(grailObj->ID == Obj::GRAIL);

	//find all objects of such type
	int objCounter = 0;
	for(auto o : _map->objects)
	{
		if(o->ID == grailObj->ID && o->subID == grailObj->subID)
		{
			++objCounter;
		}
	}

	if(objCounter >= 1)
	{
		error = QObject::tr("There can only be one grail object on the map.");
		return false; //maplimit reached
	}
	
	return true;
}

bool MapController::canPlaceHero(const CGObjectInstance * heroObj, QString & error) const
{
	assert(heroObj->ID == Obj::HERO || heroObj->ID == Obj::RANDOM_HERO);

	PlayerSelectionDialog dialog(main);
	if(dialog.exec() == QDialog::Accepted)
	{
		main->switchDefaultPlayer(dialog.getSelectedPlayer());
		return true;
	}
	
	error = tr("Hero %1 cannot be created as NEUTRAL.").arg(QString::fromStdString(heroObj->instanceName));
	return false;
}

bool MapController::checkRequiredMods(const CGObjectInstance * obj, QString & error) const
{
	ModCompatibilityInfo modsInfo;
	modAssessmentObject(obj, modsInfo);

	for(auto & mod : modsInfo)
	{
		if(!_map->mods.count(mod.first))
		{
			auto reply = QMessageBox::question(main,
				tr("Missing Required Mod"), modMissingMessage(mod.second) + tr("\n\nDo you want to do that now ?"),
				QMessageBox::Yes | QMessageBox::No);

			if(reply == QMessageBox::Yes)
			{
				_map->mods[mod.first] = LIBRARY->modh->getModInfo(mod.first).getVerificationInfo();
				Q_EMIT requestModsUpdate(modsInfo, true); // signal for MapSettings
			}
			else
			{
				error = tr("This object's mod is mandatory for map to remain valid.");
				return false;
			}
		}
	}
	return true;
}

QString MapController::modMissingMessage(const ModVerificationInfo & info)
{
	QString modName = QString::fromStdString(info.name);
	QString submod;
	if(!info.parent.empty())
		submod = QObject::tr(" (submod of %1)").arg(QString::fromStdString(info.parent));

	return QObject::tr("The mod '%1'%2, is required by an object on the map.\n"
		"Add it to the map's required mods in Map->General settings.",
		"should be consistent with Map->General menu entry translation")
		.arg(modName, submod);
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
	for(auto primaryID : LIBRARY->objtypeh->knownObjects())
	{
		for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(primaryID, secondaryID);
			auto modScope = handler->getModScope();
			if(modScope != "core")
				result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();
		}
	}
	return result;
}

void MapController::modAssessmentObject(const CGObjectInstance * obj, ModCompatibilityInfo & result)
{
	auto extractEntityMod = [&result](const auto & entity)
	{
		auto modScope = entity->getModScope();
		if(modScope != "core")
			result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();
	};

	auto handler = obj->getObjectHandler();
	auto modScope = handler->getModScope();
	if(modScope != "core")
		result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();

	if(obj->ID == Obj::TOWN || obj->ID == Obj::RANDOM_TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance *>(obj);
		for(const auto & spellID : town->possibleSpells)
		{
			if(spellID == SpellID::PRESET)
				continue;
			extractEntityMod(spellID.toEntity(LIBRARY));
		}

		for(const auto & spellID : town->obligatorySpells)
		{
			extractEntityMod(spellID.toEntity(LIBRARY));
		}
	}

	if(obj->ID == Obj::HERO || obj->ID == Obj::RANDOM_HERO)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(obj);
		for(const auto & spellID : hero->getSpellsInSpellbook())
		{
			if(spellID == SpellID::PRESET || spellID == SpellID::SPELLBOOK_PRESET)
				continue;
			extractEntityMod(spellID.toEntity(LIBRARY));
		}

		for(const auto & [_, slotInfo] : hero->artifactsWorn)
		{
			extractEntityMod(slotInfo.getArt()->getTypeId().toEntity(LIBRARY));
		}

		for(const auto & art : hero->artifactsInBackpack)
		{
			extractEntityMod(art.getArt()->getTypeId().toEntity(LIBRARY));
		}
	}

//TODO: terrains?
}

ModCompatibilityInfo MapController::modAssessmentMap(const CMap & map)
{
	ModCompatibilityInfo result;

	for(auto obj : map.objects)
	{
		modAssessmentObject(obj.get(), result);
	}
	return result;
}
