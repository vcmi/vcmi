/*
 * BattleOnlyMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleOnlyMap.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/CSkillHandler.h"

const std::string BattleOnlyMap::fileURI = "Maps/BattleOnlyMode";

void BattleOnlyMap::write(const BattleOnlyModeStartInfo & startInfo)
{
	auto map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = 10;
	map->height = 10;
	map->mapLayers = {MapLayerId::SURFACE};
	map->battleOnly = true;
	map->name = MetaString::createFromTextID("vcmi.lobby.battleOnlyMode");

	EditorCallback cb(map.get());
	map->cb = &cb;

	auto rng = &CRandomGenerator::getDefault();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(rng);

	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(startInfo.selectedTerrain == TerrainId::NONE ? TerrainId::DIRT : startInfo.selectedTerrain, 0, rng);

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	auto addHero = [&](int sel, PlayerColor color, const int3 & position)
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, startInfo.selectedHero[sel].toHeroType()->getHeroClass()->getId());
		auto templates = factory->getTemplates();
		auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(&cb, templates.front()));
		obj->setHeroType(startInfo.selectedHero[sel]);

		obj->setOwner(color);
		obj->pos = position;
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			obj->pushPrimSkill(PrimarySkill(i), startInfo.primSkillLevel[sel][i]);
		obj->clearSlots();
		for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
			if(startInfo.selectedArmy[sel][slot].getId() != CreatureID::NONE && startInfo.selectedArmy[sel][slot].getCount() > 0)
				obj->setCreature(SlotID(slot), startInfo.selectedArmy[sel][slot].getId(), startInfo.selectedArmy[sel][slot].getCount());

		// give spellbook
		if(!obj->getArt(ArtifactPosition::SPELLBOOK) && startInfo.spellBook[sel])
			obj->putArtifact(ArtifactPosition::SPELLBOOK, map->createArtifact(ArtifactID::SPELLBOOK));
		else if(obj->getArt(ArtifactPosition::SPELLBOOK) && !startInfo.spellBook[sel])
			obj->removeArtifact(ArtifactPosition::SPELLBOOK);
		
		if(startInfo.warMachines[sel])
		{
			obj->putArtifact(ArtifactPosition::MACH1, map->createArtifact(ArtifactID::BALLISTA));
			obj->putArtifact(ArtifactPosition::MACH2, map->createArtifact(ArtifactID::AMMO_CART));
			obj->putArtifact(ArtifactPosition::MACH3, map->createArtifact(ArtifactID::FIRST_AID_TENT));
		}
		
		for(const auto & spell : startInfo.spells[sel])
			obj->addSpellToSpellbook(spell);

		for(auto & artifact : startInfo.artifacts[sel])
			if(artifact.second != ArtifactID::NONE)
				obj->putArtifact(artifact.first, map->createArtifact(artifact.second));

		for(const auto & skill : LIBRARY->skillh->getObjects()) // reset all standard skills
			obj->setSecSkillLevel(SecondarySkill(skill->getId()), MasteryLevel::NONE, ChangeValueMode::ABSOLUTE);
		for(int skillSlot = 0; skillSlot < 8; skillSlot++)
			obj->setSecSkillLevel(startInfo.secSkillLevel[sel][skillSlot].first, startInfo.secSkillLevel[sel][skillSlot].second, ChangeValueMode::ABSOLUTE);

		map->getEditManager()->insertObject(obj);
	};

	addHero(0, PlayerColor(0), int3(5, 6, 0));
	if(startInfo.selectedTown == FactionID::ANY)
		addHero(1, PlayerColor(1), int3(5, 5, 0));
	else
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, startInfo.selectedTown);
		auto templates = factory->getTemplates();
		auto obj = factory->create(&cb, templates.front());
		auto townObj = std::dynamic_pointer_cast<CGTownInstance>(obj);
		obj->setOwner(PlayerColor(1));
		obj->pos = int3(5, 5, 0);
		for (const auto & building : townObj->getTown()->getAllBuildings())
			townObj->addBuilding(building);
		if(startInfo.selectedHero[1] == HeroTypeID::NONE)
		{
			for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
				if(startInfo.selectedArmy[1][slot].getId() != CreatureID::NONE && startInfo.selectedArmy[1][slot].getCount() > 0)
					townObj->getArmy()->setCreature(SlotID(slot), startInfo.selectedArmy[1][slot].getId(), startInfo.selectedArmy[1][slot].getCount());
		}
		else
			addHero(1, PlayerColor(1), int3(5, 5, 0));

		map->getEditManager()->insertObject(townObj);
	}

	auto path = VCMIDirs::get().userDataPath() / "Maps";
	boost::filesystem::create_directories(path);
	CMapService mapService;
	mapService.saveMap(map, path / "BattleOnlyMode.vmap");
	CResourceHandler::get()->updateFilteredFiles([](const std::string & mount) { return boost::algorithm::starts_with(mount, "MAPS/"); });
}
