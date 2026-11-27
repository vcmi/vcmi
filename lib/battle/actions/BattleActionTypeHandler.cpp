/*
 * BattleActionTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionTypeHandler.h"

#include "CatapultAction.h"
#include "HealAction.h"
#include "MeleeAttackActions.h"
#include "MoveAction.h"
#include "NoTargetActions.h"
#include "RangedAttackAction.h"
#include "SpellCastActions.h"

#include "../../GameLibrary.h"
#include "../../modding/IdentifierStorage.h"
#include "../../modding/LibraryLoadContext.h"
#include "../../modding/ModScope.h"

static const SkipTurnAction skipTurnAction(BattleActionID::SKIP_TURN);
static const BadMoraleAction badMoraleAction(BattleActionID::BAD_MORALE);
static const WaitAction waitAction(BattleActionID::WAIT);
static const DefendAction defendAction(BattleActionID::DEFEND);
static const EndTacticsPhaseAction endTacticsPhaseAction(BattleActionID::END_TACTICS_PHASE);
static const RetreatAction retreatAction(BattleActionID::RETREAT);
static const SurrenderAction surrenderAction(BattleActionID::SURRENDER);
static const MoveAction moveAction(BattleActionID::MOVE);
static const RangedAttackAction rangedAttackAction(BattleActionID::RANGED_ATTACK);
static const HealAction healAction(BattleActionID::HEAL);
static const CatapultAction catapultAction(BattleActionID::CATAPULT);
static const MeleeAttackAction meleeAttackAction(BattleActionID::MELEE_ATTACK);
static const MeleeAttackNoReturnAction meleeAttackNoReturnAction(BattleActionID::MELEE_ATTACK_NO_RETURN);
static const LongWeaponAttackAction longWeaponAttackAction(BattleActionID::LONG_WEAPON_ATTACK);
static const HeroSpellAction heroSpellAction(BattleActionID::HERO_SPELL);
static const UnitSpellAction unitSpellAction(BattleActionID::UNIT_SPELL);
static const RandomSpellAction randomSpellAction(BattleActionID::RANDOM_SPELL);
static const AdjacentSpellAction adjacentSpellAction(BattleActionID::ADJACENT_SPELL);

struct BuiltInActionType
{
	const BattleActionType * type;
	const char * identifier;
};

static const std::array<BuiltInActionType, 18> builtInTypes = {{
	{&skipTurnAction, "skipTurn"},
	{&badMoraleAction, "badMorale"},
	{&waitAction, "wait"},
	{&defendAction, "defend"},
	{&endTacticsPhaseAction, "endTacticsPhase"},
	{&retreatAction, "retreat"},
	{&surrenderAction, "surrender"},
	{&moveAction, "move"},
	{&rangedAttackAction, "rangedAttack"},
	{&healAction, "heal"},
	{&catapultAction, "catapult"},
	{&meleeAttackAction, "meleeAttack"},
	{&meleeAttackNoReturnAction, "meleeAttackNoReturn"},
	{&longWeaponAttackAction, "longWeaponAttack"},
	{&heroSpellAction, "heroSpell"},
	{&unitSpellAction, "unitSpell"},
	{&randomSpellAction, "randomSpell"},
	{&adjacentSpellAction, "adjacentSpell"},
}};

BattleActionTypeHandler::BattleActionTypeHandler(LibraryLoadContext & context)
{
	typesById.resize(builtInTypes.size());
	jsonKeys.resize(builtInTypes.size());

	for(const auto & builtIn : builtInTypes)
	{
		const auto id = static_cast<int32_t>(builtIn.type->getId());
		typesById.at(id) = builtIn.type;
		jsonKeys.at(id) = ModScope::scopeBuiltin() + ":" + builtIn.identifier;
		types.push_back(builtIn.type);
		context.lib.identifiersHandler->registerObject(ModScope::scopeBuiltin(), "battleAction", builtIn.identifier, id);
	}
}

const BattleActionType * BattleActionTypeHandler::getById(int32_t id) const
{
	if(id < 0 || id >= static_cast<int32_t>(typesById.size()))
		return nullptr;
	return typesById[id];
}

const std::vector<const BattleActionType *> & BattleActionTypeHandler::getAll() const
{
	return types;
}

const std::string & BattleActionTypeHandler::getJsonKey(int32_t id) const
{
	if(!getById(id))
		throw std::runtime_error("No battle action type has id " + std::to_string(id));
	return jsonKeys[id];
}
