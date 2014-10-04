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

#include "VCMI_Lib.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CSpellHandler.h"

const SlotID SlotID::COMMANDER_SLOT_PLACEHOLDER = SlotID(-2);
const PlayerColor PlayerColor::CANNOT_DETERMINE = PlayerColor(253);
const PlayerColor PlayerColor::UNFLAGGABLE = PlayerColor(254);
const PlayerColor PlayerColor::NEUTRAL = PlayerColor(255);
const PlayerColor PlayerColor::PLAYER_LIMIT = PlayerColor(PLAYER_LIMIT_I);
const TeamID TeamID::NO_TEAM = TeamID(255);

#define ID_LIKE_OPERATORS_INTERNAL(A, B, AN, BN)	\
bool operator==(const A & a, const B & b)			\
{													\
	return AN == BN ;								\
}													\
bool operator!=(const A & a, const B & b)			\
{													\
	return AN != BN ;								\
}													\
bool operator<(const A & a, const B & b)			\
{													\
	return AN < BN ;								\
}													\
bool operator<=(const A & a, const B & b)			\
{													\
	return AN <= BN ;								\
}													\
bool operator>(const A & a, const B & b)			\
{													\
	return AN > BN ;								\
}													\
bool operator>=(const A & a, const B & b)			\
{													\
	return AN >= BN ;								\
}

#define ID_LIKE_OPERATORS(CLASS_NAME, ENUM_NAME)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, CLASS_NAME, a.num, b.num)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, ENUM_NAME, a.num, b)	\
	ID_LIKE_OPERATORS_INTERNAL(ENUM_NAME, CLASS_NAME, a, b.num)


ID_LIKE_OPERATORS(SecondarySkill, SecondarySkill::ESecondarySkill)

ID_LIKE_OPERATORS(Obj, Obj::EObj)

ID_LIKE_OPERATORS(ETerrainType, ETerrainType::EETerrainType)

ID_LIKE_OPERATORS(ArtifactID, ArtifactID::EArtifactID)

ID_LIKE_OPERATORS(ArtifactPosition, ArtifactPosition::EArtifactPosition)

ID_LIKE_OPERATORS(CreatureID, CreatureID::ECreatureID)

ID_LIKE_OPERATORS(SpellID, SpellID::ESpellID)

ID_LIKE_OPERATORS(BuildingID, BuildingID::EBuildingID)

ID_LIKE_OPERATORS(BFieldType, BFieldType::EBFieldType)

CArtifact * ArtifactID::toArtifact() const
{
	return VLC->arth->artifacts[*this];
}

CCreature * CreatureID::toCreature() const
{
	return VLC->creh->creatures[*this];
}

CSpell * SpellID::toSpell() const
{
	return VLC->spellh->objects[*this];
}

//template std::ostream & operator << <ArtifactInstanceID>(std::ostream & os, BaseForID<ArtifactInstanceID> id);
//template std::ostream & operator << <ObjectInstanceID>(std::ostream & os, BaseForID<ObjectInstanceID> id);

bool PlayerColor::isValidPlayer() const
{
	return num < PLAYER_LIMIT_I;
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
	if (it == actionTypeToString.end()) return os << "<Unknown type>";
	else return os << it->second;
}
