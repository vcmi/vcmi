/*
 * BonusSubtypeID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE BonusSourceID : public Identifier<BonusSourceID>
{
public:
	using Identifier<BonusSourceID>::Identifier;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	static const BonusSourceID undeadMoraleDebuff; // -2
};

class DLL_LINKAGE BonusSubtypeID : public Identifier<BonusSubtypeID>
{
public:
	using Identifier<BonusSubtypeID>::Identifier;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	static const BonusSubtypeID creatureDamageBoth; // 0
	static const BonusSubtypeID creatureDamageMin; // 1
	static const BonusSubtypeID creatureDamageMax; // 2

	static const BonusSubtypeID damageTypeAll; // -1
	static const BonusSubtypeID damageTypeMelee; // 0
	static const BonusSubtypeID damageTypeRanged; // 1

	static const BonusSubtypeID heroMovementLand; // 1
	static const BonusSubtypeID heroMovementSea; // 0

	static const BonusSubtypeID deathStareGorgon; // 0
	static const BonusSubtypeID deathStareCommander;

	static const BonusSubtypeID rebirthRegular; // 0
	static const BonusSubtypeID rebirthSpecial; // 1

	static const BonusSubtypeID visionsMonsters; // 0
	static const BonusSubtypeID visionsHeroes; // 1
	static const BonusSubtypeID visionsTowns; // 2

	static const BonusSubtypeID immunityBattleWide; // 0
	static const BonusSubtypeID immunityEnemyHero; // 1

	static const BonusSubtypeID transmutationPerHealth; // 0
	static const BonusSubtypeID transmutationPerUnit; // 1

	static const BonusSubtypeID destructionKillPercentage; // 0
	static const BonusSubtypeID destructionKillAmount; // 1

	static const BonusSubtypeID soulStealPermanent; // 0
	static const BonusSubtypeID soulStealBattle; // 1

	static const BonusSubtypeID movementFlying; // 0
	static const BonusSubtypeID movementTeleporting; // 1

	static BonusSubtypeID spellLevel(int level);
	static BonusSubtypeID creatureLevel(int level);
};

VCMI_LIB_NAMESPACE_END
