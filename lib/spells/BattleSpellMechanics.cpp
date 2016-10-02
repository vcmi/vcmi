/*
 * BattleSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleSpellMechanics.h"

#include "../NetPacks.h"
#include "../BattleState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"

///HealingSpellMechanics
void HealingSpellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	EHealLevel healLevel = getHealLevel(parameters.effectLevel);
	int hpGained = calculateHealedHP(env, parameters, ctx);
	StacksHealedOrResurrected shr;
	shr.lifeDrain = false;
	shr.tentHealing = false;
	//special case for Archangel
	shr.cure = parameters.mode == ECastingMode::CREATURE_ACTIVE_CASTING && owner->id == SpellID::RESURRECTION;

	const bool resurrect = (healLevel != EHealLevel::HEAL);
	for(auto & attackedCre : ctx.attackedCres)
	{
		StacksHealedOrResurrected::HealInfo hi;
		hi.stackID = (attackedCre)->ID;
		int stackHPgained = parameters.caster->getSpellBonus(owner, hpGained, attackedCre);
		hi.healedHP = attackedCre->calculateHealedHealthPoints(stackHPgained, resurrect);
		hi.lowLevelResurrection = (healLevel == EHealLevel::RESURRECT);
		shr.healedStacks.push_back(hi);
	}
	if(!shr.healedStacks.empty())
		env->sendAndApply(&shr);
}

int HealingSpellMechanics::calculateHealedHP(const SpellCastEnvironment* env, const BattleSpellCastParameters& parameters, SpellCastContext& ctx) const
{
	return parameters.getEffectValue();
}

///AntimagicMechanics
void AntimagicMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);

	doDispell(battle, packet, [this](const Bonus *b) -> bool
	{
		if(b->source == Bonus::SPELL_EFFECT)
		{
			const CSpell * sourceSpell = SpellID(b->sid).toSpell();
			if(!sourceSpell)
				return false;//error
			//keep positive effects
			if(sourceSpell->isPositive())
				return false;
			//keep own effects
			if(sourceSpell == owner)
				return false;
			//remove all others
			return true;
		}

		return false; //not a spell effect
	});
}

///ChainLightningMechanics
std::vector<const CStack *> ChainLightningMechanics::calculateAffectedStacks(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const
{
	std::vector<const CStack *> res;

	std::set<BattleHex> possibleHexes;
	for(auto stack : cb->battleGetAllStacks())
	{
		if(stack->isValidTarget())
		{
			for(auto hex : stack->getHexes())
			{
				possibleHexes.insert (hex);
			}
		}
	}
	int targetsOnLevel[4] = {4, 4, 5, 5};

	BattleHex lightningHex = ctx.destination;
	for(int i = 0; i < targetsOnLevel[ctx.schoolLvl]; ++i)
	{
		auto stack = cb->battleGetStackByPos(lightningHex, true);
		if(!stack)
			break;
		res.push_back(stack);
		for(auto hex : stack->getHexes())
		{
			possibleHexes.erase(hex); //can't hit same stack twice
		}
		if(possibleHexes.empty()) //not enough targets
			break;
		lightningHex = BattleHex::getClosestTile(stack->attackerOwned, lightningHex, possibleHexes);
	}

	return res;
}

///CloneMechanics
void CloneMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	const CStack * clonedStack = nullptr;
	if(ctx.attackedCres.size())
		clonedStack = *ctx.attackedCres.begin();
	if(!clonedStack)
	{
		env->complain ("No target stack to clone!");
		return;
	}
	const int attacker = !(bool)parameters.casterSide;

	BattleStackAdded bsa;
	bsa.creID = clonedStack->type->idNumber;
	bsa.attacker = attacker;
	bsa.summoned = true;
	bsa.pos = parameters.cb->getAvaliableHex(bsa.creID, attacker); //TODO: unify it
	bsa.amount = clonedStack->count;
	env->sendAndApply(&bsa);

	BattleSetStackProperty ssp;
	ssp.stackID = bsa.newStackID;//we know stack ID after apply
	ssp.which = BattleSetStackProperty::CLONED;
	ssp.val = 0;
	ssp.absolute = 1;
	env->sendAndApply(&ssp);

	ssp.stackID = clonedStack->ID;
	ssp.which = BattleSetStackProperty::HAS_CLONE;
	ssp.val = bsa.newStackID;
	ssp.absolute = 1;
	env->sendAndApply(&ssp);

	SetStackEffect sse;
	sse.stacks.push_back(bsa.newStackID);
	Bonus lifeTimeMarker(Bonus::N_TURNS, Bonus::NONE, Bonus::SPELL_EFFECT, 0, owner->id.num);
	lifeTimeMarker.turnsRemain = parameters.enchantPower;
	sse.effect.push_back(lifeTimeMarker);
	env->sendAndApply(&sse);
}

ESpellCastProblem::ESpellCastProblem CloneMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//can't clone already cloned creature
	if(vstd::contains(obj->state, EBattleStackState::CLONED))
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	if(obj->cloneID != -1)
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	ui8 schoolLevel;
	if(caster)
	{
		schoolLevel = caster->getEffectLevel(owner);
	}
	else
	{
		schoolLevel = 3;//todo: remove
	}

	if(schoolLevel < 3)
	{
		int maxLevel = (std::max(schoolLevel, (ui8)1) + 4);
		int creLevel = obj->getCreature()->level;
		if(maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, any level for expert
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	//use default algorithm only if there is no mechanics-related problem
	return DefaultSpellMechanics::isImmuneByStack(caster, obj);
}

///CureMechanics
void CureMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);

	doDispell(battle, packet, dispellSelector);
}

HealingSpellMechanics::EHealLevel CureMechanics::getHealLevel(int effectLevel) const
{
	return EHealLevel::HEAL;
}

bool CureMechanics::dispellSelector(const Bonus * b)
{
	if(b->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sp = SpellID(b->sid).toSpell();
		return sp && sp->isNegative();
	}
	return false; //not a spell effect
}

ESpellCastProblem::ESpellCastProblem CureMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//Selector method name is ok as cashing string. --AVS
	if(!obj->canBeHealed() && !canDispell(obj, dispellSelector, "CureMechanics::dispellSelector"))
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;

	return DefaultSpellMechanics::isImmuneByStack(caster, obj);
}

///DispellMechanics
void DispellMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);
	doDispell(battle, packet, Selector::all);
}

ESpellCastProblem::ESpellCastProblem DispellMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//just in case
	if(!obj->alive())
		return ESpellCastProblem::WRONG_SPELL_TARGET;

	//DISPELL ignores all immunities, except specific absolute immunity
	{
		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << owner->id.toEnum() << "addInfo_1";
		if(obj->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, owner->id.toEnum(), 1), cachingStr.str()))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	if(canDispell(obj, Selector::all, "DefaultSpellMechanics::dispellSelector"))
		return ESpellCastProblem::OK;
	else
		return ESpellCastProblem::WRONG_SPELL_TARGET;
	//any other immunities are ignored - do not execute default algorithm
}

void DispellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(parameters.spellLvl > 2)
	{
		//expert DISPELL also removes spell-created obstacles
		ObstaclesRemoved packet;

		for(const auto obstacle : parameters.cb->obstacles)
		{
			if(obstacle->obstacleType == CObstacleInstance::FIRE_WALL
				|| obstacle->obstacleType == CObstacleInstance::FORCE_FIELD
				|| obstacle->obstacleType == CObstacleInstance::LAND_MINE)
				packet.obstacles.insert(obstacle->uniqueID);
		}

		if(!packet.obstacles.empty())
			env->sendAndApply(&packet);
	}
}

///EarthquakeMechanics
void EarthquakeMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(nullptr == parameters.cb->battleGetDefendedTown())
	{
		env->complain("EarthquakeMechanics: not town siege");
		return;
	}

	if(CGTownInstance::NONE == parameters.cb->battleGetDefendedTown()->fortLevel())
	{
		env->complain("EarthquakeMechanics: town has no fort");
		return;
	}

	//start with all destructible parts
	std::set<EWallPart::EWallPart> possibleTargets =
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

	const int targetsToAttack = 2 + std::max<int>(parameters.spellLvl - 1, 0);

	CatapultAttack ca;
	ca.attacker = -1;

	for(int i = 0; i < targetsToAttack; i++)
	{
		//Any destructible part can be hit regardless of its HP. Multiple hit on same target is allowed.
		EWallPart::EWallPart target = *RandomGeneratorUtil::nextItem(possibleTargets, env->getRandomGenerator());

		auto & currentHP = parameters.cb->si.wallState;

		if(currentHP.at(target) == EWallState::DESTROYED || currentHP.at(target) == EWallState::NONE)
			continue;

		CatapultAttack::AttackInfo attackInfo;

		attackInfo.damageDealt = 1;
		attackInfo.attackedPart = target;
		attackInfo.destinationTile = parameters.cb->wallPartToBattleHex(target);

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

		if(posRemove != BattleHex::INVALID)
		{
			BattleStacksRemoved bsr;
			for(auto & elem : parameters.cb->stacks)
			{
				if(elem->position == posRemove)
				{
					bsr.stackIDs.insert(elem->ID);
					break;
				}
			}
			if(bsr.stackIDs.size() > 0)
				env->sendAndApply(&bsr);
		}
	};

	env->sendAndApply(&ca);
}

ESpellCastProblem::ESpellCastProblem EarthquakeMechanics::canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, mode); //should not even try to do it
		return ESpellCastProblem::INVALID;
	}

	if(nullptr == cb->battleGetDefendedTown())
	{
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	if(CGTownInstance::NONE == cb->battleGetDefendedTown()->fortLevel())
	{
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	CSpell::TargetInfo ti(owner, caster->getSpellSchoolLevel(owner));
	if(ti.smart)
	{
		//if spell targeting is smart, then only attacker can use it
		if(cb->playerToSide(caster->getOwner()) != 0)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	const auto attackableBattleHexes = cb->getAttackableBattleHexes();

	if(attackableBattleHexes.empty())
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;

	return ESpellCastProblem::OK;
}

bool EarthquakeMechanics::requiresCreatureTarget() const
{
	return false;
}

///HypnotizeMechanics
ESpellCastProblem::ESpellCastProblem HypnotizeMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//todo: maybe do not resist on passive cast
	if(nullptr != caster)
	{
		//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
		ui64 subjectHealth = (obj->count - 1) * obj->MaxHealth() + obj->firstHPleft;
		//apply 'damage' bonus for hypnotize, including hero specialty
		ui64 maxHealth = caster->getSpellBonus(owner, owner->calculateRawEffectValue(caster->getEffectLevel(owner), caster->getEffectPower(owner)), obj);
		if (subjectHealth > maxHealth)
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	return DefaultSpellMechanics::isImmuneByStack(caster, obj);
}

///ObstacleMechanics
ESpellCastProblem::ESpellCastProblem ObstacleMechanics::canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const
{
	ui8 side = cb->playerToSide(ctx.caster->getOwner());

	bool hexesOutsideBattlefield = false;

	auto tilesThatMustBeClear = owner->rangeInHexes(ctx.destination, ctx.schoolLvl, side, &hexesOutsideBattlefield);

	for(const BattleHex & hex : tilesThatMustBeClear)
		if(!isHexAviable(cb, hex, ctx.ti.clearAffected))
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;

	if(hexesOutsideBattlefield)
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;

	return ESpellCastProblem::OK;
}

bool ObstacleMechanics::isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear)
{
	if(!hex.isAvailable())
		return false;

	if(!mustBeClear)
		return true;

	if(cb->battleGetStackByPos(hex, true))
		return false;

	auto obst = cb->battleGetObstacleOnPos(hex, false);
	if(obst /*&& obst->type != CObstacleInstance::MOAT*/)//todo: issue 2366, uncomment once obstacle tile sharing implemented
		return false;

	if(nullptr != cb->battleGetDefendedTown() && CGTownInstance::NONE != cb->battleGetDefendedTown()->fortLevel())
	{
		EWallPart::EWallPart part = cb->battleHexToWallPart(hex);

		if(part == EWallPart::INVALID)
			return true;//no fortification here
		else if(static_cast<int>(part) < 0)
			return false;//indestuctible part (cant be checked by battleGetWallState)
		else if(part == EWallPart::BOTTOM_TOWER || part == EWallPart::UPPER_TOWER)
			return false;//destructible, but should not be available
		else if(cb->battleGetWallState(part) != EWallState::DESTROYED && cb->battleGetWallState(part) != EWallState::NONE)
			return false;
	}

	return true;
}

