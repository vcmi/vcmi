/*
 * Catapult.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Catapult.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::string EFFECT_NAME = "core:catapult";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Catapult, EFFECT_NAME);

Catapult::Catapult()
	: LocationEffect(),
	targetsToAttack(0)
{
}

Catapult::~Catapult() = default;

bool Catapult::applicable(Problem & problem, const Mechanics * m) const
{
	auto town = m->battle()->battleGetDefendedTown();

	if(nullptr == town)
	{
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	if(CGTownInstance::NONE == town->fortLevel())
	{
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	if(m->isSmart() && m->casterSide != BattleSide::ATTACKER)
	{
		//if spell targeting is smart, then only attacker can use it
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	const auto attackableBattleHexes = m->battle()->getAttackableBattleHexes();

	return !attackableBattleHexes.empty() || m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
}

void Catapult::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & /* eTarget */) const
{
	//start with all destructible parts
	static const std::set<EWallPart::EWallPart> possibleTargets =
	{
		EWallPart::KEEP,
		EWallPart::BOTTOM_TOWER,
		EWallPart::BOTTOM_WALL,
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::UPPER_WALL,
		EWallPart::UPPER_TOWER,
		EWallPart::GATE
	};

	assert(possibleTargets.size() == EWallPart::PARTS_COUNT);

	CatapultAttack ca;
	ca.attacker = -1;

	BattleUnitsChanged removeUnits;

	for(int i = 0; i < targetsToAttack; i++)
	{
		//Any destructible part can be hit regardless of its HP. Multiple hit on same target is allowed.
		EWallPart::EWallPart target = *RandomGeneratorUtil::nextItem(possibleTargets, *server->getRNG());

		auto state = m->battle()->battleGetWallState(target);

		if(state == EWallState::DESTROYED || state == EWallState::NONE)
			continue;

		CatapultAttack::AttackInfo attackInfo;

		attackInfo.damageDealt = 1;
		attackInfo.attackedPart = target;
		attackInfo.destinationTile = m->battle()->wallPartToBattleHex(target);

		ca.attackedParts.push_back(attackInfo);

		//removing creatures in turrets / keep if one is destroyed
		BattleHex posRemove;

		switch(target)
		{
		case EWallPart::KEEP:
			posRemove = -2;
			break;
		case EWallPart::BOTTOM_TOWER:
			posRemove = -3;
			break;
		case EWallPart::UPPER_TOWER:
			posRemove = -4;
			break;
		}

		if(posRemove != BattleHex::INVALID && state - attackInfo.damageDealt <= 0) //HP enum subtraction not intuitive, consider using SiegeInfo::applyDamage
		{
			auto all = m->battle()->battleGetUnitsIf([=](const battle::Unit * unit)
			{
				return !unit->isGhost();
			});

			for(auto & elem : all)
			{
				if(elem->getPosition() == posRemove)
				{
					removeUnits.changedStacks.emplace_back(elem->unitId(), UnitChanges::EOperation::REMOVE);
					break;
				}
			}
		}
	}

	server->apply(&ca);

	if(!removeUnits.changedStacks.empty())
		server->apply(&removeUnits);
}

void Catapult::serializeJsonEffect(JsonSerializeFormat & handler)
{
	//TODO: add configuration unifying with Catapult ability
	handler.serializeInt("targetsToAttack", targetsToAttack);
}


}
}

VCMI_LIB_NAMESPACE_END
