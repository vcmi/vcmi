/*
 * DimensionDoorEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "DimensionDoorEffect.h"

#include "../CSpellHandler.h"

#include "../../IGameSettings.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/TerrainTile.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

DimensionDoorEffect::DimensionDoorEffect(const CSpell * s, const JsonNode & config)
	: AdventureSpellRangedEffect(config)
	, cursor(config["cursor"].String())
	, cursorGuarded(config["cursorGuarded"].String())
	, movementPointsRequired(config["movementPointsRequired"].Integer())
	, movementPointsTaken(config["movementPointsTaken"].Integer())
	, waterLandFailureTakesPoints(config["waterLandFailureTakesPoints"].Bool())
	, exposeFow(config["exposeFow"].Bool())
{
}

std::string DimensionDoorEffect::getCursorForTarget(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_TRIGGERS_GUARDS))
		return cursor;

	if (!exposeFow && !cb->isVisibleFor(pos, caster->getCasterOwner()))
		return cursor;

	if (!cb->isTileGuardedUnchecked(pos))
		return cursor;

	return cursorGuarded;
}

bool DimensionDoorEffect::canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!caster->getHeroCaster())
		return false;

	if(caster->getHeroCaster()->movementPointsRemaining() <= movementPointsRequired)
	{
		problem.add(MetaString::createFromTextID("core.genrltxt.125"));
		return false;
	}

	return true;
}

bool DimensionDoorEffect::canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if (!isTargetInRange(cb, caster, pos))
		return false;

	int3 casterPosition = caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = cb->getTileUnchecked(pos);
	const TerrainTile * curr = cb->getTileUnchecked(casterPosition);

	if(!dest)
		return false;

	if(!curr)
		return false;

	if(exposeFow)
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

ESpellCastResult DimensionDoorEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(!dest->isClear(curr))
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();

		// tile is either blocked or not possible to move (e.g. water <-> land)
		if(waterLandFailureTakesPoints)
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

	SetMovePoints smp;
	smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
	if(movementPointsTaken < static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()))
		smp.val = parameters.caster->getHeroCaster()->movementPointsRemaining() - movementPointsTaken;
	else
		smp.val = 0;
	env->apply(smp);

	return ESpellCastResult::OK;
}

void DimensionDoorEffect::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(dest->isClear(curr))
		env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(parameters.pos), EMovementMode::DIMENSION_DOOR);
}

VCMI_LIB_NAMESPACE_END