void ObstacleMechanics::placeObstacle(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, const BattleHex & pos) const
{
	const int obstacleIdToGive = parameters.cb->obstacles.size()+1;

	auto obstacle = std::make_shared<SpellCreatedObstacle>();
	setupObstacle(obstacle.get());

	obstacle->pos = pos;
	obstacle->casterSide = parameters.casterSide;
	obstacle->ID = owner->id;
	obstacle->spellLevel = parameters.effectLevel;
	obstacle->casterSpellPower = parameters.effectPower;
	obstacle->uniqueID = obstacleIdToGive;

	BattleObstaclePlaced bop;
	bop.obstacle = obstacle;
	env->sendAndApply(&bop);
}

///PatchObstacleMechanics
void PatchObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	std::vector<BattleHex> availableTiles;
	for(int i = 0; i < GameConstants::BFIELD_SIZE; i += 1)
	{
		BattleHex hex = i;
		if(isHexAviable(parameters.cb, hex, true))
			availableTiles.push_back(hex);
	}
	RandomGeneratorUtil::randomShuffle(availableTiles, env->getRandomGenerator());
	const int patchesForSkill[] = {4, 4, 6, 8};
	const int patchesToPut = std::min<int>(patchesForSkill[parameters.spellLvl], availableTiles.size());

	//land mines or quicksand patches are handled as spell created obstacles
	for (int i = 0; i < patchesToPut; i++)
		placeObstacle(env, parameters, availableTiles.at(i));
}

