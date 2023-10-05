/*
 * BonusSubtypes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/MetaIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

using TBonusSubtype = MetaIdentifier;

class DLL_LINKAGE BonusSubtypes
{
public:
	static const TBonusSubtype creatureDamageBoth; // 0
	static const TBonusSubtype creatureDamageMin; // 1
	static const TBonusSubtype creatureDamageMax; // 2

	static const TBonusSubtype damageTypeAll; // -1
	static const TBonusSubtype damageTypeMelee; // 0
	static const TBonusSubtype damageTypeRanged; // 1

	static const TBonusSubtype heroMovementLand; // 1
	static const TBonusSubtype heroMovementSea; // 0

	static const TBonusSubtype heroMovementPenalty; // 2
	static const TBonusSubtype heroMovementFull; // 1

	static const TBonusSubtype deathStareGorgon; // 0
	static const TBonusSubtype deathStareCommander;

	static const TBonusSubtype rebirthRegular; // 0
	static const TBonusSubtype rebirthSpecial; // 1

	static const TBonusSubtype visionsMonsters; // 0
	static const TBonusSubtype visionsHeroes; // 1
	static const TBonusSubtype visionsTowns; // 2

	static const TBonusSubtype immunityBattleWide; // 0
	static const TBonusSubtype immunityEnemyHero; // 1

	static const TBonusSubtype transmutationPerHealth; // 0
	static const TBonusSubtype transmutationPerUnit; // 1

	static const TBonusSubtype destructionKillPercentage; // 0
	static const TBonusSubtype destructionKillAmount; // 1

	static const TBonusSubtype soulStealPermanent; // 0
	static const TBonusSubtype soulStealBattle; // 1

	static const TBonusSubtype movementFlying; // 0
	static const TBonusSubtype movementTeleporting; // 1

	static TBonusSubtype spellLevel(int level);
	static TBonusSubtype creatureLevel(int level);
};

VCMI_LIB_NAMESPACE_END
