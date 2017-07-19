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

#include "VCMI_Lib.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "spells/CSpellHandler.h"
#include "StringConstants.h"
#include "CGeneralTextHandler.h"

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

const CArtifact * ArtifactID::toArtifact() const
{
	return VLC->arth->artifacts.at(*this);
}

const CCreature * CreatureID::toCreature() const
{
	return VLC->creh->creatures.at(*this);
}

const CSpell * SpellID::toSpell() const
{
	if(num < 0 || num >= VLC->spellh->objects.size())
	{
		logGlobal->errorStream() << "Unable to get spell of invalid ID " << int(num);
		return nullptr;
	}
	return VLC->spellh->objects[*this];
}

//template std::ostream & operator << <ArtifactInstanceID>(std::ostream & os, BaseForID<ArtifactInstanceID> id);
//template std::ostream & operator << <ObjectInstanceID>(std::ostream & os, BaseForID<ObjectInstanceID> id);

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

std::ostream & operator<<(std::ostream & os, const Battle::ActionType actionType)
{
	static const std::map<Battle::ActionType, std::string> actionTypeToString =
	{
		{Battle::END_TACTIC_PHASE, "End tactic phase"},
		{Battle::INVALID, "Invalid"},
		{Battle::NO_ACTION, "No action"},
		{Battle::HERO_SPELL, "Hero spell"},
		{Battle::WALK, "Walk"},
		{Battle::DEFEND, "Defend"},
		{Battle::RETREAT, "Retreat"},
		{Battle::SURRENDER, "Surrender"},
		{Battle::WALK_AND_ATTACK, "Walk and attack"},
		{Battle::SHOOT, "Shoot"},
		{Battle::WAIT, "Wait"},
		{Battle::CATAPULT, "Catapult"},
		{Battle::MONSTER_SPELL, "Monster spell"},
		{Battle::BAD_MORALE, "Bad morale"},
		{Battle::STACK_HEAL, "Stack heal"},
		{Battle::DAEMON_SUMMONING, "Daemon summoning"}
	};

	auto it = actionTypeToString.find(actionType);
	if(it == actionTypeToString.end())
		return os << "<Unknown type>";
	else
		return os << it->second;
}

std::ostream & operator<<(std::ostream & os, const ETerrainType terrainType)
{
	static const std::map<ETerrainType::EETerrainType, std::string> terrainTypeToString =
	{
	#define DEFINE_ELEMENT(element) {ETerrainType::element, #element \
}
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
	if(it == terrainTypeToString.end())
		return os << "<Unknown type>";
	else
		return os << it->second;
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
	#define DEFINE_ELEMENT(element) {EPathfindingLayer::element, #element \
}
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
	if(it == pathfinderLayerToString.end())
		return os << "<Unknown type>";
	else
		return os << it->second;
}
