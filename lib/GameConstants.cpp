/*
 * GameConstants.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#define INSTANTIATE_BASE_FOR_ID_HERE

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

#include "VCMI_Lib.h"
#include "mapObjects/CObjectClassesHandler.h"//todo: remove
#include "CArtHandler.h"//todo: remove
#include "CCreatureHandler.h"//todo: remove
#include "spells/CSpellHandler.h" //todo: remove
#include "CSkillHandler.h"//todo: remove
#include "StringConstants.h"
#include "CGeneralTextHandler.h"
#include "CModHandler.h"//todo: remove

const SlotID SlotID::COMMANDER_SLOT_PLACEHOLDER = SlotID(-2);
const SlotID SlotID::SUMMONED_SLOT_PLACEHOLDER = SlotID(-3);
const SlotID SlotID::WAR_MACHINES_SLOT = SlotID(-4);
const SlotID SlotID::ARROW_TOWERS_SLOT = SlotID(-5);

const PlayerColor PlayerColor::SPECTATOR = PlayerColor(252);
const PlayerColor PlayerColor::CANNOT_DETERMINE = PlayerColor(253);
const PlayerColor PlayerColor::UNFLAGGABLE = PlayerColor(254);
const PlayerColor PlayerColor::NEUTRAL = PlayerColor(255);
const PlayerColor PlayerColor::PLAYER_LIMIT = PlayerColor(PLAYER_LIMIT_I);
const TeamID TeamID::NO_TEAM = TeamID(255);

namespace GameConstants
{
#ifdef VCMI_NO_EXTRA_VERSION
	const std::string VCMI_VERSION = std::string("VCMI 0.99");
#else
	const std::string VCMI_VERSION = std::string("VCMI 0.99 ") + GIT_SHA1;
#endif
}

si32 HeroTypeID::decode(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier("core", "hero", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string HeroTypeID::encode(const si32 index)
{
	return VLC->heroTypes()->getByIndex(index)->getJsonKey();
}

const CArtifact * ArtifactID::toArtifact() const
{
	return VLC->arth->objects.at(*this);
}

const Artifact * ArtifactID::toArtifact(const ArtifactService * service) const
{
	return service->getById(*this);
}

si32 ArtifactID::decode(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier("core", "artifact", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string ArtifactID::encode(const si32 index)
{
	return VLC->artifacts()->getByIndex(index)->getJsonKey();
}

const CCreature * CreatureID::toCreature() const
{
	return VLC->creh->objects.at(*this);
}

const Creature * CreatureID::toCreature(const CreatureService * creatures) const
{
	return creatures->getById(*this);
}

si32 CreatureID::decode(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier("core", "creature", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string CreatureID::encode(const si32 index)
{
	return VLC->creatures()->getById(CreatureID(index))->getJsonKey();
}

const CSpell * SpellID::toSpell() const
{
	if(num < 0 || num >= VLC->spellh->objects.size())
	{
		logGlobal->error("Unable to get spell of invalid ID %d", int(num));
		return nullptr;
	}
	return VLC->spellh->objects[*this];
}

const spells::Spell * SpellID::toSpell(const spells::Service * service) const
{
	return service->getById(*this);
}

si32 SpellID::decode(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier("core", "spell", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string SpellID::encode(const si32 index)
{
	return VLC->spells()->getByIndex(index)->getJsonKey();
}

bool PlayerColor::isValidPlayer() const
{
	return num < PLAYER_LIMIT_I;
}

bool PlayerColor::isSpectator() const
{
	return num == 252;
}

std::string PlayerColor::getStr(bool L10n) const
{
	std::string ret = "unnamed";
	if(isValidPlayer())
	{
		if(L10n)
			ret = VLC->generaltexth->colors[num];
		else
			ret = GameConstants::PLAYER_COLOR_NAMES[num];
	}
	else if(L10n)
	{
		ret = VLC->generaltexth->allTexts[508];
		ret[0] = std::tolower(ret[0]);
	}

	return ret;
}

std::string PlayerColor::getStrCap(bool L10n) const
{
	std::string ret = getStr(L10n);
	ret[0] = std::toupper(ret[0]);
	return ret;
}

const FactionID FactionID::ANY = FactionID(-1);
const FactionID FactionID::CASTLE = FactionID(0);
const FactionID FactionID::RAMPART = FactionID(1);
const FactionID FactionID::TOWER = FactionID(2);
const FactionID FactionID::INFERNO = FactionID(3);
const FactionID FactionID::NECROPOLIS = FactionID(4);
const FactionID FactionID::DUNGEON = FactionID(5);
const FactionID FactionID::STRONGHOLD = FactionID(6);
const FactionID FactionID::FORTRESS = FactionID(7);
const FactionID FactionID::CONFLUX = FactionID(8);
const FactionID FactionID::NEUTRAL = FactionID(9);

si32 FactionID::decode(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier("core", "faction", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string FactionID::encode(const si32 index)
{
	return VLC->factions()->getByIndex(index)->getJsonKey();
}

std::ostream & operator<<(std::ostream & os, const EActionType actionType)
{
	static const std::map<EActionType, std::string> actionTypeToString =
	{
		{EActionType::END_TACTIC_PHASE, "End tactic phase"},
		{EActionType::INVALID, "Invalid"},
		{EActionType::NO_ACTION, "No action"},
		{EActionType::HERO_SPELL, "Hero spell"},
		{EActionType::WALK, "Walk"},
		{EActionType::DEFEND, "Defend"},
		{EActionType::RETREAT, "Retreat"},
		{EActionType::SURRENDER, "Surrender"},
		{EActionType::WALK_AND_ATTACK, "Walk and attack"},
		{EActionType::SHOOT, "Shoot"},
		{EActionType::WAIT, "Wait"},
		{EActionType::CATAPULT, "Catapult"},
		{EActionType::MONSTER_SPELL, "Monster spell"},
		{EActionType::BAD_MORALE, "Bad morale"},
		{EActionType::STACK_HEAL, "Stack heal"},
		{EActionType::DAEMON_SUMMONING, "Daemon summoning"}
	};

	auto it = actionTypeToString.find(actionType);
	if (it == actionTypeToString.end()) return os << "<Unknown type>";
	else return os << it->second;
}

std::ostream & operator<<(std::ostream & os, const ETerrainType terrainType)
{
	static const std::map<ETerrainType::EETerrainType, std::string> terrainTypeToString =
	{
	#define DEFINE_ELEMENT(element) {ETerrainType::element, #element}
		DEFINE_ELEMENT(WRONG),
		DEFINE_ELEMENT(BORDER),
		DEFINE_ELEMENT(DIRT),
		DEFINE_ELEMENT(SAND),
		DEFINE_ELEMENT(GRASS),
		DEFINE_ELEMENT(SNOW),
		DEFINE_ELEMENT(SWAMP),
		DEFINE_ELEMENT(ROUGH),
		DEFINE_ELEMENT(SUBTERRANEAN),
		DEFINE_ELEMENT(LAVA),
		DEFINE_ELEMENT(WATER),
		DEFINE_ELEMENT(ROCK)
	#undef DEFINE_ELEMENT
	};

	auto it = terrainTypeToString.find(terrainType.num);
	if (it == terrainTypeToString.end()) return os << "<Unknown type>";
	else return os << it->second;
}

std::string ETerrainType::toString() const
{
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream & operator<<(std::ostream & os, const EPathfindingLayer pathfindingLayer)
{
	static const std::map<EPathfindingLayer::EEPathfindingLayer, std::string> pathfinderLayerToString
	{
	#define DEFINE_ELEMENT(element) {EPathfindingLayer::element, #element}
		DEFINE_ELEMENT(WRONG),
		DEFINE_ELEMENT(AUTO),
		DEFINE_ELEMENT(LAND),
		DEFINE_ELEMENT(SAIL),
		DEFINE_ELEMENT(WATER),
		DEFINE_ELEMENT(AIR),
		DEFINE_ELEMENT(NUM_LAYERS)
	#undef DEFINE_ELEMENT
	};

	auto it = pathfinderLayerToString.find(pathfindingLayer.num);
	if (it == pathfinderLayerToString.end()) return os << "<Unknown type>";
	else return os << it->second;
}
