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
#include "BonusSubtypes.h"

VCMI_LIB_NAMESPACE_BEGIN

const TBonusSubtype BonusSubtypes::creatureDamageBoth("", "", 0);
const TBonusSubtype BonusSubtypes::creatureDamageMin("", "", 1);
const TBonusSubtype BonusSubtypes::creatureDamageMax("", "", 2);
const TBonusSubtype BonusSubtypes::damageTypeAll("", "", -1);
const TBonusSubtype BonusSubtypes::damageTypeMelee("", "", 0);
const TBonusSubtype BonusSubtypes::damageTypeRanged("", "", 1);
const TBonusSubtype BonusSubtypes::heroMovementLand("", "", 1);
const TBonusSubtype BonusSubtypes::heroMovementSea("", "", 0);
const TBonusSubtype BonusSubtypes::heroMovementPenalty("", "", 2);
const TBonusSubtype BonusSubtypes::heroMovementFull("", "", 1);
const TBonusSubtype BonusSubtypes::deathStareGorgon("", "", 0);
const TBonusSubtype BonusSubtypes::deathStareCommander("", "", 1);
const TBonusSubtype BonusSubtypes::rebirthRegular("", "", 0);
const TBonusSubtype BonusSubtypes::rebirthSpecial("", "", 1);
const TBonusSubtype BonusSubtypes::visionsMonsters("", "", 0);
const TBonusSubtype BonusSubtypes::visionsHeroes("", "", 1);
const TBonusSubtype BonusSubtypes::visionsTowns("", "", 2);
const TBonusSubtype BonusSubtypes::immunityBattleWide("", "", 0);
const TBonusSubtype BonusSubtypes::immunityEnemyHero("", "", 1);
const TBonusSubtype BonusSubtypes::transmutationPerHealth("", "", 0);
const TBonusSubtype BonusSubtypes::transmutationPerUnit("", "", 1);
const TBonusSubtype BonusSubtypes::destructionKillPercentage("", "", 0);
const TBonusSubtype BonusSubtypes::destructionKillAmount("", "", 1);
const TBonusSubtype BonusSubtypes::soulStealPermanent("", "", 0);
const TBonusSubtype BonusSubtypes::soulStealBattle("", "", 1);
const TBonusSubtype BonusSubtypes::movementFlying("", "", 0);
const TBonusSubtype BonusSubtypes::movementTeleporting("", "", 1);

TBonusSubtype BonusSubtypes::spellLevel(int level)
{
	assert(0); //todo
	return TBonusSubtype();
}

TBonusSubtype BonusSubtypes::creatureLevel(int level)
{
	assert(0); //todo
	return TBonusSubtype();
}

VCMI_LIB_NAMESPACE_END