ESpellCastProblem::ESpellCastProblem PatchObstacleMechanics::canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	//Quicksand|LandMine are useless if enemy has native stack, check for LandMine damage immunity is done in general way by CSpell
	const ui8 otherSide = !cb->playerToSide(caster->getOwner());

	if(cb->battleHasNativeStack(otherSide))
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;

	return DefaultSpellMechanics::canBeCast(cb, mode, caster);
}

///LandMineMechanics
bool LandMineMechanics::requiresCreatureTarget() const
{
	return true;
}

void LandMineMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::LAND_MINE;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///QuicksandMechanics
bool QuicksandMechanics::requiresCreatureTarget() const
{
	return false;
}

void QuicksandMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::QUICKSAND;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///WallMechanics
std::vector<BattleHex> WallMechanics::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes) const
{
	std::vector<BattleHex> ret;

	//Special case - shape of obstacle depends on caster's side
	//TODO make it possible through spell config

	BattleHex::EDir firstStep, secondStep;
	if(side)
	{
		firstStep = BattleHex::TOP_LEFT;
		secondStep = BattleHex::TOP_RIGHT;
	}
	else
	{
		firstStep = BattleHex::TOP_RIGHT;
		secondStep = BattleHex::TOP_LEFT;
	}

	//Adds hex to the ret if it's valid. Otherwise sets output arg flag if given.
	auto addIfValid = [&](BattleHex hex)
	{
		if(hex.isValid())
			ret.push_back(hex);
		else if(outDroppedHexes)
			*outDroppedHexes = true;
	};

	ret.push_back(centralHex);
	addIfValid(centralHex.moveInDir(firstStep, false));
	if(schoolLvl >= 2) //advanced versions of fire wall / force field cotnains of 3 hexes
		addIfValid(centralHex.moveInDir(secondStep, false)); //moveInDir function modifies subject hex

	return ret;
}

