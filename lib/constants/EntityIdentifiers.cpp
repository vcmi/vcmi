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
#include "VCMI_Lib.h"
#include "CHeroHandler.h"
#include "CArtHandler.h"//todo: remove
#include "CCreatureHandler.h"//todo: remove
#include "spells/CSpellHandler.h" //todo: remove
#include "CSkillHandler.h"//todo: remove
#include "constants/StringConstants.h"
#include "CGeneralTextHandler.h"
#include "TerrainHandler.h" //TODO: remove
#include "RiverHandler.h"
#include "RoadHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "CTownHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

const CampaignScenarioID CampaignScenarioID::NONE(-1);
const BattleID BattleID::NONE(-1);
const QueryID QueryID::NONE(-1);
const QueryID QueryID::CLIENT(-2);
const HeroTypeID HeroTypeID::NONE(-1);
const HeroTypeID HeroTypeID::RANDOM(-2);
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
const PrimarySkill PrimarySkill::BEGIN(0);
const PrimarySkill PrimarySkill::END(4);
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

si32 HeroClassID::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "heroClass", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string HeroClassID::encode(const si32 index)
{
	return VLC->heroClasses()->getByIndex(index)->getJsonKey();
}

std::string HeroClassID::entityType()
{
	return "heroClass";
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

std::string Obj::encode(int32_t index)
{
	return VLC->objtypeh->getObjectHandlerName(index);
}

si32 Obj::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "objects", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

si32 HeroTypeID::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string HeroTypeID::encode(const si32 index)
{
	return VLC->heroTypes()->getByIndex(index)->getJsonKey();
}

std::string HeroTypeID::entityType()
{
	return "hero";
}

const CArtifact * ArtifactIDBase::toArtifact() const
{
	return dynamic_cast<const CArtifact*>(toEntity(VLC));
}

const Artifact * ArtifactIDBase::toEntity(const Services * services) const
{
	return services->artifacts()->getByIndex(num);
}

si32 ArtifactID::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "artifact", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string ArtifactID::encode(const si32 index)
{
	return VLC->artifacts()->getByIndex(index)->getJsonKey();
}

std::string ArtifactID::entityType()
{
	return "artifact";
}

si32 SecondarySkill::decode(const std::string& identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "secondarySkill", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string SecondarySkill::encode(const si32 index)
{
	return VLC->skills()->getById(SecondarySkill(index))->getJsonKey();
}

const CCreature * CreatureIDBase::toCreature() const
{
	return VLC->creh->objects.at(num);
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
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "creature", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string CreatureID::encode(const si32 index)
{
	return VLC->creatures()->getById(CreatureID(index))->getJsonKey();
}

std::string CreatureID::entityType()
{
	return "creature";
}

const CSpell * SpellIDBase::toSpell() const
{
	if(num < 0 || num >= VLC->spellh->objects.size())
	{
		logGlobal->error("Unable to get spell of invalid ID %d", static_cast<int>(num));
		return nullptr;
	}
	return VLC->spellh->objects[num];
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
	return dynamic_cast<const CHero*>(toEntity(VLC));
}

const HeroType * HeroTypeID::toEntity(const Services * services) const
{
	return services->heroTypes()->getByIndex(num);
}

si32 SpellID::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "spell", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string SpellID::encode(const si32 index)
{
	return VLC->spells()->getByIndex(index)->getJsonKey();
}

si32 BattleField::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "spell", identifier);
	if(rawId)
		return rawId.value();
	else
		return -1;
}

std::string BattleField::encode(const si32 index)
{
	return VLC->spells()->getByIndex(index)->getJsonKey();
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
	return *VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
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
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
	if(rawId)
		return rawId.value();
	else
		return FactionID::DEFAULT.getNum();
}

std::string FactionID::encode(const si32 index)
{
	return VLC->factions()->getByIndex(index)->getJsonKey();
}

std::string FactionID::entityType()
{
	return "faction";
}

const Faction * FactionID::toEntity(const Services * service) const
{
	return service->factions()->getByIndex(num);
}

si32 TerrainId::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
	if(rawId)
		return rawId.value();
	else
		return static_cast<si32>(TerrainId::NONE);
}

std::string TerrainId::encode(const si32 index)
{
	return VLC->terrainTypeHandler->getByIndex(index)->getJsonKey();
}

std::string TerrainId::entityType()
{
	return "terrain";
}

const TerrainType * TerrainId::toEntity(const Services * service) const
{
	return VLC->terrainTypeHandler->getByIndex(num);
}

const RoadType * RoadId::toEntity(const Services * service) const
{
	return VLC->roadTypeHandler->getByIndex(num);
}

const RiverType * RiverId::toEntity(const Services * service) const
{
	return VLC->riverTypeHandler->getByIndex(num);
}

const BattleField BattleField::NONE;

const BattleFieldInfo * BattleField::getInfo() const
{
	return VLC->battlefields()->getById(*this);
}

const ObstacleInfo * Obstacle::getInfo() const
{
	return VLC->obstacles()->getById(*this);
}

si32 SpellSchool::decode(const std::string & identifier)
{
	return *VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
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
	return *VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
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

VCMI_LIB_NAMESPACE_END
