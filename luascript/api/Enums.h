/*
 * Enums.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>
#include <vcmi/spells/Magic.h>

#include "SignatureOf.h"

#include "../../lib/constants/Enumerations.h"
#include "../../lib/bonuses/BonusEnum.h"
#include "../../lib/battle/BattleSide.h"
#include "../../lib/battle/CObstacleInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

/// One named integer-valued constant in an EnumGroup, with the description shown by LuaLS
/// on hover. The C++ value is what scripts get when they read `ENUM.<Group>.<key>`.
template<typename E>
struct EnumItem
{
	std::string key;
	E           value;
	std::string description;
};

/// Carrier for one enum group exported through `Enums::serializeScript`. Replaces the
/// plain `std::map<std::string, E>` so per-key descriptions can be attached.
template<typename E>
struct EnumGroup
{
	std::vector<EnumItem<E>> items;
};

class Enums : public scripting::ApiSerializable<Enums>
{
	EnumGroup<EHealLevel> exportHealLevel() const;
	EnumGroup<EHealPower> exportHealPower() const;
	EnumGroup<ESpellCastProblem> exportSpellCastProblem() const;
	EnumGroup<spells::AimType> exportAimType() const;
	EnumGroup<BonusDuration::BonusDuration> exportBonusDuration() const;
	EnumGroup<BonusSource> exportBonusSource() const;
	EnumGroup<BonusValueType> exportBonusValueType() const;
	EnumGroup<CObstacleInstance::EObstacleType> exportObstacleType() const;
	EnumGroup<EWallPart> exportWallPart() const;
	EnumGroup<BattleSide> exportBattleSide() const;

public:
	static constexpr std::string_view luaName = "Enums";
	static constexpr std::string_view luaDescription =
		"The global `ENUM` table — string->integer constants scripts use to avoid magic "
		"numbers. Each field below is a named group; the runtime value is a Lua table whose "
		"keys are the constant names.";

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("HealLevel",        exportHealLevel(),        "Outcome categories for the heal action: plain heal / resurrect / overheal.");
		s("HealPower",        exportHealPower(),        "Persistence of healing effects: one-battle vs permanent.");
		s("SpellCastProblem", exportSpellCastProblem(), "Error codes returned from spell-cast validation.");
		s("AimType",          exportAimType(),          "Targeting modes a spell can require: none, creature, obstacle, location.");
		s("BonusDuration",    exportBonusDuration(),    "Lifetime selectors used by Bonus / BonusDescriptor `duration`.");
		s("BonusSource",      exportBonusSource(),      "Origin classes used by Bonus / BonusDescriptor `sourceType`.");
		s("BonusValueType",   exportBonusValueType(),   "Combination rules used by Bonus / BonusDescriptor `valueType`.");
		s("ObstacleType",     exportObstacleType(),     "Obstacle categories used by SpellObstacleDescriptor `obstacleType`.");
		s("WallPart",         exportWallPart(),         "Town-wall sections referenced by siege APIs and `catapultAttack`.");
		s("BattleSide",       exportBattleSide(),       "Battlefield side identifiers: none / attacker / defender.");
	}
};

}

VCMI_LIB_NAMESPACE_END
