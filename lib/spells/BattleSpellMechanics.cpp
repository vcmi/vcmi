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

///AntimagicMechanics
void AntimagicMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);

	doDispell(battle, packet, [this](const Bonus * b) -> bool
	{
		if(b->source == Bonus::SPELL_EFFECT)
		{				
			return b->sid != owner->id; //effect from this spell
		}
		return false; //not a spell effect
	});	
}

///ChainLightningMechanics
std::set<const CStack *> ChainLightningMechanics::getAffectedStacks(SpellTargetingContext & ctx) const
{
	std::set<const CStack* > attackedCres;

	std::set<BattleHex> possibleHexes;
	for(auto stack : ctx.cb->battleGetAllStacks())
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
		auto stack = ctx.cb->battleGetStackByPos(lightningHex, true);
		if(!stack)
			break;
		attackedCres.insert (stack);
		for(auto hex : stack->getHexes())
		{
			possibleHexes.erase(hex); //can't hit same place twice
		}
		if(possibleHexes.empty()) //not enough targets
			break;
		lightningHex = BattleHex::getClosestTile(stack->attackerOwned, ctx.destination, possibleHexes);
	}

	return attackedCres;
}

///CloneMechanics
void CloneMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
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
}

ESpellCastProblem::ESpellCastProblem CloneMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	//can't clone already cloned creature
	if(vstd::contains(obj->state, EBattleStackState::CLONED))
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	//TODO: how about stacks casting Clone?
	//currently Clone casted by stack is assumed Expert level
	ui8 schoolLevel;
	if(caster)
	{
		schoolLevel = caster->getSpellSchoolLevel(owner);
	}
	else
	{
		schoolLevel = 3;
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

	for(auto stackID : packet->affectedCres)
	{
		if(vstd::contains(packet->resisted, stackID))
		{
			logGlobal->errorStream() << "Resistance to positive spell CURE";
			continue;
		}

		CStack *s = battle->getStack(stackID);
		s->popBonuses([&](const Bonus *b) -> bool
		{
			if(b->source == Bonus::SPELL_EFFECT)
			{
				CSpell * sp = SpellID(b->sid).toSpell();
				return sp->isNegative();
			}
			return false; //not a spell effect
		});
	}
}

///DispellMechanics
void DispellMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	DefaultSpellMechanics::applyBattle(battle, packet);
	doDispell(battle, packet, Selector::sourceType(Bonus::SPELL_EFFECT));
}

ESpellCastProblem::ESpellCastProblem DispellMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	//DISPELL ignores all immunities, so do not call default
	std::stringstream cachingStr;
	cachingStr << "source_" << Bonus::SPELL_EFFECT;

	if(obj->hasBonus(Selector::sourceType(Bonus::SPELL_EFFECT), cachingStr.str()))
	{
		return ESpellCastProblem::OK;
	}

	return ESpellCastProblem::WRONG_SPELL_TARGET;
}

void DispellMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	DefaultSpellMechanics::applyBattleEffects(env, parameters, ctx);

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
void EarthquakeMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(nullptr == parameters.cb->town)
	{
		env->complain("EarthquakeMechanics: not town siege");
		return;
	}

	if(CGTownInstance::NONE == parameters.cb->town->fortLevel())
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

