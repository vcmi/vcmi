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

bool Catapult::applicable(Problem & problem, const Mechanics * m) const
{
	const auto *town = m->battle()->battleGetDefendedTown();

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

void Catapult::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & eTarget) const
{
	if(m->isMassive())
		applyMassive(server, m); // Like earthquake
	else
		applyTargeted(server, m, eTarget); // Like catapult shots
}


void Catapult::applyMassive(ServerCallback * server, const Mechanics * m) const
{
	//start with all destructible parts
	std::vector<EWallPart> allowedTargets = getPotentialTargets(m, true, true);

	assert(!allowedTargets.empty());
	if (allowedTargets.empty())
		return;

	CatapultAttack ca;
	ca.attacker = m->caster->getCasterUnitId();

	for(int i = 0; i < targetsToAttack; i++)
	{
		// Hit on any existing, not destroyed targets are allowed
		// Multiple hit on same target are allowed.
		// Potential overshots (more hits on same targets than remaining HP) are allowed
		EWallPart target = *RandomGeneratorUtil::nextItem(allowedTargets, *server->getRNG());

		auto attackInfo = ca.attackedParts.begin();
		for ( ; attackInfo != ca.attackedParts.end(); ++attackInfo)
			if ( attackInfo->attackedPart == target )
				break;

		if (attackInfo == ca.attackedParts.end()) // new part
		{
			CatapultAttack::AttackInfo newInfo;
			newInfo.damageDealt = getRandomDamage(server);
			newInfo.attackedPart = target;
			newInfo.destinationTile = m->battle()->wallPartToBattleHex(target);
			ca.attackedParts.push_back(newInfo);
			attackInfo = ca.attackedParts.end() - 1;
		}
		else // already damaged before, update damage
		{
			attackInfo->damageDealt += getRandomDamage(server);
		}
	}
	server->apply(&ca);

	removeTowerShooters(server, m);
}

void Catapult::applyTargeted(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	assert(!target.empty());
	auto destination = target.at(0).hexValue;
	auto desiredTarget = m->battle()->battleHexToWallPart(destination);

	auto nearestAttackableWall = [&m,&server](EWallPart part) {
		auto choosePart = [&] (EWallPart left, EWallPart right, int max = 2) {
			bool bothSidesAttackable = m->battle()->isWallPartAttackable(left)
										&& m->battle()->isWallPartAttackable(right);
			if(bothSidesAttackable)
				return server->getRNG()->getInt64Range(0, max)() < max ? left : right;
			else if(m->battle()->isWallPartAttackable(left))
				return left;
			else if(m->battle()->isWallPartAttackable(right))
				return right;
			return EWallPart::INVALID;
		};
		switch (part)
		{
			case EWallPart::BOTTOM_TOWER:
				return choosePart(EWallPart::BOTTOM_WALL, EWallPart::BELOW_GATE);
			case EWallPart::BOTTOM_WALL:
				return choosePart(EWallPart::BELOW_GATE, EWallPart::OVER_GATE);
			case EWallPart::BELOW_GATE:
				return choosePart(EWallPart::BOTTOM_WALL, EWallPart::OVER_GATE);
			case EWallPart::OVER_GATE:
				return choosePart(EWallPart::UPPER_WALL, EWallPart::BELOW_GATE);
			case EWallPart::UPPER_WALL:
				return choosePart(EWallPart::OVER_GATE, EWallPart::BELOW_GATE);
			case EWallPart::UPPER_TOWER:
				return choosePart(EWallPart::UPPER_WALL, EWallPart::OVER_GATE);
			case EWallPart::GATE:
				return choosePart(EWallPart::OVER_GATE, EWallPart::BELOW_GATE, 1);
			default:
				return EWallPart::INVALID;
		}
	};

	for(int i = 0; i < targetsToAttack; i++)
	{
		auto actualTarget = EWallPart::INVALID;

		if ( m->battle()->isWallPartAttackable(desiredTarget) &&
				server->getRNG()->getInt64Range(0, 99)() < getCatapultHitChance(desiredTarget))
		{
			actualTarget = desiredTarget;
		}
		else if(nearestAttackableWall(desiredTarget) != EWallPart::INVALID)
		{
			actualTarget = nearestAttackableWall(desiredTarget);
		}
		else
		{
			std::vector<EWallPart> potentialTargets = getPotentialTargets(m, false, false);

			if (potentialTargets.empty())
				break; // everything is gone, can't attack anymore

			actualTarget = *RandomGeneratorUtil::nextItem(potentialTargets, *server->getRNG());
		}
		assert(actualTarget != EWallPart::INVALID);

		CatapultAttack::AttackInfo attack;
		attack.attackedPart = actualTarget;
		attack.destinationTile = m->battle()->wallPartToBattleHex(actualTarget);
		attack.damageDealt = getRandomDamage(server);

		CatapultAttack ca; //package for clients
		ca.attacker = m->caster->getCasterUnitId();
		ca.attackedParts.push_back(attack);
		server->apply(&ca);
		removeTowerShooters(server, m);
	}
}

