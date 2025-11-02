/*
 * BonusCustomTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusCustomTypes.h"
#include "CBonusTypeHandler.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

const BonusCustomSubtype BonusCustomSubtype::creatureDamageBoth(0);
const BonusCustomSubtype BonusCustomSubtype::creatureDamageMin(1);
const BonusCustomSubtype BonusCustomSubtype::creatureDamageMax(2);
const BonusCustomSubtype BonusCustomSubtype::damageTypeAll(-1);
const BonusCustomSubtype BonusCustomSubtype::damageTypeMelee(0);
const BonusCustomSubtype BonusCustomSubtype::damageTypeRanged(1);
const BonusCustomSubtype BonusCustomSubtype::heroMovementLand(1);
const BonusCustomSubtype BonusCustomSubtype::heroMovementSea(0);
const BonusCustomSubtype BonusCustomSubtype::deathStareGorgon(0);
const BonusCustomSubtype BonusCustomSubtype::deathStareCommander(1);
const BonusCustomSubtype BonusCustomSubtype::deathStareNoRangePenalty(2);
const BonusCustomSubtype BonusCustomSubtype::deathStareRangePenalty(3);
const BonusCustomSubtype BonusCustomSubtype::deathStareObstaclePenalty(4);
const BonusCustomSubtype BonusCustomSubtype::deathStareRangeObstaclePenalty(5);
const BonusCustomSubtype BonusCustomSubtype::rebirthRegular(0);
const BonusCustomSubtype BonusCustomSubtype::rebirthSpecial(1);
const BonusCustomSubtype BonusCustomSubtype::visionsMonsters(0);
const BonusCustomSubtype BonusCustomSubtype::visionsHeroes(1);
const BonusCustomSubtype BonusCustomSubtype::visionsTowns(2);
const BonusCustomSubtype BonusCustomSubtype::immunityBattleWide(0);
const BonusCustomSubtype BonusCustomSubtype::immunityEnemyHero(1);
const BonusCustomSubtype BonusCustomSubtype::transmutationPerHealth(0);
const BonusCustomSubtype BonusCustomSubtype::transmutationPerUnit(1);
const BonusCustomSubtype BonusCustomSubtype::destructionKillPercentage(0);
const BonusCustomSubtype BonusCustomSubtype::destructionKillAmount(1);
const BonusCustomSubtype BonusCustomSubtype::soulStealPermanent(0);
const BonusCustomSubtype BonusCustomSubtype::soulStealBattle(1);
const BonusCustomSubtype BonusCustomSubtype::movementFlying(0);
const BonusCustomSubtype BonusCustomSubtype::movementTeleporting(1);

const BonusCustomSource BonusCustomSource::undeadMoraleDebuff(-2);

BonusCustomSubtype BonusCustomSubtype::spellLevel(int level)
{
	return BonusCustomSubtype(level);
}

BonusCustomSubtype BonusCustomSubtype::creatureLevel(int level)
{
	return BonusCustomSubtype(level);
}

si32 BonusCustomSubtype::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusCustomSubtype::encode(const si32 index)
{
	return std::to_string(index);
}

si32 BonusCustomSource::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusCustomSource::encode(const si32 index)
{
	return std::to_string(index);
}

std::string BonusTypeID::encode(int32_t index)
{
	if (index == static_cast<int32_t>(BonusType::NONE))
		return "";
	return LIBRARY->bth->bonusToString(static_cast<BonusType>(index));
}

si32 BonusTypeID::decode(const std::string & identifier)
{
	if (identifier.empty())
		return RiverId::NO_RIVER.getNum();

	return resolveIdentifier("bonus", identifier);
}

VCMI_LIB_NAMESPACE_END