ESpellCastProblem::ESpellCastProblem EarthquakeMechanics::canBeCasted(const CBattleInfoCallback * cb, PlayerColor player) const
{
	if(nullptr == cb->battleGetDefendedTown())
	{
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	if(CGTownInstance::NONE == cb->battleGetDefendedTown()->fortLevel())
	{
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	if(owner->getTargetInfo(0).smart) //TODO: use real spell level
	{
		//if spell targeting is smart, then only attacker can use it
		if(cb->playerToSide(player) != 0)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	return ESpellCastProblem::OK;
}

///HypnotizeMechanics
ESpellCastProblem::ESpellCastProblem HypnotizeMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	if(nullptr != caster) //do not resist hypnotize casted after attack, for example
	{
		//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
		ui64 subjectHealth = (obj->count - 1) * obj->MaxHealth() + obj->firstHPleft;
		//apply 'damage' bonus for hypnotize, including hero specialty
		ui64 maxHealth = owner->calculateBonus(caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER)
			* owner->power + owner->getPower(caster->getSpellSchoolLevel(owner)), caster, obj);
		if (subjectHealth > maxHealth)
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	return DefaultSpellMechanics::isImmuneByStack(caster, obj);
}

///ObstacleMechanics
void ObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	auto placeObstacle = [&, this](BattleHex pos)
	{
		static int obstacleIdToGive =  parameters.cb->obstacles.size()
									? (parameters.cb->obstacles.back()->uniqueID+1)
									: 0;

		auto obstacle = make_shared<SpellCreatedObstacle>();
		switch(owner->id) // :/
		{
		case SpellID::QUICKSAND:
			obstacle->obstacleType = CObstacleInstance::QUICKSAND;
			obstacle->turnsRemaining = -1;
			obstacle->visibleForAnotherSide = false;
			break;
		case SpellID::LAND_MINE:
			obstacle->obstacleType = CObstacleInstance::LAND_MINE;
			obstacle->turnsRemaining = -1;
			obstacle->visibleForAnotherSide = false;
			break;
		case SpellID::FIRE_WALL:
			obstacle->obstacleType = CObstacleInstance::FIRE_WALL;
			obstacle->turnsRemaining = 2;
			obstacle->visibleForAnotherSide = true;
			break;
		case SpellID::FORCE_FIELD:
			obstacle->obstacleType = CObstacleInstance::FORCE_FIELD;
			obstacle->turnsRemaining = 2;
			obstacle->visibleForAnotherSide = true;
			break;
		default:
			//this function cannot be used with spells that do not create obstacles
			assert(0);
		}

		obstacle->pos = pos;
		obstacle->casterSide = parameters.casterSide;
		obstacle->ID = owner->id;
		obstacle->spellLevel = parameters.spellLvl;
		obstacle->casterSpellPower = parameters.usedSpellPower;
		obstacle->uniqueID = obstacleIdToGive++;

		BattleObstaclePlaced bop;
		bop.obstacle = obstacle;
		env->sendAndApply(&bop);
	};

	switch(owner->id)
	{
	case SpellID::QUICKSAND:
	case SpellID::LAND_MINE:
		{
			std::vector<BattleHex> availableTiles;
			for(int i = 0; i < GameConstants::BFIELD_SIZE; i += 1)
			{
				BattleHex hex = i;
				if(hex.getX() > 2 && hex.getX() < 14 && !(parameters.cb->battleGetStackByPos(hex, false)) && !(parameters.cb->battleGetObstacleOnPos(hex, false)))
					availableTiles.push_back(hex);
			}
			boost::range::random_shuffle(availableTiles);

			const int patchesForSkill[] = {4, 4, 6, 8};
			const int patchesToPut = std::min<int>(patchesForSkill[parameters.spellLvl], availableTiles.size());

			//land mines or quicksand patches are handled as spell created obstacles
			for (int i = 0; i < patchesToPut; i++)
				placeObstacle(availableTiles.at(i));
		}

		break;
	case SpellID::FORCE_FIELD:
		placeObstacle(parameters.destination);
		break;
	case SpellID::FIRE_WALL:
		{
			//fire wall is build from multiple obstacles - one fire piece for each affected hex
			auto affectedHexes = owner->rangeInHexes(parameters.destination, parameters.spellLvl, parameters.casterSide);
			for(BattleHex hex : affectedHexes)
				placeObstacle(hex);
		}
		break;
	default:
		assert(0);
	}
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

///RemoveObstacleMechanics
void RemoveObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	if(auto obstacleToRemove = parameters.cb->battleGetObstacleOnPos(parameters.destination, false))
	{
		ObstaclesRemoved obr;
		obr.obstacles.insert(obstacleToRemove->uniqueID);
		env->sendAndApply(&obr);
	}
	else
		env->complain("There's no obstacle to remove!");
}

///SpecialRisingSpellMechanics
ESpellCastProblem::ESpellCastProblem SacrificeMechanics::canBeCasted(const CBattleInfoCallback * cb, PlayerColor player) const
{
	// for sacrifice we have to check for 2 targets (one dead to resurrect and one living to destroy)

	bool targetExists = false;
	bool targetToSacrificeExists = false;

	for(const CStack * stack : cb->battleGetAllStacks())
	{
		//using isImmuneBy directly as this mechanics does not have overridden immunity check
		//therefore we do not need to check caster and casting mode
		//TODO: check that we really should check immunity for both stacks
		const bool immune =  ESpellCastProblem::OK != owner->isImmuneBy(stack);
		const bool casterStack = stack->owner == player;

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


void SacrificeMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	RisingSpellMechanics::applyBattleEffects(env, parameters, ctx);

	if(parameters.selectedStack == parameters.cb->battleActiveStack())
	//set another active stack than the one removed, or bad things will happen
	//TODO: make that part of BattleStacksRemoved? what about client update?
	{
		//makeStackDoNothing(gs->curB->getStack (selectedStack));

		BattleSetActiveStack sas;

		//std::vector<const CStack *> hlp;
		//battleGetStackQueue(hlp, 1, selectedStack); //next after this one

		//if(hlp.size())
		//{
		//	sas.stack = hlp[0]->ID;
		//}
		//else
		//	complain ("No new stack to activate!");
		sas.stack = parameters.cb->getNextStack()->ID; //why the hell next stack has same ID as current?
		env->sendAndApply(&sas);

	}
	BattleStacksRemoved bsr;
	bsr.stackIDs.insert(parameters.selectedStack->ID); //somehow it works for teleport?
	env->sendAndApply(&bsr);
}


///SpecialRisingSpellMechanics
ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	// following does apply to resurrect and animate dead(?) only
	// for sacrifice health calculation and health limit check don't matter

	if(obj->count >= obj->baseAmount)
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;

	if(caster) //FIXME: Archangels can cast immune stack
	{
		auto maxHealth = calculateHealedHP(caster, obj, nullptr);
		if (maxHealth < obj->MaxHealth()) //must be able to rise at least one full creature
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	return DefaultSpellMechanics::isImmuneByStack(caster,obj);
}

///SummonMechanics
ESpellCastProblem::ESpellCastProblem SummonMechanics::canBeCasted(const CBattleInfoCallback * cb, PlayerColor player) const
{
	const ui8 side = cb->playerToSide(player);

	//check if there are summoned elementals of other type
	
	auto otherSummoned = cb->battleGetStacksIf([side, this](const CStack * st)
	{
		return (st->attackerOwned == !side)
			&& (vstd::contains(st->state, EBattleStackState::SUMMONED))
			&& (st->getCreature()->idNumber != creatureToSummon);
	});
	
	if(!otherSummoned.empty())
		return ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED;	

	return ESpellCastProblem::OK;
}

void SummonMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	BattleStackAdded bsa;
	bsa.creID = creatureToSummon;
	bsa.attacker = !(bool)parameters.casterSide;
	bsa.summoned = true;
	bsa.pos = parameters.cb->getAvaliableHex(creatureToSummon, !(bool)parameters.casterSide); //TODO: unify it

	//TODO stack casting -> probably power will be zero; set the proper number of creatures manually
	int percentBonus = parameters.caster ? parameters.caster->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, owner->id.toEnum()) : 0;

	bsa.amount = parameters.usedSpellPower
		* owner->getPower(parameters.spellLvl)
		* (100 + percentBonus) / 100.0; //new feature - percentage bonus
	if(bsa.amount)
		env->sendAndApply(&bsa);
	else
		env->complain("Summoning didn't summon any!");
}

///TeleportMechanics
void TeleportMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	BattleStackMoved bsm;
	bsm.distance = -1;
	bsm.stack = parameters.selectedStack->ID;
	std::vector<BattleHex> tiles;
	tiles.push_back(parameters.destination);
	bsm.tilesToMove = tiles;
	bsm.teleporting = true;
	env->sendAndApply(&bsm);
}

