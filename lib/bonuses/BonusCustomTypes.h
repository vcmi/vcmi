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
#include "../constants/Enumerations.h"
#include "../constants/VariantIdentifier.h"
#include "BonusEnum.h"

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

	static const BonusCustomSubtype rebirthRegular; // 0
	static const BonusCustomSubtype rebirthSpecial; // 1

	static const BonusCustomSubtype visionsMonsters; // 0
	static const BonusCustomSubtype visionsHeroes; // 1
	static const BonusCustomSubtype visionsTowns; // 2

	static const BonusCustomSubtype immunityBattleWide; // 0
	static const BonusCustomSubtype immunityEnemyHero; // 1

	static const BonusCustomSubtype movementFlying; // -1
	static const BonusCustomSubtype movementTeleporting; // 1

	static const BonusCustomSubtype freeShootingNoPenalty; // 0
	static const BonusCustomSubtype freeShootingExceptAdjacent; // 1

	static BonusCustomSubtype spellLevel(int level);
	static BonusCustomSubtype creatureLevel(int level);
	static BonusCustomSubtype alignment(EAlignment alignment);
};

class DLL_LINKAGE BonusTypeID : public EntityIdentifier<BonusTypeID>
{
public:
	using EntityIdentifier<BonusTypeID>::EntityIdentifier;
	using EnumType = BonusType;

	static std::string encode(int32_t index);
	static si32 decode(const std::string & identifier);

	constexpr EnumType toEnum() const
	{
		return static_cast<EnumType>(EntityIdentifier::num);
	}

	constexpr BonusTypeID(const EnumType & enumValue)
	{
		EntityIdentifier::num = static_cast<int32_t>(enumValue);
	}
};

using BonusSubtypeID = VariantIdentifier<BonusCustomSubtype, SpellID, CreatureID, PrimarySkill, TerrainId, GameResID, SpellSchool, BonusTypeID, ScriptID>;

/// What the subtype of a bonus names. Decided by the type of the bonus alone, which is what lets a
/// subtype be given as a plain identifier - "core:devil", "damageTypeMelee" - with no say in which
/// kind of thing that identifier is.
enum class BonusSubtypeKind
{
	NONE,
	CUSTOM,
	SPELL,
	CREATURE,
	PRIMARY_SKILL,
	TERRAIN,
	RESOURCE,
	SPELL_SCHOOL,
	BONUS_TYPE,
	SCRIPT
};

DLL_LINKAGE BonusSubtypeKind bonusSubtypeKind(BonusType type);
/// Name of the identifier namespace subtypes of this kind are resolved in, empty for NONE.
DLL_LINKAGE std::string bonusSubtypeEntity(BonusSubtypeKind kind);
DLL_LINKAGE BonusSubtypeID bonusSubtypeOf(BonusSubtypeKind kind, int32_t index);
/// Subtype named by this identifier for this bonus type. Throws when the identifier names nothing.
DLL_LINKAGE BonusSubtypeID decodeBonusSubtype(BonusType type, const std::string & identifier);
using BonusSourceID = VariantIdentifier<BonusCustomSource, SpellID, CreatureID, ArtifactID, CampaignScenarioID, SecondarySkill, HeroTypeID, Obj, ObjectInstanceID, BuildingTypeUniqueID, BattleField, ArtifactInstanceID>;
