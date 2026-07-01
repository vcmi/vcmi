/*
 * UnitState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>
#include <vcmi/spells/Spell.h>

#include "../../../lib/battle/CUnitState.h"
#include "../../../lib/battle/BattleHexArray.h"
#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

class Creature;

namespace scripting::api
{

class LuaUnitState : public scripting::ApiCopyable<LuaUnitState>
{
	std::shared_ptr<::battle::CUnitState> state;
public:
	LuaUnitState() = default;
	explicit LuaUnitState(std::shared_ptr<::battle::CUnitState> state_);

	::battle::CUnitState * getState() const { return state.get(); }

	// Read-only API
	int  getMinDamage(bool ranged) const;
	int  getMaxDamage(bool ranged) const;
	int  getAttack(bool ranged) const;
	int  getDefense(bool ranged) const;
	bool isAlive() const;
	bool isClone() const;
	bool isDead() const;
	bool isGhost() const;
	bool isValidTarget(bool allowDead) const;
	bool isInvincible() const;
	bool isSummoned() const; // reads serialized field, not slot/bonus
	PlayerColor getOwner() const;
	SlotID getSlot() const;
	BattleHex getPosition() const;
	int64_t getTotalHealth() const;
	int64_t getAvailableHealth() const;
	int32_t getCount() const;
	uint32_t getMaxHealth() const;
	bool coversPos(BattleHex pos) const;
	int32_t getBaseAmount() const;

	// Mutable API
	void setDefending(bool v);
	void setCloned(bool v);
	void setDrainedMana(bool v);
	void setFear(bool v);
	void setHadMorale(bool v);
	void setCastSpellThisTurn(bool v);
	void setGhost(bool v);
	void setGhostPending(bool v);
	void setMoved(bool v);
	void setSummoned(bool v);
	void setWaiting(bool v);
	void setWaitedThisTurn(bool v);

	// Mutable — other public fields
	void setPosition(BattleHex hex);
	void setClone(const ::battle::Unit & unit);

	// Mutable — health via public CUnitState API
	int64_t damage(int64_t amount); // clamps to available health, returns actual damage dealt
};

class LuaUnitStateProxy : public CopyableWrapper<LuaUnitState, LuaUnitStateProxy>
{
public:
	static constexpr std::string_view luaName = "UnitState";
	static constexpr std::string_view luaDescription =
		"Editable copy of a battle Unit. Obtained via `Unit:copy()`; scripts edit fields "
		"(position, defending, clone, summoned, …) and inflict damage locally, then hand the "
		"result to ServerCallback `changeUnit` or `addUnit` to broadcast the update through a battle pack.";

	static void registerMethods(MethodRegistrar & R);

	static bool hasAbsoluteImmunity(const LuaUnitState & state, const spells::Spell & spell);
	static const Creature * getCreature(const LuaUnitState & state);
	static BattleHexArray getHexes(const LuaUnitState & state);
	static int heal(lua_State * L); // args: amount, level, power — returns healedHP, resurrected
};

}

VCMI_LIB_NAMESPACE_END
