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

#include "../CGeneralTextHandler.h"

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
	if (packet->castByHero)
	{
		if (packet->side < 2)
		{
			battle->sides[packet->side].castSpellsCount++;
		}
	}

	//handle countering spells
	for(auto stackID : packet->affectedCres)
	{
		CStack * s = battle->getStack(stackID);
		s->popBonuses([&](const Bonus * b) -> bool
		{
			//check for each bonus if it should be removed
			const bool isSpellEffect = Selector::sourceType(Bonus::SPELL_EFFECT)(b);
			const int spellID = isSpellEffect ? b->sid : -1;
			//No exceptions, ANY spell can be countered, even if it can`t be dispelled.
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

	if(caster->inTownGarrison)
	{
		env->complain("Attempt to cast an adventure spell in town garrison");
		return false;
	}

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
	logGlobal->debugStream() << "Started spell cast. Spell: "<<owner->name<<"; mode:"<<parameters.mode;

	if(nullptr == parameters.caster)
	{
		env->complain("No spell-caster provided.");
		return;
	}

	BattleSpellCast sc;
	prepareBattleCast(parameters, sc);

	//check it there is opponent hero
	const ui8 otherSide = 1-parameters.casterSide;
	const CGHeroInstance * otherHero = nullptr;
	if(parameters.cb->battleHasHero(otherSide))
		otherHero = parameters.cb->battleGetFightingHero(otherSide);
	int spellCost = 0;

	//calculate spell cost
	if(parameters.mode == ECastingMode::HERO_CASTING)
	{
		spellCost = parameters.cb->battleGetSpellCost(owner, parameters.casterHero);

		if(nullptr != otherHero) //handle mana channel
		{
			int manaChannel = 0;
			for(const CStack * stack : parameters.cb->battleGetAllStacks(true)) //TODO: shouldn't bonus system handle it somehow?
			{
				if(stack->owner == otherHero->tempOwner)
				{
					vstd::amax(manaChannel, stack->valOfBonuses(Bonus::MANA_CHANNELING));
				}
			}
			sc.manaGained = (manaChannel * spellCost) / 100;
		}
	}
	logGlobal->debugStream() << "spellCost: " << spellCost;

	//calculating affected creatures for all spells
	//must be vector, as in Chain Lightning order matters
	std::vector<const CStack*> attackedCres; //CStack vector is somewhat more suitable than ID vector

	auto creatures = owner->getAffectedStacks(parameters.cb, parameters.mode, parameters.caster, parameters.spellLvl, parameters.getFirstDestinationHex());
	std::copy(creatures.begin(), creatures.end(), std::back_inserter(attackedCres));

	logGlobal->debugStream() << "will affect: " << attackedCres.size() << " stacks";

	std::vector <const CStack*> reflected;//for magic mirror
	//checking if creatures resist
	handleResistance(env, attackedCres, sc);
	//it is actual spell and can be reflected to single target, no recurrence
	const bool tryMagicMirror = owner->isNegative() && owner->level && owner->getLevelInfo(0).range == "0";
	if(tryMagicMirror)
	{
		for(auto s : attackedCres)
		{
			const int mirrorChance = (s)->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(env->getRandomGenerator().nextInt(99) < mirrorChance)
				reflected.push_back(s);
		}

		vstd::erase_if(attackedCres, [&reflected](const CStack * s)
		{
			return vstd::contains(reflected, s);
		});

		for(auto s : reflected)
		{
			BattleSpellCast::CustomEffect effect;
			effect.effect = 3;
			effect.stack = s->ID;
			sc.customEffects.push_back(effect);
		}
	}

	for(auto cre : attackedCres)
	{
		sc.affectedCres.insert(cre->ID);
	}

	StacksInjured si;
	SpellCastContext ctx(attackedCres, sc, si);
	applyBattleEffects(env, parameters, ctx);

	env->sendAndApply(&sc);

	//spend mana
	if(parameters.mode == ECastingMode::HERO_CASTING)
	{
		SetMana sm;
		sm.absolute = false;

		sm.hid = parameters.casterHero->id;
		sm.val = -spellCost;

		env->sendAndApply(&sm);

		if(sc.manaGained > 0)
		{
			assert(otherHero);

			sm.hid = otherHero->id;
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
	logGlobal->debugStream() << "Finished spell cast. Spell: "<<owner->name<<"; mode:"<<parameters.mode;
	//Magic Mirror effect
	for(auto & attackedCre : reflected)
	{
		TStacks mirrorTargets = parameters.cb->battleGetStacksIf([this, parameters](const CStack * battleStack)
		{
			//Get all enemy stacks. Magic mirror can reflect to immune creature (with no effect)
			return battleStack->owner == parameters.casterColor && battleStack->isValidTarget(false);
		});

		if(!mirrorTargets.empty())
		{
			int targetHex = (*RandomGeneratorUtil::nextItem(mirrorTargets, env->getRandomGenerator()))->position;

			BattleSpellCastParameters mirrorParameters(parameters.cb, attackedCre, owner);
			mirrorParameters.spellLvl = 0;
			mirrorParameters.aimToHex(targetHex);
			mirrorParameters.mode = ECastingMode::MAGIC_MIRROR;
			mirrorParameters.selectedStack = nullptr;
			mirrorParameters.spellLvl = parameters.spellLvl;
			mirrorParameters.effectLevel = parameters.effectLevel;
			mirrorParameters.effectPower = parameters.effectPower;
			mirrorParameters.effectValue = parameters.effectValue;
			mirrorParameters.enchantPower = parameters.enchantPower;
			castMagicMirror(env, mirrorParameters);
		}
	}
}

void DefaultSpellMechanics::battleLogSingleTarget(std::vector<std::string> & logLines, const BattleSpellCast * packet,
	const std::string & casterName, const CStack * attackedStack, bool & displayDamage) const
{
	const std::string attackedName = attackedStack->getName();
	const std::string attackedNameSing = attackedStack->getCreature()->nameSing;
	const std::string attackedNamePl = attackedStack->getCreature()->namePl;

	auto getPluralFormat = [attackedStack](const int baseTextID) -> boost::format
	{
		return boost::format(VLC->generaltexth->allTexts[(attackedStack->count > 1 ? baseTextID + 1 : baseTextID)]);
	};

	auto logSimple = [&logLines, getPluralFormat, attackedName](const int baseTextID)
	{
		boost::format fmt = getPluralFormat(baseTextID);
		fmt % attackedName;
		logLines.push_back(fmt.str());
	};

	auto logPlural = [&logLines, attackedNamePl](const int baseTextID)
	{
		boost::format fmt(VLC->generaltexth->allTexts[baseTextID]);
		fmt % attackedNamePl;
		logLines.push_back(fmt.str());
	};

	displayDamage = false; //in most following cases damage info text is custom
	switch(owner->id)
	{
	case SpellID::STONE_GAZE:
		logSimple(558);
		break;
	case SpellID::POISON:
		logSimple(561);
		break;
	case SpellID::BIND:
		logPlural(560);//Roots and vines bind the %s to the ground!
		break;
	case SpellID::DISEASE:
		logSimple(553);
		break;
	case SpellID::PARALYZE:
		logSimple(563);
		break;
	case SpellID::AGE:
		{
			boost::format text = getPluralFormat(551);
			text % attackedName;
			//The %s shrivel with age, and lose %d hit points."
			TBonusListPtr bl = attackedStack->getBonuses(Selector::type(Bonus::STACK_HEALTH));
			const int fullHP = bl->totalValue();
			bl->remove_if(Selector::source(Bonus::SPELL_EFFECT, SpellID::AGE));
			text % (fullHP - bl->totalValue());
			logLines.push_back(text.str());
		}
		break;
	case SpellID::THUNDERBOLT:
		{
			logPlural(367);
			std::string text = VLC->generaltexth->allTexts[343].substr(1, VLC->generaltexth->allTexts[343].size() - 1); //Does %d points of damage.
			boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(packet->dmgToDisplay)); //no more text afterwards
			logLines.push_back(text);
		}
		break;
	case SpellID::DISPEL_HELPFUL_SPELLS:
		logPlural(555);
		break;
	case SpellID::DEATH_STARE:
		if (packet->dmgToDisplay > 0)
		{
			std::string text;
			if (packet->dmgToDisplay > 1)
			{
				text = VLC->generaltexth->allTexts[119]; //%d %s die under the terrible gaze of the %s.
				boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(packet->dmgToDisplay));
				boost::algorithm::replace_first(text, "%s", attackedNamePl);
			}
			else
			{
				text = VLC->generaltexth->allTexts[118]; //One %s dies under the terrible gaze of the %s.
				boost::algorithm::replace_first(text, "%s", attackedNameSing);
			}
			boost::algorithm::replace_first(text, "%s", casterName); //casting stack
			logLines.push_back(text);
		}
		break;
	default:
		{
			boost::format text(VLC->generaltexth->allTexts[565]); //The %s casts %s
			text % casterName % owner->name;
			displayDamage = true;
			logLines.push_back(text.str());
		}
		break;
	}
}

void DefaultSpellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters, SpellCastContext & ctx) const
{
	//applying effects
	if(owner->isOffensiveSpell())
	{
		int spellDamage = parameters.effectValue;

		int chainLightningModifier = 0;
		for(auto & attackedCre : ctx.attackedCres)
		{
			BattleStackAttacked bsa;
			if(spellDamage != 0)
				bsa.damageAmount = owner->adjustRawDamage(parameters.caster, attackedCre, spellDamage) >> chainLightningModifier;
			else
				bsa.damageAmount = owner->calculateDamage(parameters.caster, attackedCre, parameters.effectLevel, parameters.effectPower) >> chainLightningModifier;

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
		SetStackEffect sse;
		//get default spell duration (spell power with bonuses for heroes)
		int duration = parameters.enchantPower;
		//generate actual stack bonuses
		{
			int maxDuration = 0;
			std::vector<Bonus> tmp;
			owner->getEffects(tmp, parameters.effectLevel);
			for(Bonus& b : tmp)
			{
				//use configured duration if present
				if(b.turnsRemain == 0)
					b.turnsRemain = duration;
				vstd::amax(maxDuration, b.turnsRemain);
				sse.effect.push_back(b);
			}
			//if all spell effects have special duration, use it
			duration = maxDuration;
		}
		//fix to original config: shield should display damage reduction
		if(owner->id == SpellID::SHIELD || owner->id == SpellID::AIR_SHIELD)
		{
			sse.effect.back().val = (100 - sse.effect.back().val);
		}
		//we need to know who cast Bind
		if(owner->id == SpellID::BIND && parameters.casterStack)
		{
			sse.effect.back().additionalInfo =  parameters.casterStack->ID;
		}
		const Bonus * bonus = nullptr;
		if(parameters.casterHero)
			bonus = parameters.casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, owner->id));
		//TODO does hero specialty should affects his stack casting spells?

		si32 power = 0;
		for(const CStack * affected : ctx.attackedCres)
		{
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
						Bonus specialBonus = CStack::featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, power, duration);
						specialBonus.sid = owner->id;
						sse.uniqueBonuses.push_back(std::pair<ui32,Bonus> (affected->ID, specialBonus)); //additional attack to Slayer effect
					}
					break;
				}
			}
			if (parameters.casterHero && parameters.casterHero->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, owner->id)) //TODO: better handling of bonus percentages
			{
				int damagePercent = parameters.casterHero->level * parameters.casterHero->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, owner->id.toEnum()) / tier;
				Bonus specialBonus = CStack::featureGenerator(Bonus::CREATURE_DAMAGE, 0, damagePercent, duration);
				specialBonus.valType = Bonus::PERCENT_TO_ALL;
				specialBonus.sid = owner->id;
				sse.uniqueBonuses.push_back (std::pair<ui32,Bonus> (affected->ID, specialBonus));
			}
		}

		if(!sse.stacks.empty())
			env->sendAndApply(&sse);

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

	const ui8 attackerSide = ctx.cb->playerToSide(ctx.caster->getOwner()) == 1;
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
			const bool positiveToAlly = owner->isPositive() && s->owner == ctx.caster->getOwner();
			const bool negativeToEnemy = owner->isNegative() && s->owner != ctx.caster->getOwner();
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

ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const
{
	//no problems by default, this method is for spell-specific problems
	return ESpellCastProblem::OK;
}

