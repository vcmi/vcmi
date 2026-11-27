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
#include "NoTargetActions.h"
#include "ShootAction.h"
#include "SpellCastActions.h"
#include "WalkAction.h"

#include "../../GameLibrary.h"
#include "../../modding/IdentifierStorage.h"
#include "../../modding/LibraryLoadContext.h"
#include "../../modding/ModScope.h"

static const SkipTurnAction skipTurnAction(EActionType::NO_ACTION);
static const BadMoraleAction badMoraleAction(EActionType::BAD_MORALE);
static const WaitAction waitAction(EActionType::WAIT);
static const DefendAction defendAction(EActionType::DEFEND);
static const EndTacticsAction endTacticsAction(EActionType::END_TACTIC_PHASE);
static const RetreatAction retreatAction(EActionType::RETREAT);
static const SurrenderAction surrenderAction(EActionType::SURRENDER);
static const WalkAction walkAction(EActionType::WALK);
static const ShootAction shootAction(EActionType::SHOOT);
static const HealAction healAction(EActionType::STACK_HEAL);
static const CatapultAction catapultAction(EActionType::CATAPULT);
static const WalkAndAttackAction walkAndAttackAction(EActionType::WALK_AND_ATTACK);
static const WalkAndAttackNoReturnAction walkAndAttackNoReturnAction(EActionType::WALK_AND_ATTACK_NO_RETURN);
static const LongWeaponAttackAction longWeaponAttackAction(EActionType::LONG_WEAPON_ATTACK);
static const HeroSpellAction heroSpellAction(EActionType::HERO_SPELL);
static const UnitSpellAction unitSpellAction(EActionType::MONSTER_SPELL);
static const RandomSpellAction randomSpellAction(EActionType::RANDOM_SPELL);
static const WalkAndCastAction walkAndCastAction(EActionType::WALK_AND_CAST);

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
	{&endTacticsAction, "endTacticsPhase"},
	{&retreatAction, "retreat"},
	{&surrenderAction, "surrender"},
	{&walkAction, "move"},
	{&shootAction, "rangedAttack"},
	{&healAction, "heal"},
	{&catapultAction, "catapult"},
	{&walkAndAttackAction, "meleeAttack"},
	{&walkAndAttackNoReturnAction, "meleeAttackNoReturn"},
	{&longWeaponAttackAction, "longWeaponAttack"},
	{&heroSpellAction, "heroSpell"},
	{&unitSpellAction, "unitSpell"},
	{&randomSpellAction, "randomSpell"},
	{&walkAndCastAction, "adjacentSpell"},
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
