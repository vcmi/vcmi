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

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"//todo: remove
#include "CCreatureHandler.h"//todo: remove
#include "spells/CSpellHandler.h" //todo: remove
#include "CSkillHandler.h"//todo: remove
#include "constants/StringConstants.h"
#include "CGeneralTextHandler.h"
#include "TerrainHandler.h" //TODO: remove
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

const QueryID QueryID::NONE = QueryID(-1);
const HeroTypeID HeroTypeID::NONE = HeroTypeID(-1);
const ObjectInstanceID ObjectInstanceID::NONE = ObjectInstanceID(-1);

const SlotID SlotID::COMMANDER_SLOT_PLACEHOLDER = SlotID(-2);
const SlotID SlotID::SUMMONED_SLOT_PLACEHOLDER = SlotID(-3);
const SlotID SlotID::WAR_MACHINES_SLOT = SlotID(-4);
const SlotID SlotID::ARROW_TOWERS_SLOT = SlotID(-5);

const PlayerColor PlayerColor::SPECTATOR = PlayerColor(-4);
const PlayerColor PlayerColor::CANNOT_DETERMINE = PlayerColor(-3);
const PlayerColor PlayerColor::UNFLAGGABLE = PlayerColor(-2);
const PlayerColor PlayerColor::NEUTRAL = PlayerColor(-1);
const PlayerColor PlayerColor::PLAYER_LIMIT = PlayerColor(PLAYER_LIMIT_I);
const TeamID TeamID::NO_TEAM = TeamID(-1);

const SpellSchool SpellSchool::ANY = -1;
const SpellSchool SpellSchool::AIR = 0;
const SpellSchool SpellSchool::FIRE = 1;
const SpellSchool SpellSchool::WATER = 2;
const SpellSchool SpellSchool::EARTH = 3;

const FactionID FactionID::NONE = -2;
const FactionID FactionID::DEFAULT = -1;
const FactionID FactionID::RANDOM = -1;
const FactionID FactionID::ANY = -1;
const FactionID FactionID::CASTLE = 0;
const FactionID FactionID::RAMPART = 1;
const FactionID FactionID::TOWER = 2;
const FactionID FactionID::INFERNO = 3;
const FactionID FactionID::NECROPOLIS = 4;
const FactionID FactionID::DUNGEON = 5;
const FactionID FactionID::STRONGHOLD = 6;
const FactionID FactionID::FORTRESS = 7;
const FactionID FactionID::CONFLUX = 8;
const FactionID FactionID::NEUTRAL = 9;

const BoatId BoatId::NONE = -1;
const BoatId BoatId::NECROPOLIS = 0;
const BoatId BoatId::CASTLE = 1;
const BoatId BoatId::FORTRESS = 2;

const RiverId RiverId::NO_RIVER = 0;
const RiverId RiverId::WATER_RIVER = 1;
const RiverId RiverId::ICY_RIVER = 2;
const RiverId RiverId::MUD_RIVER = 3;
const RiverId RiverId::LAVA_RIVER = 4;

const RoadId RoadId::NO_ROAD = 0;
const RoadId RoadId::DIRT_ROAD = 1;
const RoadId RoadId::GRAVEL_ROAD = 2;
const RoadId RoadId::COBBLESTONE_ROAD = 3;


namespace GameConstants
{
#ifdef VCMI_NO_EXTRA_VERSION
	const std::string VCMI_VERSION = "VCMI " VCMI_VERSION_STRING;
#else
	const std::string VCMI_VERSION = "VCMI " VCMI_VERSION_STRING "." + std::string{GIT_SHA1};
#endif
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
	return dynamic_cast<const CArtifact*>(toArtifact(VLC->artifacts()));
}

const Artifact * ArtifactIDBase::toArtifact(const ArtifactService * service) const
{
	return service->getByIndex(num);
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

const CCreature * CreatureIDBase::toCreature() const
{
	return VLC->creh->objects.at(num);
}

const Creature * CreatureIDBase::toCreature(const CreatureService * creatures) const
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

const spells::Spell * SpellIDBase::toSpell(const spells::Service * service) const
{
	return service->getByIndex(num);
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
	return GameConstants::PLAYER_COLOR_NAMES[index];
}

std::string PlayerColor::entityType()
{
	return "playerColor";
}

si32 FactionID::decode(const std::string & identifier)
{
	auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), entityType(), identifier);
	if(rawId)
		return rawId.value();
	else
		return FactionID::DEFAULT;
}

std::string FactionID::encode(const si32 index)
{
	return VLC->factions()->getByIndex(index)->getJsonKey();
}

std::string FactionID::entityType()
{
	return "faction";
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

const BattleField BattleField::NONE;

const BattleFieldInfo * BattleField::getInfo() const
{
	return VLC->battlefields()->getById(*this);
}

const ObstacleInfo * Obstacle::getInfo() const
{
	return VLC->obstacles()->getById(*this);
}

VCMI_LIB_NAMESPACE_END
