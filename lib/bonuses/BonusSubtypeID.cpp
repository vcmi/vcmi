/*
 * Bonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusSubtypeID.h"

VCMI_LIB_NAMESPACE_BEGIN

const BonusSubtypeID BonusSubtypeID::creatureDamageBoth(0);
const BonusSubtypeID BonusSubtypeID::creatureDamageMin(1);
const BonusSubtypeID BonusSubtypeID::creatureDamageMax(2);
const BonusSubtypeID BonusSubtypeID::damageTypeAll(-1);
const BonusSubtypeID BonusSubtypeID::damageTypeMelee(0);
const BonusSubtypeID BonusSubtypeID::damageTypeRanged(1);
const BonusSubtypeID BonusSubtypeID::heroMovementLand(1);
const BonusSubtypeID BonusSubtypeID::heroMovementSea(0);
const BonusSubtypeID BonusSubtypeID::heroMovementPenalty(2);
const BonusSubtypeID BonusSubtypeID::heroMovementFull(1);
const BonusSubtypeID BonusSubtypeID::deathStareGorgon(0);
const BonusSubtypeID BonusSubtypeID::deathStareCommander(1);
const BonusSubtypeID BonusSubtypeID::rebirthRegular(0);
const BonusSubtypeID BonusSubtypeID::rebirthSpecial(1);
const BonusSubtypeID BonusSubtypeID::visionsMonsters(0);
const BonusSubtypeID BonusSubtypeID::visionsHeroes(1);
const BonusSubtypeID BonusSubtypeID::visionsTowns(2);
const BonusSubtypeID BonusSubtypeID::immunityBattleWide(0);
const BonusSubtypeID BonusSubtypeID::immunityEnemyHero(1);
const BonusSubtypeID BonusSubtypeID::transmutationPerHealth(0);
const BonusSubtypeID BonusSubtypeID::transmutationPerUnit(1);
const BonusSubtypeID BonusSubtypeID::destructionKillPercentage(0);
const BonusSubtypeID BonusSubtypeID::destructionKillAmount(1);
const BonusSubtypeID BonusSubtypeID::soulStealPermanent(0);
const BonusSubtypeID BonusSubtypeID::soulStealBattle(1);
const BonusSubtypeID BonusSubtypeID::movementFlying(0);
const BonusSubtypeID BonusSubtypeID::movementTeleporting(1);

const BonusSourceID BonusSourceID::undeadMoraleDebuff(-2);

BonusSubtypeID BonusSubtypeID::spellLevel(int level)
{
	return BonusSubtypeID(level);
}

BonusSubtypeID BonusSubtypeID::creatureLevel(int level)
{
	return BonusSubtypeID(level);
}

si32 BonusSubtypeID::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusSubtypeID::encode(const si32 index)
{
	return std::to_string(index);
}

si32 BonusSourceID::decode(const std::string & identifier)
{
	return std::stoi(identifier);
}

std::string BonusSourceID::encode(const si32 index)
{
	return std::to_string(index);
}

VCMI_LIB_NAMESPACE_END
