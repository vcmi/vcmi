/*
 * EditorMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMap.h"

#include "EditorTexts.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/CommonConstructors.h"
#include "../../lib/mapObjects/FlaggableMapObject.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/ObstacleProxy.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModDescription.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

EditorMap::EditorMap()
	: callback(std::make_unique<EditorCallback>(nullptr))
	, defaultPlayer(PlayerColor::NEUTRAL)
{
}

EditorMap::~EditorMap() = default;

void EditorMap::setListener(IEditorMapListener * newListener)
{
	listener = newListener;
}

EditorCallback * EditorMap::getCallback()
{
	return callback.get();
}

bool EditorMap::hasMap() const
{
	return map != nullptr;
}

CMap * EditorMap::getMap()
{
	return map.get();
}

const std::unique_ptr<CMap> & EditorMap::getMapUniquePtr() const
{
	return map;
}

int EditorMap::levels() const
{
	return map ? map->levels() : 0;
}

void EditorMap::notifyModified()
{
	if(listener)
		listener->onMapModified();
}

void EditorMap::notifySelection()
{
	if(listener)
		listener->onSelectionChanged();
}

void EditorMap::initObstaclePainters()
{
	obstaclePainters.clear();

	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
	{
		auto terrainId = terrain->getId();
		obstaclePainters[terrainId] = std::make_unique<EditorObstaclePlacer>(map.get());
		obstaclePainters[terrainId]->collectPossibleObstacles(terrainId);
	}
}

void EditorMap::setMap(std::unique_ptr<CMap> newMap)
{
	textsOverlay.reset();

	newMap->cb = callback.get();
	map = std::move(newMap);
	callback->setMap(map.get());
	textsOverlay.emplace(map->texts);

	repairMap(map.get());

	levelStates.clear();
	levelStates.resize(map->levels());

	map->getEditManager()->getUndoManager().setUndoCallback([this](bool allowUndo, bool allowRedo)
	{
		if(listener)
			listener->onUndoStateChanged(allowUndo, allowRedo);
	});
	map->getEditManager()->getUndoManager().clearAll();

	initObstaclePainters();

	if(listener)
	{
		listener->onUndoStateChanged(false, false);
		listener->onMapReloaded();
	}
}

void EditorMap::repairMap(CMap * map)
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

	for(const auto & object : map->objects)
		allImpactedObjects.push_back(object.get());

	for(const auto & hero : map->getHeroesInPool())
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
					out.push_back(spell->id);
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
				if(mine->getResourceHandler()->getResourceType() == GameResID::NONE) // fallback
					mine->producedResource = GameResID(mine->subID);
				else
					mine->producedResource = mine->getResourceHandler()->getResourceType();
				mine->producedQuantity = mine->defaultResProduction();
			}
		}
	}
}

void EditorMap::addLevel(const MapLayerId & layerType)
{
	if(!canAddLevel())
		return;

	map->addLevel(layerType);
	levelStates.resize(map->levels());

	notifyModified();
	if(listener)
		listener->onMapReloaded();
}

bool EditorMap::canAddLevel() const
{
	return map && map->levels() < MAX_LEVELS;
}

const std::set<int3> & EditorMap::getTerrainSelection(int level) const
{
	return levelStates.at(level).terrainSelection;
}

const std::set<CGObjectInstance *> & EditorMap::getObjectSelection(int level) const
{
	return levelStates.at(level).objectSelection;
}

bool EditorMap::isSelected(int level, const CGObjectInstance * object) const
{
	return levelStates.at(level).objectSelection.count(const_cast<CGObjectInstance *>(object)) != 0;
}

bool EditorMap::isLocked(int level, const CGObjectInstance * object) const
{
	return levelStates.at(level).lockedObjects.count(object) != 0;
}

const std::set<const CGObjectInstance *> & EditorMap::getLockedObjects(int level) const
{
	return levelStates.at(level).lockedObjects;
}

void EditorMap::selectTiles(int level, const std::vector<int3> & tiles)
{
	auto & selection = levelStates.at(level).terrainSelection;
	for(const auto & tile : tiles)
		if(map->isInTheMap(tile))
			selection.insert(tile);

	notifySelection();
}

void EditorMap::deselectTiles(int level, const std::vector<int3> & tiles)
{
	auto & selection = levelStates.at(level).terrainSelection;
	for(const auto & tile : tiles)
		selection.erase(tile);

	notifySelection();
}

void EditorMap::clearTerrainSelection(int level)
{
	if(levelStates.at(level).terrainSelection.empty())
		return;

	levelStates.at(level).terrainSelection.clear();
	notifySelection();
}

void EditorMap::selectObject(int level, CGObjectInstance * object)
{
	levelStates.at(level).objectSelection.insert(object);
	notifySelection();
}

void EditorMap::deselectObject(int level, CGObjectInstance * object)
{
	levelStates.at(level).objectSelection.erase(object);
	notifySelection();
}

void EditorMap::selectObjects(int level, const std::set<CGObjectInstance *> & objects)
{
	levelStates.at(level).objectSelection.insert(objects.begin(), objects.end());
	notifySelection();
}

void EditorMap::clearObjectSelection(int level)
{
	if(levelStates.at(level).objectSelection.empty())
		return;

	levelStates.at(level).objectSelection.clear();
	notifySelection();
}

void EditorMap::lockObjects(int level, const std::set<CGObjectInstance *> & objects)
{
	auto & state = levelStates.at(level);
	state.lockedObjects.insert(objects.begin(), objects.end());

	for(auto * object : objects)
		state.objectSelection.erase(object);

	notifySelection();
	if(listener)
		listener->onMapReloaded();
}

void EditorMap::lockAll(int level)
{
	std::set<CGObjectInstance *> all;
	for(const auto & object : map->objects)
		if(object && object->pos.z == level)
			all.insert(object.get());

	lockObjects(level, all);
}

void EditorMap::unlockAll(int level)
{
	levelStates.at(level).lockedObjects.clear();

	notifySelection();
	if(listener)
		listener->onMapReloaded();
}

CGObjectInstance * EditorMap::findObjectAt(const int3 & tile, const CGObjectInstance * ignore) const
{
	if(!map || !map->isInTheMap(tile))
		return nullptr;

	const auto & locked = levelStates.at(tile.z).lockedObjects;
	auto usable = [&](const CGObjectInstance * object)
	{
		return object && object != ignore && !locked.count(object);
	};

	const TerrainTile & terrainTile = map->getTile(tile);

	//visitable is most important
	for(const auto & id : terrainTile.visitableObjects)
	{
		auto * object = map->getObject(id);
		if(usable(object) && object->visitableAt(tile))
			return object;
	}

	//if not visitable tile - try to get blocked
	for(const auto & id : terrainTile.blockingObjects)
	{
		auto * object = map->getObject(id);
		if(usable(object) && object->blockingAt(tile))
			return object;
	}

	//finally, we can take any object
	for(const auto & object : map->objects)
	{
		if(object && object->pos.z == tile.z && usable(object.get()) && object->coveringAt(tile))
			return object.get();
	}

	return nullptr;
}

std::set<CGObjectInstance *> EditorMap::findObjectsIn(int level, const int3 & corner1, const int3 & corner2) const
{
	std::set<CGObjectInstance *> result;
	if(!map)
		return result;

	const int left = std::min(corner1.x, corner2.x);
	const int right = std::max(corner1.x, corner2.x);
	const int top = std::min(corner1.y, corner2.y);
	const int bottom = std::max(corner1.y, corner2.y);
	const auto & locked = levelStates.at(level).lockedObjects;

	for(const auto & object : map->objects)
	{
		if(!object || object->pos.z != level || locked.count(object.get()))
			continue;

		const int3 anchor = object->anchorPos();
		const int x1 = std::max(left, anchor.x - object->getWidth() + 1);
		const int x2 = std::min(right, anchor.x);
		const int y1 = std::max(top, anchor.y - object->getHeight() + 1);
		const int y2 = std::min(bottom, anchor.y);

		bool covered = false;
		for(int y = y1; y <= y2 && !covered; ++y)
			for(int x = x1; x <= x2 && !covered; ++x)
				covered = object->coveringAt(int3(x, y, level));

		if(covered)
			result.insert(object.get());
	}
	return result;
}

void EditorMap::commitTerrainChange(int level, const TerrainId & terrain)
{
	static const int terrainDecorationPercentageLevel = 10;

	auto & selection = levelStates.at(level).terrainSelection;
	if(selection.empty())
		return;

	std::vector<int3> tiles(selection.begin(), selection.end());
	selection.clear();

	map->getEditManager()->getTerrainSelection().setSelection(tiles);
	map->getEditManager()->drawTerrain(terrain, terrainDecorationPercentageLevel, &CRandomGenerator::getDefault());

	notifySelection();
	if(listener)
		listener->onContentChanged();
	notifyModified();
}

void EditorMap::commitRoadOrRiverChange(int level, ui8 type, bool isRoad)
{
	auto & selection = levelStates.at(level).terrainSelection;
	if(selection.empty())
		return;

	std::vector<int3> tiles(selection.begin(), selection.end());
	selection.clear();

	map->getEditManager()->getTerrainSelection().setSelection(tiles);
	if(isRoad)
		map->getEditManager()->drawRoad(RoadId(type), &CRandomGenerator::getDefault());
	else
		map->getEditManager()->drawRiver(RiverId(type), &CRandomGenerator::getDefault());

	notifySelection();
	if(listener)
		listener->onContentChanged();
	notifyModified();
}

void EditorMap::commitObjectErase(int level)
{
	auto & state = levelStates.at(level);
	std::set<CGObjectInstance *> selectedObjects = state.objectSelection;
	if(selectedObjects.empty())
		return;

	if(listener)
		for(auto * object : selectedObjects)
			listener->onObjectRemoving(object);

	if(selectedObjects.size() > 1)
	{
		//mass erase => undo in one operation
		map->getEditManager()->removeObjects(selectedObjects);
	}
	else
	{
		map->getEditManager()->removeObject(*selectedObjects.begin());
	}

	state.objectSelection.clear();
	notifySelection();
	if(listener)
		listener->onContentChanged();
	notifyModified();
}

void EditorMap::commitObstacleFill(int level)
{
	auto & selection = levelStates.at(level).terrainSelection;
	if(selection.empty())
		return;

	//split by zones
	for(auto & painter : obstaclePainters)
		painter.second->clearBlockedArea();

	for(const auto & tile : selection)
	{
		const auto & terrainTile = map->getTile(tile);
		if(terrainTile.blocked() || terrainTile.visitable())
			continue;

		obstaclePainters.at(terrainTile.getTerrainID())->addBlockedTile(tile);
	}

	for(auto & painter : obstaclePainters)
	{
		for(const auto & obstacle : painter.second->placeObstacles(CRandomGenerator::getDefault()))
		{
			if(listener)
				listener->onObjectAdded(obstacle.get());
		}
	}

	selection.clear();
	notifySelection();
	if(listener)
		listener->onContentChanged();
	notifyModified();
}

void EditorMap::commitObjectShift(int level, const Point & shiftTiles)
{
	if(shiftTiles.x == 0 && shiftTiles.y == 0)
		return;

	const std::set<CGObjectInstance *> movedObjects = levelStates.at(level).objectSelection;
	for(auto * obj : movedObjects)
	{
		int3 pos = obj->pos;
		pos.z = level;
		pos.x += shiftTiles.x;
		pos.y += shiftTiles.y;

		if(listener)
			listener->onObjectRemoving(obj);

		map->getEditManager()->moveObject(obj, pos);

		if(listener)
			listener->onObjectAdded(obj);
	}

	if(listener)
		listener->onContentChanged();
	notifyModified();
}

void EditorMap::commitObjectCreate(int level, const std::shared_ptr<CGObjectInstance> & object, const int3 & anchorTile)
{
	if(!object)
		return;

	object->pos = int3(anchorTile.x, anchorTile.y, level);
	initializeObject(object.get());

	map->getEditManager()->insertObject(object);

	if(listener)
	{
		listener->onObjectAdded(object.get());
		listener->onContentChanged();
	}
	notifyModified();
}

void EditorMap::commitObjectChange(int level, CGObjectInstance * object)
{
	if(listener)
	{
		listener->onObjectRemoving(object);
		listener->onObjectAdded(object);
		listener->onContentChanged();
	}
	notifyModified();
}

void EditorMap::commitChangeWithoutRedraw()
{
	notifyModified();
}

int EditorMap::updateAppearance(int level)
{
	int errors = 0;
	auto & state = levelStates.at(level);
	state.terrainSelection.clear();

	const std::set<CGObjectInstance *> selection = state.objectSelection;
	for(auto * obj : selection)
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(obj->ID, obj->subID);
		if(!map->isInTheMap(obj->visitablePos()))
		{
			++errors;
			continue;
		}

		auto * terrain = map->getTile(obj->visitablePos()).getTerrain();

		if(handler->isStaticObject())
		{
			if(obj->appearance->canBePlacedAt(terrain->getId()))
			{
				state.objectSelection.erase(obj);
				continue;
			}
			std::vector<int3> selectedTiles;
			for(auto & offset : obj->appearance->getBlockedOffsets())
				selectedTiles.push_back(obj->pos + offset);
			selectTiles(level, selectedTiles);
		}
		else
		{
			auto app = handler->getOverride(terrain->getId(), obj);
			if(!app)
			{
				if(obj->appearance->canBePlacedAt(terrain->getId()))
					continue;

				auto templates = handler->getTemplates(terrain->getId());
				if(templates.empty())
				{
					++errors;
					continue;
				}
				app = templates.front();
			}

			if(listener)
				listener->onObjectRemoving(obj);
			obj->appearance = app;
			if(listener)
				listener->onObjectAdded(obj);

			state.objectSelection.erase(obj);
		}
	}

	notifySelection();
	notifyModified();
	return errors;
}

void EditorMap::copyToClipboard(int level)
{
	clipboard.clear();
	clipboardShiftIndex = 0;

	for(auto * obj : levelStates.at(level).objectSelection)
	{
		assert(obj->pos.z == level);
		clipboard.push_back(CMemorySerializer::deepCopy(*obj, callback.get()));
	}
}

std::vector<std::string> EditorMap::pasteFromClipboard(int level)
{
	clearObjectSelection(level);

	auto shift = int3::getDirs()[clipboardShiftIndex++];
	if(clipboardShiftIndex == int3::getDirs().size())
		clipboardShiftIndex = 0;

	std::vector<std::string> errors;
	for(auto & clipboardObject : clipboard)
	{
		auto obj = CMemorySerializer::deepCopyShared(*clipboardObject, callback.get());

		const PlacementCheck check = checkPlacement(obj.get());
		if(!check.error.empty())
		{
			errors.push_back(check.error);
			continue;
		}
		registerRequiredMods(check.missingMods);

		auto newPos = clipboardObject->pos + shift;
		if(map->isInTheMap(newPos))
			obj->pos = newPos;
		obj->pos.z = level;

		obj->id = {};
		initializeObject(obj.get());
		map->getEditManager()->insertObject(obj);
		levelStates.at(level).objectSelection.insert(obj.get());

		if(listener)
			listener->onObjectAdded(obj.get());
	}

	notifySelection();
	if(listener)
		listener->onContentChanged();
	notifyModified();
	return errors;
}

EditorMap::PlacementCheck EditorMap::checkPlacement(const CGObjectInstance * object) const
{
	PlacementCheck result;

	if(object->ID == Obj::GRAIL)
	{
		for(const auto & other : map->objects)
		{
			if(other->ID == object->ID && other->subID == object->subID)
			{
				result.error = EditorTexts::get("error.singleGrail");
				return result;
			}
		}
		return result;
	}

	result.needsPlayer = defaultPlayer == PlayerColor::NEUTRAL && (object->ID == Obj::HERO || object->ID == Obj::RANDOM_HERO);

	ModCompatibilityInfo modsInfo;
	modAssessmentObject(object, modsInfo);
	for(const auto & mod : modsInfo)
		if(!map->mods.count(mod.first))
			result.missingMods.push_back(mod.second);

	return result;
}

void EditorMap::registerRequiredMods(const std::vector<ModVerificationInfo> & mods)
{
	for(const auto & mod : mods)
		map->mods[mod.name] = LIBRARY->modh->getModInfo(mod.name).getVerificationInfo();

	if(!mods.empty())
		notifyModified();
}

void EditorMap::dropDeadPointers()
{
	std::set<const CGObjectInstance *> alive;
	for(const auto & object : map->objects)
		alive.insert(object.get());

	for(auto & state : levelStates)
	{
		state.terrainSelection.clear();
		vstd::erase_if(state.objectSelection, [&](const CGObjectInstance * object) { return !alive.count(object); });
		vstd::erase_if(state.lockedObjects, [&](const CGObjectInstance * object) { return !alive.count(object); });
	}
}

void EditorMap::undo()
{
	map->getEditManager()->getUndoManager().undo();
	dropDeadPointers();

	notifyModified();
	if(listener)
		listener->onMapReloaded();
}

void EditorMap::redo()
{
	map->getEditManager()->getUndoManager().redo();
	dropDeadPointers();

	notifyModified();
	if(listener)
		listener->onMapReloaded();
}

ModCompatibilityInfo EditorMap::modAssessmentAll()
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

void EditorMap::modAssessmentObject(const CGObjectInstance * obj, ModCompatibilityInfo & result)
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

ModCompatibilityInfo EditorMap::modAssessmentMap(const CMap & map)
{
	ModCompatibilityInfo result;

	for(auto obj : map.objects)
	{
		if(!obj)
			continue;
		modAssessmentObject(obj.get(), result);
	}
	return result;
}

std::string EditorMap::modMissingMessage(const ModVerificationInfo & info)
{
	if(info.parent.empty())
		return EditorTexts::format("error.modRequired", {info.name});

	return EditorTexts::format("error.submodRequired", {info.name, info.parent});
}

void EditorMap::initializeObject(CGObjectInstance * o)
{
	logGlobal->info("New object instance initialized");
	o->cb = callback.get();

	///IMPORTANT! initialize order should be from base objects to derived objects
	if(auto * artifact = dynamic_cast<CGArtifact *>(o))
	{
		if(artifact->ID == Obj::SPELL_SCROLL)
		{
			std::vector<SpellID> out;
			for(auto const & spell : LIBRARY->spellh->objects) //spellh size appears to be greater (?)
			{
				if(LIBRARY->spellh->getDefaultAllowed().count(spell->id) != 0)
					out.push_back(spell->id);
			}
			artifact->setArtifactInstance(map->createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault())));
		}
		else if(artifact->ID == Obj::ARTIFACT || MapObjectID::isRandomArtifact(artifact->ID))
		{
			artifact->setArtifactInstance(map->createArtifact(artifact->getArtifactType()));
		}
		else
			throw std::runtime_error("Unimplemented initializer for CGArtifact object ID = " + std::to_string(artifact->ID.getNum()));
	}

	if(auto * shipyard = dynamic_cast<CGShipyard *>(o))
		shipyard->tempOwner = defaultPlayer;

	if(auto * garrison = dynamic_cast<CGGarrison *>(o))
	{
		garrison->tempOwner = defaultPlayer;
		garrison->removableUnits = true;
	}

	if(auto * mine = dynamic_cast<CGMine *>(o))
	{
		mine->tempOwner = defaultPlayer;
		if(mine->isAbandoned())
		{
			for(auto r = GameResID(0); r < GameResID::COUNT; ++r)
				mine->abandonedMineResources.insert(r);
		}
		else
		{
			if(mine->getResourceHandler()->getResourceType() == GameResID::NONE) // fallback
				mine->producedResource = GameResID(mine->subID);
			else
				mine->producedResource = mine->getResourceHandler()->getResourceType();
			mine->producedQuantity = mine->defaultResProduction();
		}
	}

	if(auto * dwelling = dynamic_cast<CGDwelling *>(o))
	{
		dwelling->tempOwner = defaultPlayer;

		if(dwelling->ID == Obj::RANDOM_DWELLING || dwelling->ID == Obj::RANDOM_DWELLING_LVL || dwelling->ID == Obj::RANDOM_DWELLING_FACTION)
		{
			dwelling->randomizationInfo = CGDwellingRandomizationInfo();
			if(dwelling->ID == Obj::RANDOM_DWELLING_LVL)
			{
				dwelling->randomizationInfo->minLevel = dwelling->subID;
				dwelling->randomizationInfo->maxLevel = dwelling->subID;
			}
			if(dwelling->ID == Obj::RANDOM_DWELLING_FACTION)
			{
				dwelling->randomizationInfo->allowedFactions.insert(FactionID(dwelling->subID));
			}
		}
	}

	if(auto * town = dynamic_cast<CGTownInstance *>(o))
	{
		const std::vector<std::string> castleLevels{"village", "fort", "citadel", "castle", "capitol"};
		int lvl = vstd::find_pos(castleLevels, town->appearance->stringID);
		town->addBuilding(BuildingID::DEFAULT);
		if(lvl > -1) town->addBuilding(BuildingID::TAVERN);
		if(lvl > 0) town->addBuilding(BuildingID::FORT);
		if(lvl > 1) town->addBuilding(BuildingID::CITADEL);
		if(lvl > 2) town->addBuilding(BuildingID::CASTLE);
		if(lvl > 3) town->addBuilding(BuildingID::CAPITOL);

		if(town->possibleSpells.empty())
		{
			for(auto const & spellId : LIBRARY->spellh->getDefaultAllowed()) //add all regular spells to town
				town->possibleSpells.push_back(spellId);
		}
	}

	if(auto * creature = dynamic_cast<CGCreature *>(o))
	{
		creature->initialCharacter = CGCreature::Character::HOSTILE;
		if(!creature->hasStackAtSlot(SlotID(0)))
			creature->putStack(SlotID(0), std::make_unique<CStackInstance>(creature->cb, CreatureID(creature->subID), 1, false));
	}

	if(auto * placeholder = dynamic_cast<CGHeroPlaceholder *>(o))
	{
		placeholder->tempOwner = defaultPlayer;

		if(!placeholder->powerRank.has_value() && !placeholder->heroType.has_value())
			placeholder->powerRank = 0;

		if(placeholder->powerRank.has_value() && placeholder->heroType.has_value())
			placeholder->powerRank.reset();
	}

	if(auto * hero = dynamic_cast<CGHeroInstance *>(o))
	{
		hero->tempOwner = defaultPlayer;
		if(hero->ID == Obj::PRISON)
		{
			hero->subID = 0;
			hero->tempOwner = PlayerColor::NEUTRAL;
		}

		if(hero->ID == Obj::HERO)
		{
			for(auto const & t : LIBRARY->heroh->objects)
			{
				if(t->heroClass->getId() == HeroClassID(hero->subID))
				{
					hero->subID = t->getId();
					break;
				}
			}
		}

		if(hero->getHeroTypeID().hasValue())
		{
			hero->gender = hero->getHeroType()->gender;
			hero->randomizeArmy(hero->getFactionID());
		}
	}

	if(auto * flaggable = dynamic_cast<FlaggableMapObject *>(o))
		flaggable->tempOwner = defaultPlayer;
}