///FireWallMechanics
bool FireWallMechanics::requiresCreatureTarget() const
{
	return true;
}

void FireWallMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FIRE_WALL");
		return;
	}
	//firewall is build from multiple obstacles - one fire piece for each affected hex
	auto affectedHexes = owner->rangeInHexes(destination, parameters.spellLvl, parameters.casterSide);
	for(BattleHex hex : affectedHexes)
		placeObstacle(env, parameters, hex);
}

void FireWallMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FIRE_WALL;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
}

///ForceFieldMechanics
bool ForceFieldMechanics::requiresCreatureTarget() const
{
	return false;
}

void ForceFieldMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FORCE_FIELD");
		return;
	}
	placeObstacle(env, parameters, destination);
}

void ForceFieldMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FORCE_FIELD;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
}

///RemoveObstacleMechanics
void RemoveObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(auto obstacleToRemove = parameters.cb->battleGetObstacleOnPos(parameters.getFirstDestinationHex(), false))
	{
		if(canRemove(obstacleToRemove.get(), parameters.spellLvl))
		{
			ObstaclesRemoved obr;
			obr.obstacles.insert(obstacleToRemove->uniqueID);
			env->sendAndApply(&obr);
		}
		else
		{
			env->complain("Cant remove this obstacle!");
		}
	}
	else
		env->complain("There's no obstacle to remove!");
}

