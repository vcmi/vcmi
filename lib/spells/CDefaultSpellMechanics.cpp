/*
 * CDefaultSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CDefaultSpellMechanics.h"

#include "../NetPacks.h"
#include "../BattleState.h"

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

///DefaultSpellMechanics
void DefaultSpellMechanics::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	if (packet->castedByHero)
	{
		if (packet->side < 2)
		{
			battle->sides[packet->side].castSpellsCount++;
		}
	}

	//handle countering spells
	for(auto stackID : packet->affectedCres)
	{
		if(vstd::contains(packet->resisted, stackID))
			continue;

		CStack * s = battle->getStack(stackID);
		s->popBonuses([&](const Bonus * b) -> bool
		{
			//check for each bonus if it should be removed
			const bool isSpellEffect = Selector::sourceType(Bonus::SPELL_EFFECT)(b);
			const int spellID = isSpellEffect ? b->sid : -1;

			return isSpellEffect && vstd::contains(owner->counteredSpells, spellID);
		});
	}
}

bool DefaultSpellMechanics::adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
	if(!owner->isAdventureSpell())
	{
		env->complain("Attempt to cast non adventure spell in adventure mode");
		return false;
	}

	const CGHeroInstance * caster = parameters.caster;
	const int cost = caster->getSpellCost(owner);

	if(!caster->canCastThisSpell(owner))
	{
		env->complain("Hero cannot cast this spell!");
		return false;
	}

	if(caster->mana < cost)
	{
		env->complain("Hero doesn't have enough spell points to cast this spell!");
		return false;
	}

	{
		AdvmapSpellCast asc;
		asc.caster = caster;
		asc.spellID = owner->id;
		env->sendAndApply(&asc);
	}
	
	switch(applyAdventureEffects(env, parameters))
	{
	case ESpellCastResult::OK:
		{
			SetMana sm;
			sm.hid = caster->id;
			sm.absolute = false;
			sm.val = -cost;
			env->sendAndApply(&sm);
			return true;			
		}
		break;
	case ESpellCastResult::CANCEL:
		return true;
	}
	return false;
}

ESpellCastResult DefaultSpellMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
	if(owner->hasEffects())
	{
		const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

		std::vector<Bonus> bonuses;

		owner->getEffects(bonuses, schoolLevel);

		for(Bonus b : bonuses)
		{
			GiveBonus gb;
			gb.id = parameters.caster->id.getNum();
			gb.bonus = b;
			env->sendAndApply(&gb);
		}

		return ESpellCastResult::OK;
	}
	else
	{
		//There is no generic algorithm of adventure cast
		env->complain("Unimplemented adventure spell");
		return ESpellCastResult::ERROR;
	}
}

void DefaultSpellMechanics::battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const
{
	BattleSpellCast sc;
	sc.side = parameters.casterSide;
	sc.id = owner->id;
	sc.skill = parameters.spellLvl;
	sc.tile = parameters.destination;
	sc.dmgToDisplay = 0;
	sc.castedByHero = nullptr != parameters.caster;
	sc.casterStack = (parameters.casterStack ? parameters.casterStack->ID : -1);
	sc.manaGained = 0;

	int spellCost = 0;

	//calculate spell cost
	if(parameters.caster)
	{
		spellCost = parameters.cb->battleGetSpellCost(owner, parameters.caster);

		if(parameters.secHero && parameters.mode == ECastingMode::HERO_CASTING) //handle mana channel
		{
			int manaChannel = 0;
			for(const CStack * stack : parameters.cb->battleGetAllStacks(true)) //TODO: shouldn't bonus system handle it somehow?
			{
				if(stack->owner == parameters.secHero->tempOwner)
				{
					vstd::amax(manaChannel, stack->valOfBonuses(Bonus::MANA_CHANNELING));
				}
			}
			sc.manaGained = (manaChannel * spellCost) / 100;
		}
	}

	//calculating affected creatures for all spells
	//must be vector, as in Chain Lightning order matters
	std::vector<const CStack*> attackedCres; //CStack vector is somewhat more suitable than ID vector

	auto creatures = owner->getAffectedStacks(parameters.cb, parameters.mode, parameters.casterColor, parameters.spellLvl, parameters.destination, parameters.caster);
	std::copy(creatures.begin(), creatures.end(), std::back_inserter(attackedCres));

	for (auto cre : attackedCres)
	{
		sc.affectedCres.insert(cre->ID);
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

	StacksInjured si;
	SpellCastContext ctx(attackedCres, sc, si);

	applyBattleEffects(env, parameters, ctx);

	env->sendAndApply(&sc);

	//spend mana
	if(parameters.caster)
	{
		SetMana sm;
		sm.absolute = false;

		sm.hid = parameters.caster->id;
		sm.val = -spellCost;

		env->sendAndApply(&sm);

		if(sc.manaGained > 0)
		{
			assert(parameters.secHero);

			sm.hid = parameters.secHero->id;
			sm.val = sc.manaGained;
			env->sendAndApply(&sm);
		}
	}

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
	if(owner->isNegative() && parameters.mode != ECastingMode::MAGIC_MIRROR && owner->level && owner->getLevelInfo(0).range == "0") //it is actual spell and can be reflected to single target, no recurrence
	{
		for(auto & attackedCre : attackedCres)
		{
			int mirrorChance = (attackedCre)->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(mirrorChance > env->getRandomGenerator().nextInt(99))
			{
				std::vector<const CStack *> mirrorTargets;
				auto battleStacks = parameters.cb->battleGetAllStacks(true);
				for(auto & battleStack : battleStacks)
				{
					if(battleStack->owner == parameters.casterColor) //get enemy stacks which can be affected by this spell
					{
						if (ESpellCastProblem::OK == owner->isImmuneByStack(nullptr, battleStack))
							mirrorTargets.push_back(battleStack);
					}
				}
				if(!mirrorTargets.empty())
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
}

int DefaultSpellMechanics::calculateDuration(const CGHeroInstance * caster, int usedSpellPower) const
{
	if(!caster)
	{
		if (!usedSpellPower)
			return 3; //default duration of all creature spells
		else
			return usedSpellPower; //use creature spell power
	}
	switch(owner->id)
	{
	case SpellID::FRENZY:
		return 1;
	default: //other spells
		return caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER) + caster->valOfBonuses(Bonus::SPELL_DURATION);
	}
}

ui32 DefaultSpellMechanics::calculateHealedHP(const CGHeroInstance* caster, const CStack* stack, const CStack* sacrificedStack) const
{
	int healedHealth;

	if(!owner->isHealingSpell())
	{
		logGlobal->errorStream() << "calculateHealedHP called for nonhealing spell "<< owner->name;
		return 0;
	}

	const int spellPowerSkill = caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER);
	const int levelPower = owner->getPower(caster->getSpellSchoolLevel(owner));

	if (owner->id == SpellID::SACRIFICE && sacrificedStack)
		healedHealth = (spellPowerSkill + sacrificedStack->MaxHealth() + levelPower) * sacrificedStack->count;
	else
		healedHealth = spellPowerSkill * owner->power + levelPower; //???
	healedHealth = owner->calculateBonus(healedHealth, caster, stack);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (owner->isRisingSpell() ? stack->baseAmount * stack->MaxHealth() : 0));
}


void DefaultSpellMechanics::applyBattleEffects(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//applying effects
	if(owner->isOffensiveSpell())
	{
		int spellDamage = 0;
		if(parameters.casterStack && parameters.mode != ECastingMode::MAGIC_MIRROR)
		{
			int unitSpellPower = parameters.casterStack->valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, owner->id.toEnum());
			if(unitSpellPower)
				ctx.sc.dmgToDisplay = spellDamage = parameters.casterStack->count * unitSpellPower; //TODO: handle immunities
			else //Faerie Dragon
			{
				parameters.usedSpellPower = parameters.casterStack->valOfBonuses(Bonus::CREATURE_SPELL_POWER) * parameters.casterStack->count / 100;
				ctx.sc.dmgToDisplay = 0;
			}
		}
		int chainLightningModifier = 0;
		for(auto & attackedCre : ctx.attackedCres)
		{
			if(vstd::contains(ctx.sc.resisted, (attackedCre)->ID)) //this creature resisted the spell
				continue;

			BattleStackAttacked bsa;
			if(spellDamage)
				bsa.damageAmount = spellDamage >> chainLightningModifier;
			else
				bsa.damageAmount =  owner->calculateDamage(parameters.caster, attackedCre, parameters.spellLvl, parameters.usedSpellPower) >> chainLightningModifier;

			ctx.sc.dmgToDisplay += bsa.damageAmount;

			bsa.stackAttacked = (attackedCre)->ID;
			if(parameters.mode == ECastingMode::ENCHANTER_CASTING) //multiple damage spells cast
				bsa.attackerID = parameters.casterStack->ID;
			else
				bsa.attackerID = -1;
			(attackedCre)->prepareAttacked(bsa, env->getRandomGenerator());
			ctx.si.stacks.push_back(bsa);

			if(owner->id == SpellID::CHAIN_LIGHTNING)
				++chainLightningModifier;
		}
	}

	if(owner->hasEffects())
	{
		int stackSpellPower = 0;
		if(parameters.casterStack && parameters.mode != ECastingMode::MAGIC_MIRROR)
		{
			stackSpellPower =  parameters.casterStack->valOfBonuses(Bonus::CREATURE_ENCHANT_POWER);
		}
		SetStackEffect sse;
		Bonus pseudoBonus;
		pseudoBonus.sid = owner->id;
		pseudoBonus.val = parameters.spellLvl;
		pseudoBonus.turnsRemain = calculateDuration(parameters.caster, stackSpellPower ? stackSpellPower : parameters.usedSpellPower);
		CStack::stackEffectToFeature(sse.effect, pseudoBonus);
		if(owner->id == SpellID::SHIELD || owner->id == SpellID::AIR_SHIELD)
		{
			sse.effect.back().val = (100 - sse.effect.back().val); //fix to original config: shield should display damage reduction
		}
		if(owner->id == SpellID::BIND &&  parameters.casterStack)//bind
		{
			sse.effect.back().additionalInfo =  parameters.casterStack->ID; //we need to know who casted Bind
		}
		const Bonus * bonus = nullptr;
		if(parameters.caster)
			bonus = parameters.caster->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, owner->id));
		//TODO does hero specialty should affects his stack casting spells?

		si32 power = 0;
		for(const CStack * affected : ctx.attackedCres)
		{
			if(vstd::contains(ctx.sc.resisted, affected->ID)) //this creature resisted the spell
				continue;
			sse.stacks.push_back(affected->ID);

			//Apply hero specials - peculiar enchants
			const ui8 tier = std::max((ui8)1, affected->getCreature()->level); //don't divide by 0 for certain creatures (commanders, war machines)
			if(bonus)
			{
				switch(bonus->additionalInfo)
				{
					case 0: //normal
					{
						switch(tier)
						{
							case 1: case 2:
								power = 3;
							break;
							case 3: case 4:
								power = 2;
							break;
							case 5: case 6:
								power = 1;
							break;
						}
						Bonus specialBonus(sse.effect.back());
						specialBonus.val = power; //it doesn't necessarily make sense for some spells, use it wisely
						sse.uniqueBonuses.push_back (std::pair<ui32,Bonus> (affected->ID, specialBonus)); //additional premy to given effect
					}
					break;
					case 1: //only Coronius as yet
					{
						power = std::max(5 - tier, 0);
						Bonus specialBonus = CStack::featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, power, pseudoBonus.turnsRemain);
						specialBonus.sid = owner->id;
						sse.uniqueBonuses.push_back(std::pair<ui32,Bonus> (affected->ID, specialBonus)); //additional attack to Slayer effect
					}
					break;
				}
			}
			if (parameters.caster && parameters.caster->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, owner->id)) //TODO: better handling of bonus percentages
			{
				int damagePercent = parameters.caster->level * parameters.caster->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, owner->id.toEnum()) / tier;
				Bonus specialBonus = CStack::featureGenerator(Bonus::CREATURE_DAMAGE, 0, damagePercent, pseudoBonus.turnsRemain);
				specialBonus.valType = Bonus::PERCENT_TO_ALL;
				specialBonus.sid = owner->id;
				sse.uniqueBonuses.push_back (std::pair<ui32,Bonus> (affected->ID, specialBonus));
			}
		}

		if(!sse.stacks.empty())
			env->sendAndApply(&sse);

	}

	if(owner->isHealingSpell())
	{
		int hpGained = 0;
		if(parameters.casterStack)
		{
			int unitSpellPower = parameters.casterStack->valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, owner->id.toEnum());
			if(unitSpellPower)
				hpGained = parameters.casterStack->count * unitSpellPower; //Archangel
			else //Faerie Dragon-like effect - unused so far
				parameters.usedSpellPower = parameters.casterStack->valOfBonuses(Bonus::CREATURE_SPELL_POWER) * parameters.casterStack->count / 100;
		}
		StacksHealedOrResurrected shr;
		shr.lifeDrain = false;
		shr.tentHealing = false;
		for(auto & attackedCre : ctx.attackedCres)
		{
			StacksHealedOrResurrected::HealInfo hi;
			hi.stackID = (attackedCre)->ID;
			if (parameters.casterStack) //casted by creature
			{
				const bool resurrect = owner->isRisingSpell();
				if (hpGained)
				{
					//archangel
					hi.healedHP = std::min<ui32>(hpGained, attackedCre->MaxHealth() - attackedCre->firstHPleft + (resurrect ? attackedCre->baseAmount * attackedCre->MaxHealth() : 0));
				}
				else
				{
					//any typical spell (commander's cure or animate dead)
					int healedHealth = parameters.usedSpellPower * owner->power + owner->getPower(parameters.spellLvl);
					hi.healedHP = std::min<ui32>(healedHealth, attackedCre->MaxHealth() - attackedCre->firstHPleft + (resurrect ? attackedCre->baseAmount * attackedCre->MaxHealth() : 0));
				}
			}
			else
				hi.healedHP = calculateHealedHP(parameters.caster, attackedCre, parameters.selectedStack); //Casted by hero
			hi.lowLevelResurrection = (parameters.spellLvl <= 1) && (owner->id != SpellID::ANIMATE_DEAD);
			shr.healedStacks.push_back(hi);
		}
		if(!shr.healedStacks.empty())
			env->sendAndApply(&shr);
	}
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
	if(owner->getLevelInfo(ctx.schoolLvl).range.size() > 1) //custom many-hex range
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

		if(ti.massive)
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

ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::canBeCasted(const CBattleInfoCallback * cb, PlayerColor player) const
{
	//no problems by default, this method is for spell-specific problems	
	return ESpellCastProblem::OK;
}


ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const
{
	//by default use general algorithm
	return owner->isImmuneBy(obj);
}

void DefaultSpellMechanics::doDispell(BattleInfo * battle, const BattleSpellCast * packet, const CSelector & selector) const
{
	for(auto stackID : packet->affectedCres)
	{
		if(vstd::contains(packet->resisted, stackID))
			continue;

		CStack *s = battle->getStack(stackID);
		s->popBonuses(selector);
	}	
}
