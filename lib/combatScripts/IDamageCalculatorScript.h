/*
 * IDamageCalculatorScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../battle/BattleHex.h"
#include "../battle/IBattleInfoCallback.h"

#include <vcmi/scripting/ApiTags.h>

class CBattleInfoCallback;

namespace battle
{
class Unit;
}

/// One attack, as the damage calculator script is told about it. Positions are resolved before the
/// script sees them, so an attack that has not happened yet looks like any other.
struct DLL_LINKAGE DamageAttackInfo final : public scripting::ApiSerializable<DamageAttackInfo>
{
	const battle::Unit * attacker = nullptr;
	const battle::Unit * defender = nullptr;

	BattleHex attackerHex;
	BattleHex defenderHex;

	int chargeDistance = 0;
	bool shooting = false;
	bool luckyStrike = false;
	bool unluckyStrike = false;
	bool deathBlow = false;
	bool doubleDamage = false;

	/// Which of the bonus types the script declared an interest in each of the two carries
	std::unordered_map<std::string, bool> attackerBonuses;
	std::unordered_map<std::string, bool> defenderBonuses;

	// tuning constants of the game, handed over rather than looked up so that the script needs no
	// access to the settings
	double attackFactorPerPoint = 0.0;
	double attackFactorCap = 0.0;
	double defenseFactorPerPoint = 0.0;
	double defenseFactorCap = 0.0;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("attacker", attacker, "Unit dealing the blow.");
		s("defender", defender, "Unit receiving it.");
		s("attackerHex", attackerHex, "Hex the blow is dealt from.");
		s("defenderHex", defenderHex, "Hex the blow lands on.");
		s("attackerBonuses", attackerBonuses, "Bonus types the attacker carries.");
		s("defenderBonuses", defenderBonuses, "Bonus types the defender carries.");
		s("chargeDistance", chargeDistance, "Hexes crossed to reach the target, which is what jousting scales with.");
		s("shooting", shooting, "Whether the blow is a shot.");
		s("luckyStrike", luckyStrike, "Whether luck struck.");
		s("unluckyStrike", unluckyStrike, "Whether bad luck struck.");
		s("deathBlow", deathBlow, "Whether a death blow was rolled.");
		s("doubleDamage", doubleDamage, "Whether the attack is a doubled one, as a ballista may roll.");
		s("attackFactorPerPoint", attackFactorPerPoint, "Damage added per point of attack over the target's defense.");
		s("attackFactorCap", attackFactorCap, "Most that attack points alone may add.");
		s("defenseFactorPerPoint", defenseFactorPerPoint, "Damage removed per point of defense over the attacker's attack.");
		s("defenseFactorCap", defenseFactorCap, "Most that defense points alone may remove.");
	}
};

/// Lowest and highest of something the attack produces.
struct DLL_LINKAGE DamageRangePayload final : public scripting::ApiSerializable<DamageRangePayload>
{
	int64_t min = 0;
	int64_t max = 0;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("min", min, "Lowest value.");
		s("max", max, "Highest value.");
	}
};

/// What the damage calculator script answers with.
struct DLL_LINKAGE DamageEstimationPayload final : public scripting::ApiSerializable<DamageEstimationPayload>
{
	DamageRangePayload damage;
	DamageRangePayload kills;
	DamageRangePayload damageBeforeDefense;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("damage", damage, "Damage the blow deals.");
		s("kills", kills, "Creatures the blow kills.");
		s("damageBeforeDefense", damageBeforeDefense, "Damage the blow would deal with the defences of the target left out, which is what abilities reflecting a strike work from.");
	}
};

/// Answers what one attack is worth. Exactly one of these is active at a time - it is the damage
/// calculator of the game, not an ability some unit carries.
class DLL_LINKAGE IDamageCalculatorScript
{
public:
	virtual ~IDamageCalculatorScript() = default;

	/// `info` arrives with everything the engine knows and is completed by the implementation, which
	/// is what lets a script be told only about the bonuses it asked for.
	virtual DamageEstimation calculate(const CBattleInfoCallback & battle, DamageAttackInfo & info) const = 0;
};