ESpellCastProblem::ESpellCastProblem RemoveObstacleMechanics::canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, mode); //should not even try to do it
		return ESpellCastProblem::INVALID;
	}

	const int spellLevel = caster->getSpellSchoolLevel(owner);

	for(auto obstacle : cb->battleGetAllObstacles())
		if(canRemove(obstacle.get(), spellLevel))
			return ESpellCastProblem::OK;

	return ESpellCastProblem::NO_APPROPRIATE_TARGET;
}

ESpellCastProblem::ESpellCastProblem RemoveObstacleMechanics::canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const
{
	if(auto obstacle = cb->battleGetObstacleOnPos(ctx.destination, false))
		if(canRemove(obstacle.get(), ctx.schoolLvl))
			return ESpellCastProblem::OK;

	return ESpellCastProblem::NO_APPROPRIATE_TARGET;
}

bool RemoveObstacleMechanics::canRemove(const CObstacleInstance * obstacle, const int spellLevel) const
{
	switch (obstacle->obstacleType)
	{
	case CObstacleInstance::ABSOLUTE_OBSTACLE: //cliff-like obstacles can't be removed
	case CObstacleInstance::MOAT:
		return false;
	case CObstacleInstance::USUAL:
		return true;
	case CObstacleInstance::FIRE_WALL:
		if(spellLevel >= 2)
			return true;
		break;
	case CObstacleInstance::QUICKSAND:
	case CObstacleInstance::LAND_MINE:
	case CObstacleInstance::FORCE_FIELD:
		if(spellLevel >= 3)
			return true;
		break;
	default:
		break;
	}
	return false;
}

bool RemoveObstacleMechanics::requiresCreatureTarget() const
{
	return false;
}

