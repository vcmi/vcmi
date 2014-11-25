/*
 * SpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SpellMechanics.h"

#include "mapObjects/CGHeroInstance.h"
#include "BattleState.h"
#include "CRandomGenerator.h"

#include "NetPacks.h"

namespace SRSLPraserHelpers
{
	static int XYToHex(int x, int y)
	{
		return x + GameConstants::BFIELD_WIDTH * y;
	}

	static int XYToHex(std::pair<int, int> xy)
	{
		return XYToHex(xy.first, xy.second);
	}

	static int hexToY(int battleFieldPosition)
	{
		return battleFieldPosition/GameConstants::BFIELD_WIDTH;
	}

	static int hexToX(int battleFieldPosition)
	{
		int pos = battleFieldPosition - hexToY(battleFieldPosition) * GameConstants::BFIELD_WIDTH;
		return pos;
	}

	static std::pair<int, int> hexToPair(int battleFieldPosition)
	{
		return std::make_pair(hexToX(battleFieldPosition), hexToY(battleFieldPosition));
	}

	//moves hex by one hex in given direction
	//0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left
	static std::pair<int, int> gotoDir(int x, int y, int direction)
	{
		switch(direction)
		{
		case 0: //top left
			return std::make_pair((y%2) ? x-1 : x, y-1);
		case 1: //top right
			return std::make_pair((y%2) ? x : x+1, y-1);
		case 2:  //right
			return std::make_pair(x+1, y);
		case 3: //right bottom
			return std::make_pair((y%2) ? x : x+1, y+1);
		case 4: //left bottom
			return std::make_pair((y%2) ? x-1 : x, y+1);
		case 5: //left
			return std::make_pair(x-1, y);
		default:
			throw std::runtime_error("Disaster: wrong direction in SRSLPraserHelpers::gotoDir!\n");
		}
	}

	static std::pair<int, int> gotoDir(std::pair<int, int> xy, int direction)
	{
		return gotoDir(xy.first, xy.second, direction);
	}

	static bool isGoodHex(std::pair<int, int> xy)
	{
		return xy.first >=0 && xy.first < GameConstants::BFIELD_WIDTH && xy.second >= 0 && xy.second < GameConstants::BFIELD_HEIGHT;
	}

	//helper function for rangeInHexes
	static std::set<ui16> getInRange(unsigned int center, int low, int high)
	{
		std::set<ui16> ret;
		if(low == 0)
		{
			ret.insert(center);
		}

		std::pair<int, int> mainPointForLayer[6]; //A, B, C, D, E, F points
		for(auto & elem : mainPointForLayer)
			elem = hexToPair(center);

		for(int it=1; it<=high; ++it) //it - distance to the center
		{
			for(int b=0; b<6; ++b)
				mainPointForLayer[b] = gotoDir(mainPointForLayer[b], b);

			if(it>=low)
			{
				std::pair<int, int> curHex;

				//adding lines (A-b, B-c, C-d, etc)
				for(int v=0; v<6; ++v)
				{
					curHex = mainPointForLayer[v];
					for(int h=0; h<it; ++h)
					{
						if(isGoodHex(curHex))
							ret.insert(XYToHex(curHex));
						curHex = gotoDir(curHex, (v+2)%6);
					}
				}

			} //if(it>=low)
		}

		return ret;
	}
}


///ISpellMechanics
ISpellMechanics::ISpellMechanics(CSpell * s):
	owner(s)
{
	
}


///DefaultSpellMechanics

//bool DefaultSpellMechanics::adventureCast(const SpellCastContext& context) const
//{
//	return false; //there is no general algorithm for casting adventure spells
//}

bool DefaultSpellMechanics::battleCast(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters) const
{
	BattleSpellCast sc;
	sc.side = parameters.casterSide;
	sc.id = owner->id;
	sc.skill = parameters.spellLvl;
	sc.tile = parameters.destination;
	sc.dmgToDisplay = 0;
	sc.castedByHero = nullptr != parameters.caster;
	sc.attackerType = (parameters.casterStack ? parameters.casterStack->type->idNumber : CreatureID(CreatureID::NONE));
	sc.manaGained = 0;
	sc.spellCost = 0;	
	
	//calculate spell cost
	if (parameters.caster) 
	{
		sc.spellCost = parameters.cb->battleGetSpellCost(owner, parameters.caster);

		if (parameters.secHero && parameters.mode == ECastingMode::HERO_CASTING) //handle mana channel
		{
			int manaChannel = 0;
			for(const CStack * stack : parameters.cb->battleGetAllStacks(true)) //TODO: shouldn't bonus system handle it somehow?
			{
				if (stack->owner == parameters.secHero->tempOwner)
				{
					vstd::amax(manaChannel, stack->valOfBonuses(Bonus::MANA_CHANNELING));
				}
			}
			sc.manaGained = (manaChannel * sc.spellCost) / 100;
		}
	}	
	
	
	//calculating affected creatures for all spells
	//must be vector, as in Chain Lightning order matters
	std::vector<const CStack*> attackedCres; //CStack vector is somewhat more suitable than ID vector

	auto creatures = owner->getAffectedStacks(parameters.cb, parameters.mode, parameters.casterColor, parameters.spellLvl, parameters.destination, parameters.caster);
	std::copy(creatures.begin(), creatures.end(), std::back_inserter(attackedCres));
	
	for (auto cre : attackedCres)
	{
		sc.affectedCres.insert (cre->ID);
	}
	
	//checking if creatures resist
	//resistance is applied only to negative spells
	if(owner->isNegative())
	{
		for(auto s : attackedCres)
		{
			const int prob = std::min((s)->magicResistance(), 100); //probability of resistance in %
			
			if(env->getRandomGenerator().nextInt(99) < prob)
			{
				sc.resisted.push_back(s->ID);
			}
		}
	}
	
	//TODO: extract dmg to display calculation	
	//calculating dmg to display
	if (owner->id == SpellID::DEATH_STARE || owner->id == SpellID::ACID_BREATH_DAMAGE)
	{
		sc.dmgToDisplay = parameters.usedSpellPower;
		if (owner->id == SpellID::DEATH_STARE)
			vstd::amin(sc.dmgToDisplay, (*attackedCres.begin())->count); //stack is already reduced after attack
	}	
	
	StacksInjured si;
	
	//TODO:applying effects
	
	
	env->sendAndApply(&sc);
	if(!si.stacks.empty()) //after spellcast info shows
		env->sendAndApply(&si);
	
	//reduce number of casts remaining
	//TODO: this should be part of BattleSpellCast apply
	if (parameters.mode == ECastingMode::CREATURE_ACTIVE_CASTING || parameters.mode == ECastingMode::ENCHANTER_CASTING) 
	{
		assert(parameters.casterStack);
		
		BattleSetStackProperty ssp;
		ssp.stackID = parameters.casterStack->ID;
		ssp.which = BattleSetStackProperty::CASTS;
		ssp.val = -1;
		ssp.absolute = false;
		env->sendAndApply(&ssp);
	}

	//Magic Mirror effect
	if (owner->isNegative() && parameters.mode != ECastingMode::MAGIC_MIRROR && owner->level && owner->getLevelInfo(0).range == "0") //it is actual spell and can be reflected to single target, no recurrence
	{
		for(auto & attackedCre : attackedCres)
		{
			int mirrorChance = (attackedCre)->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(mirrorChance > env->getRandomGenerator().nextInt(99))
			{
				std::vector<const CStack *> mirrorTargets;
				auto battleStacks = parameters.cb->battleGetAllStacks(true);
				for (auto & battleStack : battleStacks)
				{
					if(battleStack->owner == parameters.casterColor) //get enemy stacks which can be affected by this spell
					{
						if (ESpellCastProblem::OK == owner->isImmuneByStack(nullptr, battleStack))
							mirrorTargets.push_back(battleStack);
					}
				}
				if (!mirrorTargets.empty())
				{
					int targetHex = (*RandomGeneratorUtil::nextItem(mirrorTargets, env->getRandomGenerator()))->position;
					
					BattleSpellCastParameters mirrorParameters = parameters;
					mirrorParameters.spellLvl = 0;
					mirrorParameters.casterSide = 1-parameters.casterSide;
					mirrorParameters.casterColor = (attackedCre)->owner;
					mirrorParameters.caster = nullptr;
					mirrorParameters.destination = targetHex;
					mirrorParameters.secHero = parameters.caster;
					mirrorParameters.mode = ECastingMode::MAGIC_MIRROR;
					mirrorParameters.casterStack = (attackedCre);
					mirrorParameters.selectedStack = nullptr;
					
					battleCast(env, mirrorParameters);					
				}
			}
		}
	}	
	
	return true;
}

std::vector<BattleHex> DefaultSpellMechanics::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes) const
{
	using namespace SRSLPraserHelpers;
	
	std::vector<BattleHex> ret;
	std::string rng = owner->getLevelInfo(schoolLvl).range + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 2 && rng[0] != 'X') //there is at lest one hex in range (+artificial comma)
	{
		std::string number1, number2;
		int beg, end;
		bool readingFirst = true;
		for(auto & elem : rng)
		{
			if(std::isdigit(elem) ) //reading number
			{
				if(readingFirst)
					number1 += elem;
				else
					number2 += elem;
			}
			else if(elem == ',') //comma
			{
				//calculating variables
				if(readingFirst)
				{
					beg = atoi(number1.c_str());
					number1 = "";
				}
				else
				{
					end = atoi(number2.c_str());
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					curLayer = getInRange(centralHex, beg, beg);
				}
				else
				{
					curLayer = getInRange(centralHex, beg, end);
					readingFirst = true;
				}
				//adding abtained hexes
				for(auto & curLayer_it : curLayer)
				{
					ret.push_back(curLayer_it);
				}

			}
			else if(elem == '-') //dash
			{
				beg = atoi(number1.c_str());
				number1 = "";
				readingFirst = false;
			}
		}
	}

	//remove duplicates (TODO check if actually needed)
	range::unique(ret);
	return ret;		
}


std::set<const CStack *> DefaultSpellMechanics::getAffectedStacks(SpellTargetingContext & ctx) const
{
	std::set<const CStack* > attackedCres;//std::set to exclude multiple occurrences of two hex creatures
	
	const ui8 attackerSide = ctx.cb->playerToSide(ctx.casterColor) == 1;
	const auto attackedHexes = rangeInHexes(ctx.destination, ctx.schoolLvl, attackerSide);

	const CSpell::TargetInfo ti(owner, ctx.schoolLvl, ctx.mode);
	
	//TODO: more generic solution for mass spells
	if (owner->getLevelInfo(ctx.schoolLvl).range.size() > 1) //custom many-hex range
	{
		for(BattleHex hex : attackedHexes)
		{
			if(const CStack * st = ctx.cb->battleGetStackByPos(hex, ti.onlyAlive))
			{
				attackedCres.insert(st);
			}
		}
	}
	else if(ti.type == CSpell::CREATURE)
	{
		auto predicate = [=](const CStack * s){
			const bool positiveToAlly = owner->isPositive() && s->owner == ctx.casterColor;
			const bool negativeToEnemy = owner->isNegative() && s->owner != ctx.casterColor;
			const bool validTarget = s->isValidTarget(!ti.onlyAlive); //todo: this should be handled by spell class
	
			//for single target spells select stacks covering destination tile
			const bool rangeCovers = ti.massive || s->coversPos(ctx.destination);
			//handle smart targeting
			const bool positivenessFlag = !ti.smart || owner->isNeutral() || positiveToAlly || negativeToEnemy;
			
			return rangeCovers && positivenessFlag && validTarget;		
		};
		
		TStacks stacks = ctx.cb->battleGetStacksIf(predicate);
		
		if (ti.massive)
		{
			//for massive spells add all targets
			for (auto stack : stacks)
				attackedCres.insert(stack);

		}
		else
		{
			//for single target spells we must select one target. Alive stack is preferred (issue #1763)
			for(auto stack : stacks)
			{
				if(stack->alive())
				{
					attackedCres.insert(stack);
					break;
				}				
			}	
			
			if(attackedCres.empty() && !stacks.empty())
			{
				attackedCres.insert(stacks.front());
			}						
		}
	}
	else //custom range from attackedHexes
	{
		for(BattleHex hex : attackedHexes)
		{
			if(const CStack * st = ctx.cb->battleGetStackByPos(hex, ti.onlyAlive))
				attackedCres.insert(st);
		}
	}	
	
	return attackedCres;
}


ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	//by default use general algorithm
	return owner->isImmuneBy(obj);
}

///WallMechanics
std::vector<BattleHex> WallMechanics::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool* outDroppedHexes) const
{
	using namespace SRSLPraserHelpers;

	std::vector<BattleHex> ret;	
	
	//Special case - shape of obstacle depends on caster's side
	//TODO make it possible through spell_info config

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
ESpellCastProblem::ESpellCastProblem CloneMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack * obj) const
{
	//can't clone already cloned creature
	if (vstd::contains(obj->state, EBattleStackState::CLONED))
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	//TODO: how about stacks casting Clone?
	//currently Clone casted by stack is assumed Expert level
	ui8 schoolLevel;
	if (caster)
	{
		schoolLevel = caster->getSpellSchoolLevel(owner);
	}
	else
	{
		schoolLevel = 3;
	}

	if (schoolLevel < 3)
	{
		int maxLevel = (std::max(schoolLevel, (ui8)1) + 4);
		int creLevel = obj->getCreature()->level;
		if (maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, any level for expert
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	//use default algorithm only if there is no mechanics-related problem		
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

///DispellHelpfulMechanics
ESpellCastProblem::ESpellCastProblem DispellHelpfulMechanics::isImmuneByStack(const CGHeroInstance* caster,  const CStack* obj) const
{
	TBonusListPtr spellBon = obj->getSpellBonuses();
	bool hasPositiveSpell = false;
	for(const Bonus * b : *spellBon)
	{
		if(SpellID(b->sid).toSpell()->isPositive())
		{
			hasPositiveSpell = true;
			break;
		}
	}
	if(!hasPositiveSpell)
	{
		return ESpellCastProblem::NO_SPELLS_TO_DISPEL;
	}
	
	//use default algorithm only if there is no mechanics-related problem		
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

///HypnotizeMechanics
ESpellCastProblem::ESpellCastProblem HypnotizeMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack* obj) const
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
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);
}


///SpecialRisingSpellMechanics
ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::isImmuneByStack(const CGHeroInstance* caster, const CStack* obj) const
{
	// following does apply to resurrect and animate dead(?) only
	// for sacrifice health calculation and health limit check don't matter

	if(obj->count >= obj->baseAmount)
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	
	if (caster) //FIXME: Archangels can cast immune stack
	{
		auto maxHealth = owner->calculateHealedHP (caster, obj);
		if (maxHealth < obj->MaxHealth()) //must be able to rise at least one full creature
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}	
	
	return DefaultSpellMechanics::isImmuneByStack(caster,obj);	
}

	

