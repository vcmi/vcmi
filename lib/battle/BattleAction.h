/*
 * BattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "Destination.h"
#include "../GameConstants.h"

class CBattleInfoCallback;

namespace battle
{
	class Unit;
}

/// A struct which handles battle actions like defending, walking,... - represents a creature stack in a battle
class DLL_LINKAGE BattleAction
{
public:
	BattleSide side; //who made this action
	ui32 stackNumber; //stack ID, -1 left hero, -2 right hero,
	BattleActionID actionType;

	SpellID spell;

	BattleAction();

	static BattleAction makeHeal(const battle::Unit * healer, const battle::Unit * healed);
	/// Heal of the most wounded unit that the healer can heal, chosen when the action is applied
	static BattleAction makeHeal(const battle::Unit * healer);
	static BattleAction makeSkipTurn(const battle::Unit * stack);
	static BattleAction makeBadMorale(const battle::Unit * stack);
	static BattleAction makeDefend(const battle::Unit * stack);
	static BattleAction makeWait(const battle::Unit * stack);
	/// Melee attack from a hex next to the target. A unit with RETURN_AFTER_STRIKE returns to its position
	static BattleAction makeMeleeAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom);
	static BattleAction makeMeleeAttack(const battle::Unit * stack, const battle::Unit * target, const BattleHex & attackFrom);
	/// Attack of a LONG_WEAPON unit over one free hex
	static BattleAction makeLongWeaponAttack(const battle::Unit * stack, const BattleHex & destination, const BattleHex & attackFrom);
	static BattleAction makeRangedAttack(const battle::Unit * shooter, const battle::Unit * target);
	static BattleAction makeRangedAttack(const battle::Unit * shooter, const BattleHex & destination);
	/// Catapult shot without a target, at a wall part that the spell picks
	static BattleAction makeCatapultShot(const battle::Unit * catapult);
	static BattleAction makeAdjacentSpell(const battle::Unit * stack, const BattleHex & castFrom, const battle::Unit * target, const SpellID & spellID);
	static BattleAction makeUnitSpell(const battle::Unit * stack, const battle::Target & target, const SpellID & spellID);
	static BattleAction makeHeroSpell(BattleSide side, const battle::Target & target, const SpellID & spellID);
	/// Random spell of a RANDOM_SPELLCASTER unit at the unit on the hex. The server rolls the spell
	static BattleAction makeRandomSpell(const battle::Unit * stack, const BattleHex & destination);
	static BattleAction makeMove(const battle::Unit * stack, const BattleHex & dest);
	static BattleAction makeEndTacticsPhase(BattleSide side);
	static BattleAction makeRetreat(BattleSide side);
	static BattleAction makeSurrender(BattleSide side);

	bool isTacticsAction() const;
	bool isUnitAction() const;
	bool isBattleEndAction() const;
	std::string toString() const;

	void aimToHex(const BattleHex & destination);
	void aimToUnit(const battle::Unit * destination);

	battle::Target getTarget(const CBattleInfoCallback * cb) const;
	void setTarget(const battle::Target & target_);

	template <typename Handler> void serialize(Handler & h)
	{
		h & side;
		h & stackNumber;
		if(h.hasFeature(Handler::Version::BATTLE_ACTION_ID))
		{
			h & actionType;
		}
		else
		{
			// older versions send the number of the type, which BattleActionID keeps for built-in types
			int32_t legacyType = actionType.getNum();
			h & legacyType;
			actionType = BattleActionID(legacyType);
		}
		h & spell;
		h & target;
	}

	struct DestinationInfo
	{
		int32_t unitValue;
		BattleHex hexValue;

		template <typename Handler> void serialize(Handler & h)
		{
			h & unitValue;
			h & hexValue;
		}
	};

	std::vector<DestinationInfo> target;

private:
	static BattleAction makeUnitAction(const battle::Unit * unit, BattleActionID type);
	static BattleAction makeSideAction(BattleSide side, BattleActionID type);
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleAction & ba); //todo: remove