ESpellCastProblem DefaultSpellMechanics::canBeCast(const SpellTargetingContext & ctx) const
{
	//no problems by default, this method is for spell-specific problems
	//common problems handled by CSpell
	return ESpellCastProblem::OK;
}

ESpellCastProblem::ESpellCastProblem DefaultSpellMechanics::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	//by default use general algorithm
	return owner->internalIsImmune(caster, obj);
}

void DefaultSpellMechanics::doDispell(BattleInfo * battle, const BattleSpellCast * packet, const CSelector & selector) const
{
	auto localSelector = [](const Bonus * bonus)
	{
		const CSpell * sourceSpell = bonus->sourceSpell();
		if(sourceSpell != nullptr)
		{
			//Special case: DISRUPTING_RAY is "immune" to dispell
			//Other even PERMANENT effects can be removed (f.e. BIND)
			if(sourceSpell->id == SpellID::DISRUPTING_RAY)
				return false;
		}
		return true;
	};
	for(auto stackID : packet->affectedCres)
	{
		CStack *s = battle->getStack(stackID);
		s->popBonuses(CSelector(localSelector).And(selector));
	}
}

void DefaultSpellMechanics::castMagicMirror(const SpellCastEnvironment* env, BattleSpellCastParameters& parameters) const
{
	logGlobal->debugStream() << "Started spell cast. Spell: "<<owner->name<<"; mode: MAGIC_MIRROR";
	if(parameters.mode != ECastingMode::MAGIC_MIRROR)
	{
		env->complain("MagicMirror: invalid mode");
		return;
	}
	BattleHex destination = parameters.getFirstDestinationHex();
	if(!destination.isValid())
	{
		env->complain("MagicMirror: invalid destination");
		return;
	}

	BattleSpellCast sc;
	prepareBattleCast(parameters, sc);

	//calculating affected creatures for all spells
	//must be vector, as in Chain Lightning order matters
	std::vector<const CStack*> attackedCres; //CStack vector is somewhat more suitable than ID vector

	auto creatures = owner->getAffectedStacks(parameters.cb, parameters.mode, parameters.caster, parameters.spellLvl, destination);
	std::copy(creatures.begin(), creatures.end(), std::back_inserter(attackedCres));

	logGlobal->debugStream() << "will affect: " << attackedCres.size() << " stacks";

	handleResistance(env, attackedCres, sc);

	for(auto cre : attackedCres)
	{
		sc.affectedCres.insert(cre->ID);
	}

	StacksInjured si;
	SpellCastContext ctx(attackedCres, sc, si);
	applyBattleEffects(env, parameters, ctx);

	env->sendAndApply(&sc);
	if(!si.stacks.empty()) //after spellcast info shows
		env->sendAndApply(&si);
	logGlobal->debugStream() << "Finished spell cast. Spell: "<<owner->name<<"; mode: MAGIC_MIRROR";
}