///RisingSpellMechanics
HealingSpellMechanics::EHealLevel RisingSpellMechanics::getHealLevel(int effectLevel) const
{
	//this may be even distinct class
	if((effectLevel <= 1) && (owner->id == SpellID::RESURRECTION))
		return EHealLevel::RESURRECT;

	return EHealLevel::TRUE_RESURRECT;
}

///SacrificeMechanics
ESpellCastProblem::ESpellCastProblem SacrificeMechanics::canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, mode); //should not even try to do it
		return ESpellCastProblem::INVALID;
	}

	// for sacrifice we have to check for 2 targets (one dead to resurrect and one living to destroy)

	bool targetExists = false;
	bool targetToSacrificeExists = false;

	for(const CStack * stack : cb->battleGetAllStacks())
	{
		//using isImmuneBy directly as this mechanics does not have overridden immunity check
		//therefore we do not need to check caster and casting mode
		//TODO: check that we really should check immunity for both stacks
		ESpellCastProblem::ESpellCastProblem res = owner->internalIsImmune(caster, stack);
		const bool immune =  ESpellCastProblem::OK != res && ESpellCastProblem::NOT_DECIDED != res;
		const bool casterStack = stack->owner == caster->getOwner();

		if(!immune && casterStack)
		{
			if(stack->alive())
				targetToSacrificeExists = true;
			else
				targetExists = true;
			if(targetExists && targetToSacrificeExists)
				break;
		}
	}

	if(targetExists && targetToSacrificeExists)
		return ESpellCastProblem::OK;
	else
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
}


void SacrificeMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	const CStack * victim = nullptr;
	if(parameters.destinations.size() == 2)
	{
		victim = parameters.destinations[1].stackValue;
	}

	if(nullptr == victim)
	{
		env->complain("SacrificeMechanics: No stack to sacrifice");
		return;
	}
	//resurrect target after basic checks
	RisingSpellMechanics::applyBattleEffects(env, parameters, ctx);
	//it is safe to remove even active stack
	BattleStacksRemoved bsr;
	bsr.stackIDs.insert(victim->ID);
	env->sendAndApply(&bsr);
}

int SacrificeMechanics::calculateHealedHP(const SpellCastEnvironment* env, const BattleSpellCastParameters& parameters, SpellCastContext& ctx) const
{
	const CStack * victim = nullptr;

	if(parameters.destinations.size() == 2)
	{
		victim = parameters.destinations[1].stackValue;
	}

	if(nullptr == victim)
	{
		env->complain("SacrificeMechanics: No stack to sacrifice");
		return 0;
	}

	return (parameters.effectPower + victim->MaxHealth() + owner->getPower(parameters.effectLevel)) * victim->count;
}

bool SacrificeMechanics::requiresCreatureTarget() const
{
	return false;//canBeCast do all target existence checks
}

///SpecialRisingSpellMechanics
ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::canBeCast(const CBattleInfoCallback * cb, const SpellTargetingContext & ctx) const
{
	//find alive possible target
	const CStack * stackToHeal = cb->getStackIf([ctx, this](const CStack * s)
	{
		const bool ownerMatches = !ctx.ti.smart || s->owner == ctx.caster->getOwner();

		return ownerMatches && s->isValidTarget(false) && s->coversPos(ctx.destination) && ESpellCastProblem::OK == owner->isImmuneByStack(ctx.caster, s);
	});

	if(nullptr == stackToHeal)
	{
		//find dead possible target if there is no alive target
		stackToHeal = cb->getStackIf([ctx, this](const CStack * s)
		{
			const bool ownerMatches = !ctx.ti.smart || s->owner == ctx.caster->getOwner();

			return ownerMatches && s->isValidTarget(true) && s->coversPos(ctx.destination) && ESpellCastProblem::OK == owner->isImmuneByStack(ctx.caster, s);
		});

		//we have found dead target
		if(nullptr != stackToHeal)
		{
			for(const BattleHex & hex : stackToHeal->getHexes())
			{
				const CStack * other = cb->getStackIf([hex, stackToHeal](const CStack * s)
				{
					return s->isValidTarget(false) && s->coversPos(hex) && s != stackToHeal;
				});
				if(nullptr != other)
					return ESpellCastProblem::NO_APPROPRIATE_TARGET;//alive stack blocks resurrection
			}
		}
	}

	if(nullptr == stackToHeal)
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;

	return ESpellCastProblem::OK;
}

ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	// following does apply to resurrect and animate dead(?) only
	// for sacrifice health calculation and health limit check don't matter

	if(obj->count >= obj->baseAmount)
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;

	//FIXME: Archangels can cast immune stack and this should be applied for them and not hero
