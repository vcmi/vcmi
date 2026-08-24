/*
 * Bonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Bonus.h"

#include "../Enums.h"
#include "../Registry.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/CBonusTypeHandler.h"
#include "../../../lib/bonuses/BonusParameters.h"

namespace scripting::api
{

// --- BonusProxy ---

void BonusProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&BonusProxy::getType>("getType", {},
		"Returns the bonus type as its JSON key.");
	R.function<&BonusProxy::getVal>("getVal", {},
		"Returns the integer magnitude of the bonus.");
	R.function<&BonusProxy::getSubtype>("getSubtype", {},
		"Returns the bonus subtype as a JSON key (the meaning depends on getType).");
	R.function<&BonusProxy::getSourceID>("getSourceID", {},
		"Returns the JSON key identifying the entity that granted this bonus.");
	R.function<&BonusProxy::getSource>("getSource", {},
		"Returns the source category (artifact, creature ability, spell, ...) of the bonus.");
	R.function<&BonusProxy::getEffectRange>("getEffectRange", {},
		"Returns the kind of combat the bonus is limited to. A bonus that applies to melee only is "
		"silently absent while shooting, and the other way round.");
	R.function<&BonusProxy::getDuration>("getDuration", {},
		"Returns the list of duration flags currently set on the bonus.");
	R.function<&BonusProxy::getValType>("getValType", {},
		"Returns how the value combines with other bonuses (additive, percent, base number, ...).");
	R.function<&BonusProxy::getStacking>("getStacking", {},
		"Returns the stacking key — bonuses with the same key from the same source do not stack.");
	R.function<&BonusProxy::getTurnsRemain>("getTurnsRemain", {},
		"Returns the remaining turns until the bonus expires (0 if not turn-limited, only active if duration is set accordingly).");
	R.function<&BonusProxy::isHidden>("isHidden", {},
		"True if the bonus is hidden from the player's interface display.");
	R.function<&BonusProxy::getParametersAsNumber>("getParametersAsNumber", {},
		"Returns the bonus's extra parameters encoded as a single integer (0 if none).");
	R.function<&BonusProxy::getParametersAsVector>("getParametersAsVector", {},
		"Returns the bonus's extra parameters as a list of integers (empty if not stored as an array).");
}

std::string BonusProxy::getType(const Bonus & b)
{
	return LIBRARY->bth->bonusToString(b.type);
}

si32        BonusProxy::getVal(const Bonus & b)        { return b.val; }
std::string BonusProxy::getSubtype(const Bonus & b)    { return b.subtype.toString(); }
std::string BonusProxy::getSourceID(const Bonus & b)   { return b.sid.toString(); }
BonusSource     BonusProxy::getSource(const Bonus & b)  { return b.source; }
BonusLimitEffect BonusProxy::getEffectRange(const Bonus & b) { return b.effectRange; }
BonusValueType  BonusProxy::getValType(const Bonus & b) { return b.valType; }
std::string BonusProxy::getStacking(const Bonus & b)   { return b.stacking; }
si16        BonusProxy::getTurnsRemain(const Bonus & b) { return b.turnsRemain; }
bool        BonusProxy::isHidden(const Bonus & b)      { return b.hidden; }
si32        BonusProxy::getParametersAsNumber(const Bonus & b) { return b.parameters ? b.parameters->toNumber() : 0; }

std::vector<int32_t> BonusProxy::getParametersAsVector(const Bonus & b)
{
	if (b.parameters && b.parameters->isVector())
		return b.parameters->toVector();
	return {};
}

std::vector<BonusDuration::BonusDuration> BonusProxy::getDuration(const Bonus & b)
{
	static constexpr std::array all = {
		BonusDuration::PERMANENT,
		BonusDuration::ONE_BATTLE,
		BonusDuration::ONE_DAY,
		BonusDuration::ONE_WEEK,
		BonusDuration::N_TURNS,
		BonusDuration::N_DAYS,
		BonusDuration::UNTIL_BEING_ATTACKED,
		BonusDuration::UNTIL_ATTACK,
		BonusDuration::STACK_GETS_TURN,
		BonusDuration::COMMANDER_KILLED,
		BonusDuration::UNTIL_OWN_ATTACK,
		BonusDuration::UNTIL_TAKING_INDIRECT_DAMAGE,
		BonusDuration::UNTIL_AFTER_ATTACK_SEQUENCE,
	};
	std::vector<BonusDuration::BonusDuration> result;
	for (auto d : all)
		if (b.duration & d)
			result.push_back(d);
	return result;
}

// --- BonusListProxy ---

void BonusListProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&BonusListProxy::size>("size", {},
		"Returns the number of bonuses in this list.");
	R.function<&BonusListProxy::totalValue>("totalValue", {},
		"Computes total value of bonuses in the list, accounting for bonus value types");
	R.cfunction<&BonusListProxy::filter>("filter",
		{{"predicate", "fun(b: Bonus): boolean", "Selector — called for each bonus of the list; bonus is kept when it returns true."}},
		{"BonusList", "Bonuses for which the predicate returned true."},
		"Returns the bonuses of this list the predicate accepts. Use to narrow a list down before "
		"`totalValue`, which combines what is left by the rules of the engine.");
	R.function<&BonusListProxy::getBonus>("getBonus",
		{{"index", "1-based position of the bonus to fetch."}},
		{"Bonus stored at the given position."},
		"Returns the bonus at the given 1-based index. Aborts the script if the index is out of range.");
}

int32_t BonusListProxy::size(const BonusList & list)
{
	return static_cast<int32_t>(list.size());
}

int32_t BonusListProxy::totalValue(const BonusList & list)
{
	return list.totalValue();
}

int BonusListProxy::filter(lua_State * L)
{
	LuaStack S(L);

	BonusList list;
	S.get(1, list);

	if(!lua_isfunction(L, 2))
	{
		S.clear();
		return 0;
	}

	BonusList result;
	for(const auto & bonus : list)
	{
		lua_pushvalue(L, 2);
		S.push(*bonus);
		lua_call(L, 1, 1);
		const bool keep = lua_toboolean(L, -1);
		lua_pop(L, 1);
		if(keep)
			result.push_back(bonus);
	}

	S.clear();
	S.push(result);
	return 1;
}

Bonus BonusListProxy::getBonus(const BonusList & list, int32_t index)
{
	if (index < 1 || index > static_cast<int32_t>(list.size()))
		throw std::out_of_range("BonusList index out of range");
	return *list[index - 1];
}

}
