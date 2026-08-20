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
using BonusSourceID = VariantIdentifier<BonusCustomSource, SpellID, CreatureID, ArtifactID, CampaignScenarioID, SecondarySkill, HeroTypeID, Obj, ObjectInstanceID, BuildingTypeUniqueID, BattleField, ArtifactInstanceID>;

/// Kind of entity an identifier names - a creature, a spell, a terrain, ... The same vocabulary as
/// `Identifier::entityType()`, as an enum. Bonus subtypes are given as plain identifiers
/// ("core:devil", "damageTypeMelee") with no say in what they name, so this is what the type of the
/// bonus resolves to before such an identifier can be looked up.
enum class EntityTypeEnum
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

/// What the subtype of a bonus of this type names.
DLL_LINKAGE EntityTypeEnum bonusSubtypeEntityType(BonusType type);
/// Name this entity type is known by, which is what identifiers of it are resolved under. Empty for NONE.
DLL_LINKAGE std::string entityTypeName(EntityTypeEnum entityType);
/// Subtype holding the given index, as an identifier of this entity type.
DLL_LINKAGE BonusSubtypeID bonusSubtypeOf(EntityTypeEnum entityType, int32_t index);
/// Subtype named by this identifier for this bonus type. Throws when the identifier names nothing.
DLL_LINKAGE BonusSubtypeID decodeBonusSubtype(BonusType type, const std::string & identifier);
