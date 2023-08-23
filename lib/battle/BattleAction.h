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

VCMI_LIB_NAMESPACE_BEGIN

class CBattleInfoCallback;

namespace battle
{
	class Unit;
}

/// A struct which handles battle actions like defending, walking,... - represents a creature stack in a battle
class DLL_LINKAGE BattleAction
{
public:
	ui8 side; //who made this action
	ui32 stackNumber; //stack ID, -1 left hero, -2 right hero,
	EActionType actionType; //use ActionType enum for values

	SpellID spell;

	BattleAction();

	static BattleAction makeHeal(const battle::Unit * healer, const battle::Unit * healed);
	static BattleAction makeDefend(const battle::Unit * stack);
	static BattleAction makeWait(const battle::Unit * stack);
	static BattleAction makeMeleeAttack(const battle::Unit * stack, BattleHex destination, BattleHex attackFrom, bool returnAfterAttack = true);
	static BattleAction makeShotAttack(const battle::Unit * shooter, const battle::Unit * target);
	static BattleAction makeCreatureSpellcast(const battle::Unit * stack, const battle::Target & target, const SpellID & spellID);
	static BattleAction makeMove(const battle::Unit * stack, BattleHex dest);
	static BattleAction makeEndOFTacticPhase(ui8 side);
	static BattleAction makeRetreat(ui8 side);
	static BattleAction makeSurrender(ui8 side);

	bool isTacticsAction() const;
	bool isUnitAction() const;
	bool isSpellAction() const;
	bool isBattleEndAction() const;
	std::string toString() const;

	void aimToHex(const BattleHex & destination);
	void aimToUnit(const battle::Unit * destination);

	battle::Target getTarget(const CBattleInfoCallback * cb) const;
	void setTarget(const battle::Target & target_);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & side;
		h & stackNumber;
		h & actionType;
		h & spell;
		h & target;
	}
private:

	struct DestinationInfo
	{
		int32_t unitValue;
		BattleHex hexValue;

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & unitValue;
			h & hexValue;
		}
	};

	std::vector<DestinationInfo> target;
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleAction & ba); //todo: remove

VCMI_LIB_NAMESPACE_END
