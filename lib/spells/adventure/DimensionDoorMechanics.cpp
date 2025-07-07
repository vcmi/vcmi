/*
 * DimensionDoorMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "DimensionDoorMechanics.h"

#include "../CSpellHandler.h"

#include "../../IGameSettings.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/TerrainTile.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

DimensionDoorMechanics::DimensionDoorMechanics(const CSpell * s)
	: AdventureSpellMechanics(s)
{
}

bool DimensionDoorMechanics::canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!caster->getHeroCaster())
		return false;

	if(caster->getHeroCaster()->movementPointsRemaining() <= 0) //unlike town portal non-zero MP is enough
	{
		problem.add(MetaString::createFromTextID("core.genrltxt.125"));
		return false;
	}

	const auto schoolLevel = caster->getSpellSchoolLevel(owner);

	std::stringstream cachingStr;
	cachingStr << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT) << "id_" << owner->id.num;

	int castsAlreadyPerformedThisTurn = caster->getHeroCaster()->getBonuses(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(owner->id)), cachingStr.str())->size();
	int castsLimit = owner->getLevelPower(schoolLevel);

	bool isTournamentRulesLimitEnabled = cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_TOURNAMENT_RULES_LIMIT);
	if(isTournamentRulesLimitEnabled)
	{
		int3 mapSize = cb->getMapSize();

		bool meetsTournamentRulesTwoCastsRequirements =  mapSize.x * mapSize.y * mapSize.z >= GameConstants::TOURNAMENT_RULES_DD_MAP_TILES_THRESHOLD && schoolLevel == MasteryLevel::EXPERT;

		castsLimit = meetsTournamentRulesTwoCastsRequirements ? 2 : 1;
	}

	if(castsAlreadyPerformedThisTurn >= castsLimit) //limit casts per turn
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.338");
		caster->getCasterName(message);
		problem.add(std::move(message));
		return false;
	}

	return true;
}

bool DimensionDoorMechanics::canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if(cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_ONLY_TO_UNCOVERED_TILES))
	{
		if(!cb->isVisibleFor(pos, caster->getCasterOwner()))
			return false;
	}

	int3 casterPosition = caster->getHeroCaster()->getSightCenter();

	const TerrainTile * dest = cb->getTileUnchecked(pos);
	const TerrainTile * curr = cb->getTileUnchecked(casterPosition);

	if(!dest)
		return false;

	if(!curr)
		return false;

	if(!isInScreenRange(casterPosition, pos))
		return false;

	if(cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_EXPOSES_TERRAIN_TYPE))
	{
		if(!dest->isClear(curr))
			return false;
	}
	else
	{
		if(dest->blocked())
			return false;
	}

	return true;
}

ESpellCastResult DimensionDoorMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	const int baseCost = env->getCb()->getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
	const int movementCost = baseCost * ((schoolLevel >= 3) ? 2 : 3);

	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(!dest->isClear(curr))
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();

		// tile is either blocked or not possible to move (e.g. water <-> land)
		if(env->getCb()->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_FAILURE_SPENDS_POINTS))
		{
			// SOD: DD to such "wrong" terrain results in mana and move points spending, but fails to move hero
			iw.text = MetaString::createFromTextID("core.genrltxt.70"); // Dimension Door failed!
			env->apply(iw);
			// no return - resources will be spent
		}
		else
		{
			// HotA: game will show error message without taking mana or move points, even when DD into terra incognita
			iw.text = MetaString::createFromTextID("vcmi.dimensionDoor.seaToLandError");
			env->apply(iw);
			return ESpellCastResult::CANCEL;
		}
	}

	GiveBonus gb;
	gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
	gb.bonus = Bonus(BonusDuration::ONE_DAY, BonusType::NONE, BonusSource::SPELL_EFFECT, 0, BonusSourceID(owner->id));
	env->apply(gb);

	SetMovePoints smp;
	smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
	if(movementCost < static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()))
		smp.val = parameters.caster->getHeroCaster()->movementPointsRemaining() - movementCost;
	else
		smp.val = 0;
	env->apply(smp);

	return ESpellCastResult::OK;
}

void DimensionDoorMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(dest->isClear(curr))
		env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(parameters.pos), EMovementMode::DIMENSION_DOOR);
}

VCMI_LIB_NAMESPACE_END
