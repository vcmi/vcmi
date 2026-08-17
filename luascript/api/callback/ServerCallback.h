/*
 * ServerCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/ServerCallback.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/IBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusList.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

struct CObstacleInstance;

namespace battle { class Unit; class UnitInfo; class Destination; }
namespace spells { class Spell; }

namespace scripting::api
{

struct BonusDescriptor;
struct LuaMetaString;
struct SpellObstacleDescriptor;

class ServerCallbackProxy : public RawPointerWrapper<ServerCallback, ServerCallbackProxy>
{
public:
	static constexpr std::string_view luaName = "BattleServer";
	static constexpr std::string_view luaDescription =
		"The authoritative battle mutation interface. Available only to scripts running on the "
		"server: spawn or remove battle units, move them, deal damage, alter bonuses, drop "
		"obstacles, append to the combat log, draw from the seeded RNG. Every call emits a "
		"network pack so clients receive the resulting state change.";

	static void registerMethods(MethodRegistrar & R);

	static const battle::Unit * addUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::UnitInfo & info);
	static void removeUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit);
	static void removeObstacle(ServerCallback & object, const IBattleInfoCallback & battle, std::shared_ptr<const CObstacleInstance> obstacle);
	static void moveUnit(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, BattleHex destination, bool isTeleport);
	static void appendLog(ServerCallback & object, const IBattleInfoCallback & battle, const LuaMetaString & config);
	static bool describeChanges(ServerCallback & object);
	static void removeUnitBonuses(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, const BonusList & bonusList);
	static void addUnitBonus(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, const BonusDescriptor & data, bool cumulative);
	static void addBattleBonus(ServerCallback & object, const IBattleInfoCallback & battle, const BonusDescriptor & data);
	static void addObstacle(ServerCallback & object, const IBattleInfoCallback & battle, const SpellObstacleDescriptor & descriptor);
	static void catapultAttack(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit * attacker, EWallPart attackedPart, int32_t damageDealt);
	static bool rollCombatAbility(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & actor, int percentageChance);
	static void applySpellEffects(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & caster, const spells::Spell & spell, const std::vector<const battle::Unit *> & target, int spellLevel, int effectDuration, bool ignoreImmunity);
	static void refreshBattleUnits(ServerCallback & object, const IBattleInfoCallback & battle);
	static void showBattleAnimation(ServerCallback & object, const IBattleInfoCallback & battle, const std::vector<battle::Destination> & target, const std::string & animation, const std::string & sound, double transparency, std::optional<bool> deferred);
	static void castSpell(ServerCallback & object, const IBattleInfoCallback & battle, const battle::Unit & caster, const spells::Spell & spell, const std::vector<const battle::Unit *> & target, int64_t effectValue);
	static int rngInt(ServerCallback & object, int low, int high);
	static int rngBinomial(ServerCallback & object, int trials, double chance);
	static int healUnit(lua_State * L);
	static int changeUnit(lua_State * L); // args: battle, unitState, [healthDelta=0]
	static int damageUnit(lua_State * L); // args: battle, unit, damageAmount; returns: actualDamage, killedAmount
};

}