int Catapult::getCatapultHitChance(EWallPart part) const
{
	switch(part)
	{
	case EWallPart::GATE:
		return gate;
	case EWallPart::KEEP:
		return keep;
	case EWallPart::BOTTOM_TOWER:
	case EWallPart::UPPER_TOWER:
		return tower;
	case EWallPart::BOTTOM_WALL:
	case EWallPart::BELOW_GATE:
	case EWallPart::OVER_GATE:
	case EWallPart::UPPER_WALL:
		return wall;
	default:
		return 0;
	}
}

int Catapult::getRandomDamage (ServerCallback * server) const
{
	std::array<int, 3> damageChances = { noDmg, hit, crit }; //dmgChance[i] - chance for doing i dmg when hit is successful
	int totalChance = std::accumulate(damageChances.begin(), damageChances.end(), 0);
	int damageRandom = server->getRNG()->getInt64Range(0, totalChance - 1)();
	int dealtDamage = 0;

	//calculating dealt damage
	for (int damage = 0; damage < damageChances.size(); ++damage)
	{
		if (damageRandom <= damageChances[damage])
		{
			dealtDamage = damage;
			break;
		}
		damageRandom -= damageChances[damage];
	}
	return dealtDamage;
}

void Catapult::removeTowerShooters(ServerCallback * server, const Mechanics * m) const
{
	BattleUnitsChanged removeUnits;

	for (auto const wallPart : { EWallPart::KEEP, EWallPart::BOTTOM_TOWER, EWallPart::UPPER_TOWER })
	{
		//removing creatures in turrets / keep if one is destroyed
		BattleHex posRemove;
		auto state = m->battle()->battleGetWallState(wallPart);

		switch(wallPart)
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

		if(state == EWallState::DESTROYED) //HP enum subtraction not intuitive, consider using SiegeInfo::applyDamage
		{
			auto all = m->battle()->battleGetUnitsIf([=](const battle::Unit * unit)
			{
				return !unit->isGhost() && unit->getPosition() == posRemove;
			});

			assert(all.size() == 0 || all.size() == 1);
			for(auto & elem : all)
				removeUnits.changedStacks.emplace_back(elem->unitId(), UnitChanges::EOperation::REMOVE);
		}
	}

	if(!removeUnits.changedStacks.empty())
		server->apply(&removeUnits);
}

std::vector<EWallPart> Catapult::getPotentialTargets(const Mechanics * m, bool bypassGateCheck, bool bypassTowerCheck) const
{
	std::vector<EWallPart> potentialTargets;
	constexpr std::array<EWallPart, 4> walls = { EWallPart::BOTTOM_WALL, EWallPart::BELOW_GATE, EWallPart::OVER_GATE, EWallPart::UPPER_WALL };
	constexpr std::array<EWallPart, 3> towers= { EWallPart::BOTTOM_TOWER, EWallPart::KEEP, EWallPart::UPPER_TOWER };
	constexpr EWallPart gates = EWallPart::GATE;

	// in H3, catapult under automatic control will attack objects in following order:
	// walls, gates, towers
	for (auto & part : walls)
		if (m->battle()->isWallPartAttackable(part))
			potentialTargets.push_back(part);

	if ((potentialTargets.empty() || bypassGateCheck) && (m->battle()->isWallPartAttackable(gates)))
		potentialTargets.push_back(gates);

	if (potentialTargets.empty() || bypassTowerCheck)
		for (auto & part : towers)
			if (m->battle()->isWallPartAttackable(part))
				potentialTargets.push_back(part);

	return potentialTargets;
}

void Catapult::adjustHitChance()
{
	vstd::abetween(keep, 0, 100);
	vstd::abetween(tower, 0, 100);
	vstd::abetween(gate, 0, 100);
	vstd::abetween(wall, 0, 100);
	vstd::abetween(crit, 0, 100);
	vstd::abetween(hit, 0, 100 - crit);
	vstd::amin(noDmg, 100 - hit - crit);
}

void Catapult::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeInt("targetsToAttack", targetsToAttack);
	handler.serializeInt("chanceToHitKeep", keep);
	handler.serializeInt("chanceToHitGate", gate);
	handler.serializeInt("chanceToHitTower", tower);
	handler.serializeInt("chanceToHitWall", wall);
	handler.serializeInt("chanceToNormalHit", hit);
	handler.serializeInt("chanceToCrit", crit);
	adjustHitChance();
}


}
}

VCMI_LIB_NAMESPACE_END
