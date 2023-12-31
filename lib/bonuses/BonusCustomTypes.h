/*
 * BonusCustomTypes.h, part of VCMI engine
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

class DLL_LINKAGE BonusCustomSource : public StaticIdentifier<BonusCustomSource>
{
public:
	using StaticIdentifier<BonusCustomSource>::StaticIdentifier;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	static const BonusCustomSource undeadMoraleDebuff; // -2
};

class DLL_LINKAGE BonusCustomSubtype : public StaticIdentifier<BonusCustomSubtype>
{
public:
	using StaticIdentifier<BonusCustomSubtype>::StaticIdentifier;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	static const BonusCustomSubtype creatureDamageBoth; // 0
	static const BonusCustomSubtype creatureDamageMin; // 1
	static const BonusCustomSubtype creatureDamageMax; // 2

	static const BonusCustomSubtype damageTypeAll; // -1
	static const BonusCustomSubtype damageTypeMelee; // 0
	static const BonusCustomSubtype damageTypeRanged; // 1

	static const BonusCustomSubtype heroMovementLand; // 1
	static const BonusCustomSubtype heroMovementSea; // 0

	static const BonusCustomSubtype deathStareGorgon; // 0
	static const BonusCustomSubtype deathStareCommander;

	static const BonusCustomSubtype rebirthRegular; // 0
	static const BonusCustomSubtype rebirthSpecial; // 1

	static const BonusCustomSubtype visionsMonsters; // 0
	static const BonusCustomSubtype visionsHeroes; // 1
	static const BonusCustomSubtype visionsTowns; // 2

	static const BonusCustomSubtype immunityBattleWide; // 0
	static const BonusCustomSubtype immunityEnemyHero; // 1

	static const BonusCustomSubtype transmutationPerHealth; // 0
	static const BonusCustomSubtype transmutationPerUnit; // 1

	static const BonusCustomSubtype destructionKillPercentage; // 0
	static const BonusCustomSubtype destructionKillAmount; // 1

	static const BonusCustomSubtype soulStealPermanent; // 0
	static const BonusCustomSubtype soulStealBattle; // 1

	static const BonusCustomSubtype movementFlying; // 0
	static const BonusCustomSubtype movementTeleporting; // 1

	static BonusCustomSubtype spellLevel(int level);
	static BonusCustomSubtype creatureLevel(int level);
};

VCMI_LIB_NAMESPACE_END