//	if(caster)
//	{
//		auto maxHealth = calculateHealedHP(caster, obj, nullptr);
//		if (maxHealth < obj->MaxHealth()) //must be able to rise at least one full creature
//			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
//	}

	return DefaultSpellMechanics::isImmuneByStack(caster,obj);
}

///SummonMechanics
ESpellCastProblem::ESpellCastProblem SummonMechanics::canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, mode); //should not even try to do it
		return ESpellCastProblem::INVALID;
	}

	//check if there are summoned elementals of other type

	auto otherSummoned = cb->battleGetStacksIf([caster, this](const CStack * st)
	{
		return (st->owner == caster->getOwner())
			&& (vstd::contains(st->state, EBattleStackState::SUMMONED))
			&& (!vstd::contains(st->state, EBattleStackState::CLONED))
			&& (st->getCreature()->idNumber != creatureToSummon);
	});

	if(!otherSummoned.empty())
		return ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED;

	return ESpellCastProblem::OK;
}

void SummonMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	BattleStackAdded bsa;
	bsa.creID = creatureToSummon;
	bsa.attacker = !(bool)parameters.casterSide;
	bsa.summoned = true;
	bsa.pos = parameters.cb->getAvaliableHex(creatureToSummon, !(bool)parameters.casterSide); //TODO: unify it

	//TODO stack casting -> probably power will be zero; set the proper number of creatures manually
	int percentBonus = parameters.casterHero ? parameters.casterHero->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, owner->id.toEnum()) : 0;

	bsa.amount = parameters.effectPower
		* owner->getPower(parameters.spellLvl)
		* (100 + percentBonus) / 100.0; //new feature - percentage bonus
	if(bsa.amount)
		env->sendAndApply(&bsa);
	else
		env->complain("Summoning didn't summon any!");
}

bool SummonMechanics::requiresCreatureTarget() const
{
	return false;
}

///TeleportMechanics
void TeleportMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(parameters.destinations.size() == 2)
	{
		//first destination hex to move to
		const BattleHex destination = parameters.destinations[0].hexValue;
		if(!destination.isValid())
		{
			env->complain("TeleportMechanics: invalid teleport destination");
			return;
		}

		//second destination creature to move
		const CStack * target = parameters.destinations[1].stackValue;
		if(nullptr == target)
		{
			env->complain("TeleportMechanics: no stack to teleport");
			return;
		}

		if(!parameters.cb->battleCanTeleportTo(target, destination, parameters.effectLevel))
		{
			env->complain("TeleportMechanics: forbidden teleport");
			return;
		}

		BattleStackMoved bsm;
		bsm.distance = -1;
		bsm.stack = target->ID;
		std::vector<BattleHex> tiles;
		tiles.push_back(destination);
		bsm.tilesToMove = tiles;
		bsm.teleporting = true;
		env->sendAndApply(&bsm);
	}
	else
	{
		env->complain("TeleportMechanics: 2 destinations required.");
			return;
	}
}

ESpellCastProblem::ESpellCastProblem TeleportMechanics::canBeCast(const CBattleInfoCallback* cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const
{
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, mode); //should not even try to do it
		return ESpellCastProblem::INVALID;
	}

	return DefaultSpellMechanics::canBeCast(cb, mode, caster);
}