void DefaultSpellMechanics::handleResistance(const SpellCastEnvironment * env, std::vector<const CStack* >& attackedCres, BattleSpellCast& sc) const
{
	//checking if creatures resist
	//resistance/reflection is applied only to negative spells
	if(owner->isNegative())
	{
		std::vector <const CStack*> resisted;
		for(auto s : attackedCres)
		{
			//magic resistance
			const int prob = std::min((s)->magicResistance(), 100); //probability of resistance in %

			if(env->getRandomGenerator().nextInt(99) < prob)
			{
				resisted.push_back(s);
			}
		}

		vstd::erase_if(attackedCres, [&resisted](const CStack * s)
		{
			return vstd::contains(resisted, s);
		});

		for(auto s : resisted)
		{
			BattleSpellCast::CustomEffect effect;
			effect.effect = 78;
			effect.stack = s->ID;
			sc.customEffects.push_back(effect);
		}
	}
}

void DefaultSpellMechanics::prepareBattleCast(const BattleSpellCastParameters& parameters, BattleSpellCast& sc) const
{
	sc.side = parameters.casterSide;
	sc.id = owner->id;
	sc.skill = parameters.spellLvl;
	sc.tile = parameters.getFirstDestinationHex();
	sc.dmgToDisplay = 0;
	sc.castByHero = parameters.mode == ECastingMode::HERO_CASTING;
	sc.casterStack = (parameters.casterStack ? parameters.casterStack->ID : -1);
	sc.manaGained = 0;
}
