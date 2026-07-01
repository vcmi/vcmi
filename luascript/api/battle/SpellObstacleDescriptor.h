/*
 * SpellObstacleDescriptor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>

#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/constants/EntityIdentifiers.h"

#include "../SignatureOf.h"

#include <vcmi/spells/Spell.h>

VCMI_LIB_NAMESPACE_BEGIN

struct SpellCreatedObstacle;

namespace scripting::api
{

/// POD descriptor for a spell-created obstacle passed from Lua scripts.
struct SpellObstacleDescriptor final : ApiSerializable<SpellObstacleDescriptor>
{
	static constexpr std::string_view luaName = "SpellObstacleDescriptor";
	static constexpr std::string_view luaDescription =
		"Configuration for a battlefield obstacle that scripts hand to ServerCallback "
		"`addObstacle`. Captures anchor position, owning spell, caster stats, behavior flags, "
		"presentation assets, and an obstacle hex footprint.";

	BattleHex pos;
	CObstacleInstance::EObstacleType obstacleType = CObstacleInstance::SPELL_CREATED;
	const ::spells::Spell * spell = nullptr;
	int32_t turnsRemaining = -1;
	int32_t casterSpellPower = 0;
	int32_t spellLevel = 0;
	BattleSide casterSide = BattleSide::ATTACKER;
	int32_t minimalDamage = 0;

	bool hidden = false;
	bool passable = false;
	bool trap = false;
	bool removeOnTrigger = false;
	bool nativeVisible = true;

	std::string trigger;
	std::string appearSound;
	std::string appearAnimation;
	std::string animation;

	std::vector<BattleHex> customSize;

	/// Materializes a SpellCreatedObstacle ready to be sent through the battle pack.
	SpellCreatedObstacle toObstacle() const;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("pos",              pos,              "Anchor BattleHex of the obstacle (its primary position). Usually located in bottom-left corner of the obstacle");
		s("obstacleType",     obstacleType,     "Obstacle category — usual, absolute, spell-created, moat. See the ObstacleType enum.");
		s("spell",            spell,            "Spell that created the obstacle. Used for dispel and damage-source attribution. Only for spell-created obstacles");
		s("turnsRemaining",   turnsRemaining,   "How many turns the obstacle persists. -1 means permanent for the battle.");
		s("casterSpellPower", casterSpellPower, "Spell power of the caster at the moment of creation; feeds damage formulas.");
		s("spellLevel",       spellLevel,       "Spell skill level (0–3) the obstacle was cast at.");
		s("casterSide",       casterSide,       "Which battle side cast it; relevant for native-visibility and friendly-fire rules.");
		s("minimalDamage",    minimalDamage,    "Floor for the damage the obstacle inflicts on trigger.");
		s("hidden",           hidden,           "If true, the obstacle is invisible to the opposing side until triggered.");
		s("passable",         passable,         "If true, units may step onto the obstacle's hexes (e.g. trap obstacles).");
		s("trap",             trap,             "If true, behaves as a trap: triggers on enter rather than blocking movement.");
		s("removeOnTrigger",  removeOnTrigger,  "If true, the obstacle disappears after being triggered once.");
		s("nativeVisible",    nativeVisible,    "If true, the caster's side always sees the obstacle even when `hidden` is set.");
		s("trigger",          trigger,          "Script-side identifier of the trigger spell (looked up by the spell mechanics).");
		s("appearSound",      appearSound,      "Sound effect played when the obstacle appears.");
		s("appearAnimation",  appearAnimation,  "Animation played when the obstacle appears.");
		s("animation",        animation,        "Looping animation shown while the obstacle is on the battlefield.");
		s("customSize",       customSize,       "Optional list of BattleHex values defining a multi-hex footprint.");
	}
};

}

VCMI_LIB_NAMESPACE_END
