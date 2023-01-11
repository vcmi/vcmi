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
	static const std::set<EWallPart::EWallPart> potentialTargets =
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

	assert(potentialTargets.size() == EWallPart::PARTS_COUNT);

	std::set<EWallPart::EWallPart> allowedTargets;

	for (auto const & target : potentialTargets)
	{
		auto state = m->battle()->battleGetWallState(target);

		if(state != EWallState::DESTROYED && state != EWallState::NONE)
			allowedTargets.insert(target);
	}
	assert(!allowedTargets.empty());
	if (allowedTargets.empty())
		return;

	CatapultAttack ca;
	ca.attacker = -1;

	BattleUnitsChanged removeUnits;

	for(int i = 0; i < targetsToAttack; i++)
	{
		// Hit on any existing, not destroyed targets are allowed
		// Multiple hit on same target are allowed.
		// Potential overshots (more hits on same targets than remaining HP) are allowed
		EWallPart::EWallPart target = *RandomGeneratorUtil::nextItem(allowedTargets, *server->getRNG());

		auto state = m->battle()->battleGetWallState(target);

		auto attackInfo = ca.attackedParts.begin();
		for ( ; attackInfo != ca.attackedParts.end(); ++attackInfo)
			if ( attackInfo->attackedPart == target )
				break;

		if (attackInfo == ca.attackedParts.end()) // new part
		{
			CatapultAttack::AttackInfo newInfo;
			newInfo.damageDealt = 1;
			newInfo.attackedPart = target;
			newInfo.destinationTile = m->battle()->wallPartToBattleHex(target);
			ca.attackedParts.push_back(newInfo);
			attackInfo = ca.attackedParts.end() - 1;
		}
		else // already damaged before, update damage
		{
			attackInfo->damageDealt += 1;
		}

		//removing creatures in turrets / keep if one is destroyed
		BattleHex posRemove;

		switch(target)
		{
		case EWallPart::KEEP:
			posRemove = BattleHex::CASTLE_CENTRAL_TOWER;
			break;
		case EWallPart::BOTTOM_TOWER:
			posRemove = BattleHex::CASTLE_BOTTOM_TOWER;
			break;
		case EWallPart::UPPER_TOWER:
			posRemove = BattleHex::CASTLE_UPPER_TOWER;
			break;
		}

		if(posRemove != BattleHex::INVALID && state - attackInfo->damageDealt <= 0) //HP enum subtraction not intuitive, consider using SiegeInfo::applyDamage
		{
			auto all = m->battle()->battleGetUnitsIf([=](const battle::Unit * unit)
			{
				return !unit->isGhost();
			});

			for(auto & elem : all)
			{
				if(elem->getPosition() != posRemove)
					continue;

				// if tower was hit multiple times, it may have been destroyed already
				bool stackWasRemovedBefore = false;
				for(auto & removed : removeUnits.changedStacks)
				{
					if (removed.id == elem->unitId())
						stackWasRemovedBefore = true;
				}

				if (!stackWasRemovedBefore)
					removeUnits.changedStacks.emplace_back(elem->unitId(), UnitChanges::EOperation::REMOVE);
				break;
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
