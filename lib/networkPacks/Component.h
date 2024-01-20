/*
 * Component.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/VariantIdentifier.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class ComponentType : int8_t
{
	NONE = -1,
	PRIM_SKILL,
	SEC_SKILL,
	RESOURCE,
	RESOURCE_PER_DAY,
	CREATURE,
	ARTIFACT,
	SPELL_SCROLL,
	MANA,
	EXPERIENCE,
	LEVEL,
	SPELL,
	MORALE,
	LUCK,
	BUILDING,
	HERO_PORTRAIT,
	FLAG
};

using ComponentSubType = VariantIdentifier<PrimarySkill, SecondarySkill, GameResID, CreatureID, ArtifactID, SpellID, BuildingTypeUniqueID, HeroTypeID, PlayerColor>;

struct Component
{
	ComponentType type = ComponentType::NONE;
	ComponentSubType subType;
	std::optional<int32_t> value; // + give; - take

	template <typename Handler> void serialize(Handler &h)
	{
		h & type;
		h & subType;
		h & value;
	}

	Component() = default;

	template<typename Numeric, std::enable_if_t<std::is_arithmetic_v<Numeric>, bool> = true>
	Component(ComponentType type, Numeric value)
		: type(type)
		, value(value)
	{
	}

	template<typename IdentifierType, std::enable_if_t<std::is_base_of_v<IdentifierBase, IdentifierType>, bool> = true>
	Component(ComponentType type, IdentifierType subType)
		: type(type)
		, subType(subType)
	{
	}

	Component(ComponentType type, ComponentSubType subType, int32_t value)
		: type(type)
		, subType(subType)
		, value(value)
	{
	}
};

VCMI_LIB_NAMESPACE_END
