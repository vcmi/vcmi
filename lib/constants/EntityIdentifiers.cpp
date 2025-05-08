/*
 * EntityIdentifiers.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#ifndef VCMI_NO_EXTRA_VERSION
#include "../Version.h"
#endif
#include <vcmi/Artifact.h>
#include <vcmi/ArtifactService.h>
#include <vcmi/Faction.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroClass.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/Services.h>

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "GameLibrary.h"
#include "CCreatureHandler.h"//todo: remove
#include "spells/CSpellHandler.h" //todo: remove
#include "CSkillHandler.h"//todo: remove
#include "entities/artifact/CArtifact.h"
#include "entities/faction/CFaction.h"
#include "entities/hero/CHero.h"
#include "entities/hero/CHeroClass.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "constants/StringConstants.h"
#include "texts/CGeneralTextHandler.h"
#include "TerrainHandler.h" //TODO: remove
#include "RiverHandler.h"
#include "RoadHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

const CampaignScenarioID CampaignScenarioID::NONE(-1);
const BattleID BattleID::NONE(-1);
const QueryID QueryID::NONE(-1);
const QueryID QueryID::CLIENT(-2);
const HeroTypeID HeroTypeID::NONE(-1);
const HeroTypeID HeroTypeID::RANDOM(-2);
const HeroTypeID HeroTypeID::GEM(27);
const HeroTypeID HeroTypeID::SOLMYR(45);
const HeroTypeID HeroTypeID::CAMP_STRONGEST(0xFFFD);
const HeroTypeID HeroTypeID::CAMP_GENERATED(0xFFFE);
const HeroTypeID HeroTypeID::CAMP_RANDOM(0xFFFF);

const ObjectInstanceID ObjectInstanceID::NONE(-1);

const SlotID SlotID::COMMANDER_SLOT_PLACEHOLDER(-2);
const SlotID SlotID::SUMMONED_SLOT_PLACEHOLDER(-3);
const SlotID SlotID::WAR_MACHINES_SLOT(-4);
const SlotID SlotID::ARROW_TOWERS_SLOT(-5);

const PlayerColor PlayerColor::SPECTATOR(-4);
const PlayerColor PlayerColor::CANNOT_DETERMINE(-3);
const PlayerColor PlayerColor::UNFLAGGABLE(-2);
const PlayerColor PlayerColor::NEUTRAL(-1);
const PlayerColor PlayerColor::PLAYER_LIMIT(PLAYER_LIMIT_I);
const TeamID TeamID::NO_TEAM(-1);

const SpellSchool SpellSchool::ANY(-1);
const SpellSchool SpellSchool::AIR(0);
const SpellSchool SpellSchool::FIRE(1);
const SpellSchool SpellSchool::WATER(2);
const SpellSchool SpellSchool::EARTH(3);

const FactionID FactionID::NONE(-2);
const FactionID FactionID::DEFAULT(-1);
const FactionID FactionID::RANDOM(-1);
const FactionID FactionID::ANY(-1);
const FactionID FactionID::CASTLE(0);
const FactionID FactionID::RAMPART(1);
const FactionID FactionID::TOWER(2);
const FactionID FactionID::INFERNO(3);
const FactionID FactionID::NECROPOLIS(4);
const FactionID FactionID::DUNGEON(5);
const FactionID FactionID::STRONGHOLD(6);
const FactionID FactionID::FORTRESS(7);
const FactionID FactionID::CONFLUX(8);
const FactionID FactionID::NEUTRAL(9);

const PrimarySkill PrimarySkill::NONE(-1);
const PrimarySkill PrimarySkill::ATTACK(0);
const PrimarySkill PrimarySkill::DEFENSE(1);
const PrimarySkill PrimarySkill::SPELL_POWER(2);
const PrimarySkill PrimarySkill::KNOWLEDGE(3);
const PrimarySkill PrimarySkill::EXPERIENCE(4);

const BoatId BoatId::NONE(-1);
const BoatId BoatId::NECROPOLIS(0);
const BoatId BoatId::CASTLE(1);
const BoatId BoatId::FORTRESS(2);

const RiverId RiverId::NO_RIVER(0);
const RiverId RiverId::WATER_RIVER(1);
const RiverId RiverId::ICY_RIVER(2);
const RiverId RiverId::MUD_RIVER(3);
const RiverId RiverId::LAVA_RIVER(4);

const RoadId RoadId::NO_ROAD(0);
const RoadId RoadId::DIRT_ROAD(1);
const RoadId RoadId::GRAVEL_ROAD(2);
const RoadId RoadId::COBBLESTONE_ROAD(3);

namespace GameConstants
{
#ifdef VCMI_NO_EXTRA_VERSION
	const std::string VCMI_VERSION = "VCMI " VCMI_VERSION_STRING;
#else
	const std::string VCMI_VERSION = "VCMI " VCMI_VERSION_STRING "." + std::string{GIT_SHA1};
#endif
}

BuildingTypeUniqueID::BuildingTypeUniqueID(FactionID factionID, BuildingID buildingID ):
	BuildingTypeUniqueID(factionID.getNum() * 0x10000 + buildingID.getNum())
{
	assert(factionID.getNum() >= 0);
	assert(factionID.getNum() < 0x10000);
	assert(buildingID.getNum() >= 0);
	assert(buildingID.getNum() < 0x10000);
}

BuildingID BuildingTypeUniqueID::getBuilding() const
{
	return BuildingID(getNum() % 0x10000);
}

FactionID BuildingTypeUniqueID::getFaction() const
{
	return FactionID(getNum() / 0x10000);
}

int32_t IdentifierBase::resolveIdentifier(const std::string & entityType, const std::string identifier)
{
	if (identifier.empty())
		return -1;

	auto rawId = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), entityType, identifier);

	if (rawId)
		return rawId.value();
	throw IdentifierResolutionException(identifier);
}

si32 HeroClassID::decode(const std::string & identifier)
{
	return resolveIdentifier("heroClass", identifier);
}

std::string HeroClassID::encode(const si32 index)
{
	if (index == -1)
		return "";
	return LIBRARY->heroClasses()->getByIndex(index)->getJsonKey();
}

std::string HeroClassID::entityType()
{
	return "heroClass";
}

const CHeroClass * HeroClassID::toHeroClass() const
{
	return dynamic_cast<const CHeroClass*>(toEntity(LIBRARY));
}

const HeroClass * HeroClassID::toEntity(const Services * services) const
{
	return services->heroClasses()->getByIndex(num);
}

si32 ObjectInstanceID::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string ObjectInstanceID::encode(const si32 index)
{
	return std::to_string(index);
}

si32 CampaignScenarioID::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string CampaignScenarioID::encode(const si32 index)
{
	return std::to_string(index);
}

std::string MapObjectID::encode(int32_t index)
{
	if (index == -1)
		return "";
	return LIBRARY->objtypeh->getJsonKey(MapObjectID(index));
}

si32 MapObjectID::decode(const std::string & identifier)
{
	return resolveIdentifier("object", identifier);
}

std::string MapObjectSubID::encode(MapObjectID primaryID, int32_t index)
{
	if (index == -1)
		return "";

	if(primaryID == Obj::PRISON || primaryID == Obj::HERO)
		return HeroTypeID::encode(index);

	if (primaryID == Obj::SPELL_SCROLL)
		return SpellID::encode(index);

	return LIBRARY->objtypeh->getHandlerFor(primaryID, index)->getJsonKey();
}

si32 MapObjectSubID::decode(MapObjectID primaryID, const std::string & identifier)
{
	if(primaryID == Obj::PRISON || primaryID == Obj::HERO)
		return HeroTypeID::decode(identifier);

	if (primaryID == Obj::SPELL_SCROLL)
		return SpellID::decode(identifier);

	return resolveIdentifier(LIBRARY->objtypeh->getJsonKey(primaryID), identifier);
}

std::string BoatId::encode(int32_t index)
{
	if (index == -1)
		return "";
	return LIBRARY->objtypeh->getHandlerFor(MapObjectID::BOAT, index)->getJsonKey();
}

si32 BoatId::decode(const std::string & identifier)
{
	return resolveIdentifier("core:boat", identifier);
}

si32 HeroTypeID::decode(const std::string & identifier)
{
	if (identifier == "random")
		return -2;
	return resolveIdentifier("hero", identifier);
}

std::string HeroTypeID::encode(const si32 index)
{
	if (index == -1)
		return "";
	if (index == -2)
		return "random";
	return LIBRARY->heroTypes()->getByIndex(index)->getJsonKey();
}

std::string HeroTypeID::entityType()
{
	return "hero";
}

const CArtifact * ArtifactIDBase::toArtifact() const
{
	return dynamic_cast<const CArtifact*>(toEntity(LIBRARY));
}

const Artifact * ArtifactIDBase::toEntity(const Services * services) const
{
	return services->artifacts()->getByIndex(num);
}

si32 ArtifactID::decode(const std::string & identifier)
{
	return resolveIdentifier("artifact", identifier);
}

std::string ArtifactID::encode(const si32 index)
{
	if (index == -1)
		return "";
	return LIBRARY->artifacts()->getByIndex(index)->getJsonKey();
}

std::string ArtifactID::entityType()
{
	return "artifact";
}

si32 SecondarySkill::decode(const std::string& identifier)
{
	return resolveIdentifier("secondarySkill", identifier);
}

std::string SecondarySkill::encode(const si32 index)
{
	if (index == -1)
		return "";
	return LIBRARY->skills()->getById(SecondarySkill(index))->getJsonKey();
}

const CSkill * SecondarySkill::toSkill() const
{
	return dynamic_cast<const CSkill *>(toEntity(LIBRARY));
}

const Skill * SecondarySkill::toEntity(const Services * services) const
{
	return services->skills()->getByIndex(num);
}

const CCreature * CreatureIDBase::toCreature() const
{
	return (*LIBRARY->creh)[num];
}

const Creature * CreatureIDBase::toEntity(const Services * services) const
{
	return toEntity(services->creatures());
}

const Creature * CreatureIDBase::toEntity(const CreatureService * creatures) const
{
	return creatures->getByIndex(num);
}

si32 CreatureID::decode(const std::string & identifier)
{
	return resolveIdentifier("creature", identifier);
}

std::string CreatureID::encode(const si32 index)
{
	if (index == -1)
		return "";
	return LIBRARY->creatures()->getById(CreatureID(index))->getJsonKey();
}

std::string CreatureID::entityType()
{
	return "creature";
}

const CSpell * SpellIDBase::toSpell() const
{
	return dynamic_cast<const CSpell*>(toEntity(LIBRARY));
}

const spells::Spell * SpellIDBase::toEntity(const Services * services) const
{
	return toEntity(services->spells());
}

const spells::Spell * SpellIDBase::toEntity(const spells::Service * service) const
{
	return service->getByIndex(num);
}

const CHero * HeroTypeID::toHeroType() const
{
	return dynamic_cast<const CHero*>(toEntity(LIBRARY));
}

const HeroType * HeroTypeID::toEntity(const Services * services) const
{
	return services->heroTypes()->getByIndex(num);
}

si32 SpellID::decode(const std::string & identifier)
{
	if (identifier == "preset")
		return SpellID::PRESET;
	if (identifier == "spellbook_preset")
		return SpellID::SPELLBOOK_PRESET;
	return resolveIdentifier("spell", identifier);
}

std::string SpellID::encode(const si32 index)
{
	if (index == -1)
		return "";
	if (index == SpellID::PRESET)
		return "preset";
	if (index == SpellID::SPELLBOOK_PRESET)
		return "spellbook_preset";
	return LIBRARY->spells()->getByIndex(index)->getJsonKey();
}

si32 BattleField::decode(const std::string & identifier)
{
	return resolveIdentifier("battlefield", identifier);
}

std::string BattleField::encode(const si32 index)
{
	return LIBRARY->battlefields()->getByIndex(index)->getJsonKey();
}

std::string SpellID::entityType()
{
	return "spell";
}

bool PlayerColor::isValidPlayer() const
{
	return num >= 0 && num < PLAYER_LIMIT_I;
}

bool PlayerColor::isSpectator() const
{
	return num == SPECTATOR.num;
}

std::string PlayerColor::toString() const
{
	return encode(num);
}

si32 PlayerColor::decode(const std::string & identifier)
{
	return vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, identifier);
}

std::string PlayerColor::encode(const si32 index)
{
	if (index == -1)
		return "neutral";

	if (index < 0 || index >= std::size(GameConstants::PLAYER_COLOR_NAMES))
	{
		assert(0);
		return "invalid";
	}

	return GameConstants::PLAYER_COLOR_NAMES[index];
}

std::string PlayerColor::entityType()
{
	return "playerColor";
}

si32 PrimarySkill::decode(const std::string& identifier)
{
	return resolveIdentifier(entityType(), identifier);
}

std::string PrimarySkill::encode(const si32 index)
{
	return NPrimarySkill::names[index];
}

std::string PrimarySkill::entityType()
{
	return "primarySkill";
}

si32 FactionID::decode(const std::string & identifier)
{
	return resolveIdentifier(entityType(), identifier);
}

std::string FactionID::encode(const si32 index)
{
	if (index == -1)
		return "";
	return LIBRARY->factions()->getByIndex(index)->getJsonKey();
}

std::string FactionID::entityType()
{
	return "faction";
}

const CFaction * FactionID::toFaction() const
{
	return dynamic_cast<const CFaction*>(toEntity(LIBRARY));
}

const Faction * FactionID::toEntity(const Services * service) const
{
	return service->factions()->getByIndex(num);
}

si32 TerrainId::decode(const std::string & identifier)
{
	if (identifier == "native")
		return TerrainId::NATIVE_TERRAIN;

	return resolveIdentifier(entityType(), identifier);
}

std::string TerrainId::encode(const si32 index)
{
	if (index == TerrainId::NONE)
		return "";
	if (index == TerrainId::NATIVE_TERRAIN)
		return "native";
	return LIBRARY->terrainTypeHandler->getByIndex(index)->getJsonKey();
}

std::string TerrainId::entityType()
{
	return "terrain";
}

si32 RoadId::decode(const std::string & identifier)
{
	if (identifier.empty())
		return RoadId::NO_ROAD.getNum();

	return resolveIdentifier(entityType(), identifier);
}

std::string RoadId::encode(const si32 index)
{
	if (index == RoadId::NO_ROAD.getNum())
		return "";
	return LIBRARY->roadTypeHandler->getByIndex(index)->getJsonKey();
}

std::string RoadId::entityType()
{
	return "road";
}

si32 RiverId::decode(const std::string & identifier)
{
	if (identifier.empty())
		return RiverId::NO_RIVER.getNum();

	return resolveIdentifier(entityType(), identifier);
}

std::string RiverId::encode(const si32 index)
{
	if (index == RiverId::NO_RIVER.getNum())
		return "";
	return LIBRARY->riverTypeHandler->getByIndex(index)->getJsonKey();
}

std::string RiverId::entityType()
{
	return "river";
}

const TerrainType * TerrainId::toEntity(const Services * service) const
{
	return LIBRARY->terrainTypeHandler->getByIndex(num);
}

const RoadType * RoadId::toEntity(const Services * service) const
{
	return LIBRARY->roadTypeHandler->getByIndex(num);
}

const RiverType * RiverId::toEntity(const Services * service) const
{
	return LIBRARY->riverTypeHandler->getByIndex(num);
}

const BattleField BattleField::NONE;

const BattleFieldInfo * BattleField::getInfo() const
{
	return LIBRARY->battlefields()->getById(*this);
}

const ObstacleInfo * Obstacle::getInfo() const
{
	return LIBRARY->obstacles()->getById(*this);
}

si32 SpellSchool::decode(const std::string & identifier)
{
	return resolveIdentifier(entityType(), identifier);
}

std::string SpellSchool::encode(const si32 index)
{
	if (index == ANY.getNum())
		return "any";

	return SpellConfig::SCHOOL[index].jsonName;
}

std::string SpellSchool::entityType()
{
	return "spellSchool";
}

si32 GameResID::decode(const std::string & identifier)
{
	return resolveIdentifier(entityType(), identifier);
}

std::string GameResID::encode(const si32 index)
{
	return GameConstants::RESOURCE_NAMES[index];
}

si32 BuildingTypeUniqueID::decode(const std::string & identifier)
{
	assert(0); //TODO
	return -1;
}

std::string BuildingTypeUniqueID::encode(const si32 index)
{
	assert(0); // TODO
	return "";
}

std::string GameResID::entityType()
{
	return "resource";
}

const std::array<PlayerColor, PlayerColor::PLAYER_LIMIT_I> & PlayerColor::ALL_PLAYERS()
{
	static const std::array allPlayers = {
		PlayerColor(0),
		PlayerColor(1),
		PlayerColor(2),
		PlayerColor(3),
		PlayerColor(4),
		PlayerColor(5),
		PlayerColor(6),
		PlayerColor(7)
	};

	return allPlayers;
}

const std::array<PrimarySkill, 4> & PrimarySkill::ALL_SKILLS()
{
	static const std::array allSkills = {
		PrimarySkill(ATTACK),
		PrimarySkill(DEFENSE),
		PrimarySkill(SPELL_POWER),
		PrimarySkill(KNOWLEDGE)
	};

	return allSkills;
}

const std::array<GameResID, 7> & GameResID::ALL_RESOURCES()
{
	static const std::array allResources = {
		GameResID(WOOD),
		GameResID(MERCURY),
		GameResID(ORE),
		GameResID(SULFUR),
		GameResID(CRYSTAL),
		GameResID(GEMS),
		GameResID(GOLD)
	};

	return allResources;
}

std::string SecondarySkill::entityType()
{
	return "secondarySkill";
}

std::string BuildingID::encode(int32_t index)
{
	return std::to_string(index);
}

si32 BuildingID::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

VCMI_LIB_NAMESPACE_END
