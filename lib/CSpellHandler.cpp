#include "StdInc.h"
#include "CSpellHandler.h"

#include "CGeneralTextHandler.h"
#include "filesystem/Filesystem.h"

#include "JsonNode.h"
#include <cctype>
#include "BattleHex.h"
#include "CModHandler.h"
#include "StringConstants.h"

#include "mapObjects/CGHeroInstance.h"
#include "BattleState.h"
#include "CBattleCallback.h"

#include "SpellMechanics.h"



/*
 * CSpellHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace SpellConfig
{
	static const std::string LEVEL_NAMES[] = {"none", "basic", "advanced", "expert"};

}

namespace SRSLPraserHelpers
{
	static int XYToHex(int x, int y)
	{
		return x + 17 * y;
	}

	static int XYToHex(std::pair<int, int> xy)
	{
		return XYToHex(xy.first, xy.second);
	}

	static int hexToY(int battleFieldPosition)
	{
		return battleFieldPosition/17;
	}

	static int hexToX(int battleFieldPosition)
	{
		int pos = battleFieldPosition - hexToY(battleFieldPosition) * 17;
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
		return xy.first >=0 && xy.first < 17 && xy.second >= 0 && xy.second < 11;
	}

	//helper function for std::set<ui16> CSpell::rangeInHexes(unsigned int centralHex, ui8 schoolLvl ) const
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

///CSpell::LevelInfo
CSpell::LevelInfo::LevelInfo()
	:description(""),cost(0),power(0),AIValue(0),smartTarget(true),range("0")
{

}

CSpell::LevelInfo::~LevelInfo()
{

}

///CSpell
CSpell::CSpell():
	id(SpellID::NONE), level(0),
	earth(false), water(false), fire(false), air(false),
	combatSpell(false), creatureAbility(false),
	positiveness(ESpellPositiveness::NEUTRAL),
	mainEffectAnim(-1),
	defaultProbability(0),
	isRising(false), isDamage(false), isOffensive(false),
	targetType(ETargetType::NO_TARGET),
	mechanics(nullptr)
{
	levels.resize(GameConstants::SPELL_SCHOOL_LEVELS);
}

CSpell::~CSpell()
{
	delete mechanics;
}

const CSpell::LevelInfo & CSpell::getLevelInfo(const int level) const
{
	if(level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->errorStream() << __FUNCTION__ << " invalid school level " << level;
		throw new std::runtime_error("Invalid school level");
	}

	return levels.at(level);
}

ui32 CSpell::calculateBonus(ui32 baseDamage, const CGHeroInstance* caster, const CStack* affectedCreature) const
{
	ui32 ret = baseDamage;
	//applying sorcery secondary skill
	if(caster)
	{
		ret *= (100.0 + caster->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::SORCERY)) / 100.0;
		ret *= (100.0 + caster->valOfBonuses(Bonus::SPELL_DAMAGE) + caster->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, id.toEnum())) / 100.0;
		
		for(const SpellSchoolInfo & cnf : spellSchoolConfig)
		{
			if(school.at(cnf.id))
			{
				ret *= (100.0 + caster->valOfBonuses(cnf.damagePremyBonus)) / 100.0;
				break; //only bonus from one school is used
			}				
		}		

		if (affectedCreature && affectedCreature->getCreature()->level) //Hero specials like Solmyr, Deemer
			ret *= (100. + ((caster->valOfBonuses(Bonus::SPECIAL_SPELL_LEV, id.toEnum()) * caster->level) / affectedCreature->getCreature()->level)) / 100.0;
	}
	return ret;	
}

ui32 CSpell::calculateDamage(const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower) const
{
	ui32 ret = 0; //value to return

	//check if spell really does damage - if not, return 0
	if(!isDamageSpell())
		return 0;

	ret = usedSpellPower * power;
	ret += getPower(spellSchoolLevel);

	//affected creature-specific part
	if(nullptr != affectedCreature)
	{
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		
		for(const SpellSchoolInfo & cnf : spellSchoolConfig)
		{
			if(school.at(cnf.id) && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id))
			{
				ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id);
				ret /= 100;
				break; //only bonus from one school is used
			}				
		}		
		
		//general spell dmg reduction
		//FIXME?
		if(air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, -1))
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, -1);
			ret /= 100;
		}
		//dmg increasing
		if(affectedCreature->hasBonusOfType(Bonus::MORE_DAMAGE_FROM_SPELL, id))
		{
			ret *= 100 + affectedCreature->valOfBonuses(Bonus::MORE_DAMAGE_FROM_SPELL, id.toEnum());
			ret /= 100;
		}
	}
	ret = calculateBonus(ret, caster, affectedCreature);
	return ret;	
}


ui32 CSpell::calculateHealedHP(const CGHeroInstance* caster, const CStack* stack, const CStack* sacrificedStack) const
{
//todo: use Mechanics class
	int healedHealth;
	
	if(!isHealingSpell())
	{
		logGlobal->errorStream() << "calculateHealedHP called for nonhealing spell "<< name;
		return 0;
	}		
	
	const int spellPowerSkill = caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER);
	const int levelPower = getPower(caster->getSpellSchoolLevel(this));
	
	if (id == SpellID::SACRIFICE && sacrificedStack)
		healedHealth = (spellPowerSkill + sacrificedStack->MaxHealth() + levelPower) * sacrificedStack->count;
	else
		healedHealth = spellPowerSkill * power + levelPower; //???
	healedHealth = calculateBonus(healedHealth, caster, stack);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (isRisingSpell() ? stack->baseAmount * stack->MaxHealth() : 0));	
}


std::vector<BattleHex> CSpell::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes) const
{
	using namespace SRSLPraserHelpers;

	std::vector<BattleHex> ret;

	if(id == SpellID::FIRE_WALL  ||  id == SpellID::FORCE_FIELD)
	{
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


	std::string rng = getLevelInfo(schoolLvl).range + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 1 && rng[0] != 'X') //there is at lest one hex in range
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

std::set<const CStack* > CSpell::getAffectedStacks(const CBattleInfoCallback * cb, ECastingMode::ECastingMode mode, PlayerColor casterColor, int spellLvl, BattleHex destination, const CGHeroInstance * caster) const
{
	std::set<const CStack* > attackedCres;//std::set to exclude multiple occurrences of two hex creatures
	
	const ui8 attackerSide = cb->playerToSide(casterColor) == 1;
	const auto attackedHexes = rangeInHexes(destination, spellLvl, attackerSide);
	
	const CSpell::TargetInfo ti(this, spellLvl, mode);
	
	
	//TODO: more generic solution for mass spells
	if (id == SpellID::CHAIN_LIGHTNING)
	{
		std::set<BattleHex> possibleHexes;
		for (auto stack : cb->battleGetAllStacks())
		{
			if (stack->isValidTarget())
			{
				for (auto hex : stack->getHexes())
				{
					possibleHexes.insert (hex);
				}
			}
		}
		int targetsOnLevel[4] = {4, 4, 5, 5};

		BattleHex lightningHex =  destination;
		for (int i = 0; i < targetsOnLevel[spellLvl]; ++i)
		{
			auto stack = cb->battleGetStackByPos (lightningHex, true);
			if (!stack)
				break;
			attackedCres.insert (stack);
			for (auto hex : stack->getHexes())
			{
				possibleHexes.erase (hex); //can't hit same place twice
			}
			if (possibleHexes.empty()) //not enough targets
				break;
			lightningHex = BattleHex::getClosestTile (stack->attackerOwned, destination, possibleHexes);
		}
	}
	else if (getLevelInfo(spellLvl).range.size() > 1) //custom many-hex range
	{
		for(BattleHex hex : attackedHexes)
		{
			if(const CStack * st = cb->battleGetStackByPos(hex, ti.onlyAlive))
			{
				attackedCres.insert(st);
			}
		}
	}
	else if(getTargetType() == CSpell::CREATURE)
	{
		auto predicate = [=](const CStack * s){
			const bool positiveToAlly = isPositive() && s->owner == casterColor;
			const bool negativeToEnemy = isNegative() && s->owner != casterColor;
			const bool validTarget = s->isValidTarget(!ti.onlyAlive); //todo: this should be handled by spell class
	
			//for single target spells select stacks covering destination tile
			const bool rangeCovers = ti.massive || s->coversPos(destination);
			//handle smart targeting
			const bool positivenessFlag = !ti.smart || isNeutral() || positiveToAlly || negativeToEnemy;
			
			return rangeCovers  && positivenessFlag && validTarget;		
		};
		
		TStacks stacks = cb->battleGetStacksIf(predicate);
		
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
			if(const CStack * st = cb->battleGetStackByPos(hex, ti.onlyAlive))
				attackedCres.insert(st);
		}
	}	
	
	//now handle immunities		
	auto predicate = [&, this](const CStack * s)->bool
	{
		bool hitDirectly = ti.alwaysHitDirectly && s->coversPos(destination);
		bool notImmune = (ESpellCastProblem::OK == isImmuneByStack(caster, s));
		
		return !(hitDirectly || notImmune);  
	};
	
	vstd::erase_if(attackedCres, predicate);
	
	return attackedCres;
}


CSpell::ETargetType CSpell::getTargetType() const
{
	return targetType;
}

CSpell::TargetInfo CSpell::getTargetInfo(const int level) const
{
	TargetInfo info(this, level);
	return info;
}

bool CSpell::isCombatSpell() const
{
	return combatSpell;
}

bool CSpell::isAdventureSpell() const
{
	return !combatSpell;
}

bool CSpell::isCreatureAbility() const
{
	return creatureAbility;
}

bool CSpell::isPositive() const
{
	return positiveness == POSITIVE;
}

bool CSpell::isNegative() const
{
	return positiveness == NEGATIVE;
}

bool CSpell::isNeutral() const
{
	return positiveness == NEUTRAL;
}

bool CSpell::isHealingSpell() const
{
	return isRisingSpell() || (id == SpellID::CURE);
}

bool CSpell::isRisingSpell() const
{
	return isRising;
}

bool CSpell::isDamageSpell() const
{
	return isDamage;
}

bool CSpell::isOffensiveSpell() const
{
	return isOffensive;
}

bool CSpell::isSpecialSpell() const
{
	return isSpecial;
}

bool CSpell::hasEffects() const
{
	return !levels[0].effects.empty();
}

const std::string& CSpell::getIconImmune() const
{
	return iconImmune;
}

const std::string& CSpell::getCastSound() const
{
	return castSound;
}



si32 CSpell::getCost(const int skillLevel) const
{
	return getLevelInfo(skillLevel).cost;
}

si32 CSpell::getPower(const int skillLevel) const
{
	return getLevelInfo(skillLevel).power;
}

//si32 CSpell::calculatePower(const int skillLevel) const
//{
//    return power + getPower(skillLevel);
//}

si32 CSpell::getProbability(const TFaction factionId) const
{
	if(!vstd::contains(probabilities,factionId))
	{
		return defaultProbability;
	}
	return probabilities.at(factionId);
}


void CSpell::getEffects(std::vector<Bonus>& lst, const int level) const
{
	if(level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->errorStream() << __FUNCTION__ << " invalid school level " << level;
		return;
	}

	const std::vector<Bonus> & effects = levels[level].effects;

	if(effects.empty())
	{
		logGlobal->errorStream() << __FUNCTION__ << " This spell ("  + name + ") has no effects for level " << level;
		return;
	}

	lst.reserve(lst.size() + effects.size());

	for(const Bonus & b : effects)
	{
		lst.push_back(Bonus(b));
	}
}

ESpellCastProblem::ESpellCastProblem CSpell::isImmuneBy(const IBonusBearer* obj) const
{	
	//todo: use new bonus API
	//1. Check absolute limiters
	for(auto b : absoluteLimiters)
	{
		if (!obj->hasBonusOfType(b))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	//2. Check absolute immunities
	for(auto b : absoluteImmunities)
	{
		if (obj->hasBonusOfType(b))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	
	//check receptivity
	if (isPositive() && obj->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
		return ESpellCastProblem::OK;	

	//3. Check negation
	//FIXME: Orb of vulnerability mechanics is not such trivial
	if(obj->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES)) //Orb of vulnerability
		return ESpellCastProblem::NOT_DECIDED;
		
	//4. Check negatable limit
	for(auto b : limiters)
	{
		if (!obj->hasBonusOfType(b))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}


	//5. Check negatable immunities
	for(auto b : immunities)
	{
		if (obj->hasBonusOfType(b))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	auto battleTestElementalImmunity = [&,this](Bonus::BonusType element) -> bool
	{
		if(obj->hasBonusOfType(element, 0)) //always resist if immune to all spells altogether
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		else if(!isPositive()) //negative or indifferent
		{
			if((isDamageSpell() && obj->hasBonusOfType(element, 2)) || obj->hasBonusOfType(element, 1))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		return ESpellCastProblem::NOT_DECIDED;
	};

	//6. Check elemental immunities
	
	for(const SpellSchoolInfo & cnf : spellSchoolConfig)
	{
		if(school.at(cnf.id))
			if(battleTestElementalImmunity(cnf.immunityBonus))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}


	TBonusListPtr levelImmunities = obj->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));

	if(obj->hasBonusOfType(Bonus::SPELL_IMMUNITY, id)
		|| ( levelImmunities->size() > 0  &&  levelImmunities->totalValue() >= level  &&  level))
	{
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	return ESpellCastProblem::NOT_DECIDED;
}

ESpellCastProblem::ESpellCastProblem CSpell::isImmuneByStack(const CGHeroInstance* caster, const CStack* obj) const
{
	const auto immuneResult = mechanics->isImmuneByStack(caster,obj);
	
	if (ESpellCastProblem::NOT_DECIDED != immuneResult) 
		return immuneResult;
	return ESpellCastProblem::OK;	
}


void CSpell::setIsOffensive(const bool val)
{
	isOffensive = val;

	if(val)
	{
		positiveness = CSpell::NEGATIVE;
		isDamage = true;
	}
}

void CSpell::setIsRising(const bool val)
{
	isRising = val;

	if(val)
	{
		positiveness = CSpell::POSITIVE;
	}
}

void CSpell::setup()
{
	setupMechanics();
	
	school[ESpellSchool::AIR] = air;
	school[ESpellSchool::FIRE] = fire;
	school[ESpellSchool::WATER] = water;
	school[ESpellSchool::EARTH] = earth;	
}


void CSpell::setupMechanics()
{
	if(nullptr != mechanics)
	{
		logGlobal->errorStream() << "Spell " << this->name << " mechanics already set";
		delete mechanics;
		mechanics = nullptr;	
	}
	
	switch (id)
	{
	case SpellID::CLONE:
		mechanics = new CloneMechanics(this);
		break;
	case SpellID::DISPEL_HELPFUL_SPELLS:
		mechanics = new DispellHelpfulMechanics(this);
		break;
	case SpellID::SACRIFICE:
		mechanics = new SacrificeMechanics(this);
		break;
	case SpellID::CHAIN_LIGHTNING:
		mechanics = new ChainLightningMechanics(this);
		break;		
	default:		
		if(isRisingSpell())
			mechanics = new SpecialRisingSpellMechanics(this);
		else	
			mechanics = new DefaultSpellMechanics(this);
		break;
	}
	
}

///CSpell::TargetInfo
CSpell::TargetInfo::TargetInfo(const CSpell * spell, const int level)
{
	init(spell, level);
}

CSpell::TargetInfo::TargetInfo(const CSpell * spell, const int level, ECastingMode::ECastingMode mode)
{
	init(spell, level);
	if(mode == ECastingMode::ENCHANTER_CASTING)
	{
		smart = true; //FIXME: not sure about that, this makes all spells smart in this mode
		massive = true;
	}
	else if(mode == ECastingMode::SPELL_LIKE_ATTACK)
	{
		alwaysHitDirectly = true;
	}	
}

void CSpell::TargetInfo::init(const CSpell * spell, const int level)
{
	auto & levelInfo = spell->getLevelInfo(level);

	type = spell->getTargetType();
	smart = levelInfo.smartTarget;
	massive = levelInfo.range == "X";
	onlyAlive = !spell->isRisingSpell();
	alwaysHitDirectly = false;	
}


bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos)
{
	int3 diff = pos - center;
	if(diff.x >= -9  &&  diff.x <= 9  &&  diff.y >= -8  &&  diff.y <= 8)
		return true;
	else
		return false;
}

///CSpellHandler
CSpellHandler::CSpellHandler()
{

}

std::vector<JsonNode> CSpellHandler::loadLegacyData(size_t dataSize)
{
	using namespace SpellConfig;
	std::vector<JsonNode> legacyData;

	CLegacyConfigParser parser("DATA/SPTRAITS.TXT");

	auto readSchool = [&](JsonMap& schools, const std::string& name)
	{
		if (parser.readString() == "x")
		{
			schools[name].Bool() = true;
		}
	};

	auto read = [&,this](bool combat, bool ability)
	{
		do
		{
			JsonNode lineNode(JsonNode::DATA_STRUCT);

			const si32 id = legacyData.size();

			lineNode["index"].Float() = id;
			lineNode["type"].String() = ability ? "ability" : (combat ? "combat" : "adventure");

			lineNode["name"].String() = parser.readString();

			parser.readString(); //ignored unused abbreviated name
			lineNode["level"].Float()      = parser.readNumber();

			auto& schools = lineNode["school"].Struct();

			readSchool(schools, "earth");
			readSchool(schools, "water");
			readSchool(schools, "fire");
			readSchool(schools, "air");

			auto& levels = lineNode["levels"].Struct();

			auto getLevel = [&](const size_t idx)->JsonMap&
			{
				assert(idx < GameConstants::SPELL_SCHOOL_LEVELS);
				return levels[LEVEL_NAMES[idx]].Struct();
			};

			auto costs = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);
			lineNode["power"].Float() = parser.readNumber();
			auto powers = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			auto& chances = lineNode["gainChance"].Struct();

			for(size_t i = 0; i < GameConstants::F_NUMBER ; i++){
				chances[ETownType::names[i]].Float() = parser.readNumber();
			}

			auto AIVals = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			std::vector<std::string> descriptions;
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS ; i++)
				descriptions.push_back(parser.readString());

			std::string attributes = parser.readString();

			std::string targetType = "NO_TARGET";

			if(attributes.find("CREATURE_TARGET_1") != std::string::npos
				|| attributes.find("CREATURE_TARGET_2") != std::string::npos)
				targetType = "CREATURE_EXPERT_MASSIVE";
			else if(attributes.find("CREATURE_TARGET") != std::string::npos)
				targetType = "CREATURE";
			else if(attributes.find("OBSTACLE_TARGET") != std::string::npos)
				targetType = "OBSTACLE";

			//save parsed level specific data
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
			{
				auto& level = getLevel(i);
				level["description"].String() = descriptions[i];
				level["cost"].Float() = costs[i];
				level["power"].Float() = powers[i];
				level["aiValue"].Float() = AIVals[i];
			}

			if(targetType == "CREATURE_EXPERT_MASSIVE")
			{
				lineNode["targetType"].String() = "CREATURE";
				getLevel(3)["range"].String() = "X";
			}
			else
			{
				lineNode["targetType"].String() = targetType;
			}

			legacyData.push_back(lineNode);


		}
		while (parser.endLine() && !parser.isNextEntryEmpty());
	};

	auto skip = [&](int cnt)
	{
		for(int i=0; i<cnt; i++)
			parser.endLine();
	};

	skip(5);// header
	read(false,false); //read adventure map spells
	skip(3);
	read(true,false); //read battle spells
	skip(3);
	read(true,true);//read creature abilities

    //TODO: maybe move to config
	//clone Acid Breath attributes for Acid Breath damage effect
	JsonNode temp = legacyData[SpellID::ACID_BREATH_DEFENSE];
	temp["index"].Float() = SpellID::ACID_BREATH_DAMAGE;
	legacyData.push_back(temp);

	objects.resize(legacyData.size());

	return legacyData;
}

const std::string CSpellHandler::getTypeName() const
{
	return "spell";
}

CSpell * CSpellHandler::loadFromJson(const JsonNode& json)
{
	using namespace SpellConfig;

	CSpell * spell = new CSpell();

	const auto type = json["type"].String();

	if(type == "ability")
	{
		spell->creatureAbility = true;
		spell->combatSpell = true;
	}
	else
	{
		spell->creatureAbility = false;
		spell->combatSpell = type == "combat";
	}

	spell->name = json["name"].String();

	logGlobal->traceStream() << __FUNCTION__ << ": loading spell " << spell->name;

	const auto schoolNames = json["school"];

	spell->air   = schoolNames["air"].Bool();
	spell->earth = schoolNames["earth"].Bool();
	spell->fire  = schoolNames["fire"].Bool();
	spell->water = schoolNames["water"].Bool();

	spell->level = json["level"].Float();
	spell->power = json["power"].Float();

	spell->defaultProbability = json["defaultGainChance"].Float();

	for(const auto & node : json["gainChance"].Struct())
	{
		const int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier(node.second.meta, "faction",node.first, [=](si32 factionID)
		{
			spell->probabilities[factionID] = chance;
		});
	}

	const std::string targetType = json["targetType"].String();

	if(targetType == "NO_TARGET")
		spell->targetType = CSpell::NO_TARGET;
	else if(targetType == "CREATURE")
		spell->targetType = CSpell::CREATURE;
	else if(targetType == "OBSTACLE")
		spell->targetType = CSpell::OBSTACLE;
	else 
		logGlobal->warnStream() << "Spell " << spell->name << ". Target type " << (targetType.empty() ? "empty" : "unknown ("+targetType+")") << ". Assumed NO_TARGET.";

	spell->mainEffectAnim = json["anim"].Float();

	for(const auto & counteredSpell: json["counters"].Struct())
		if (counteredSpell.second.Bool())
		{
			VLC->modh->identifiers.requestIdentifier(json.meta, counteredSpell.first, [=](si32 id)
			{
				spell->counteredSpells.push_back(SpellID(id));
			});
		}

	//TODO: more error checking - f.e. conflicting flags
	const auto flags = json["flags"];

	//by default all flags are set to false in constructor

	spell->isDamage = flags["damage"].Bool(); //do this before "offensive"

	if(flags["offensive"].Bool())
	{
		spell->setIsOffensive(true);
	}

	if(flags["rising"].Bool())
	{
		spell->setIsRising(true);
	}

	const bool implicitPositiveness = spell->isOffensive || spell->isRising; //(!) "damage" does not mean NEGATIVE  --AVS

	if(flags["indifferent"].Bool())
	{
		spell->positiveness = CSpell::NEUTRAL;
	}
	else if(flags["negative"].Bool())
	{
		spell->positiveness = CSpell::NEGATIVE;
	}
	else if(flags["positive"].Bool())
	{
		spell->positiveness = CSpell::POSITIVE;
	}
	else if(!implicitPositiveness)
	{
		spell->positiveness = CSpell::NEUTRAL; //duplicates constructor but, just in case
		logGlobal->warnStream() << "Spell " << spell->name << ": no positiveness specified, assumed NEUTRAL";
	}

	spell->isSpecial = flags["special"].Bool();

	auto findBonus = [&](const std::string & name, std::vector<Bonus::BonusType> &vec)
	{
		auto it = bonusNameMap.find(name);
		if(it == bonusNameMap.end())
		{
			logGlobal->errorStream() << spell->name << ": invalid bonus name" << name;
		}
		else
		{
			vec.push_back((Bonus::BonusType)it->second);
		}
	};

	auto readBonusStruct = [&](const std::string & name, std::vector<Bonus::BonusType> &vec)
	{
		for(auto bonusData: json[name].Struct())
		{
			const std::string bonusId = bonusData.first;
			const bool flag = bonusData.second.Bool();

			if(flag)
				findBonus(bonusId, vec);
		}
	};

	readBonusStruct("immunity", spell->immunities);
	readBonusStruct("absoluteImmunity", spell->absoluteImmunities);
	readBonusStruct("limit", spell->limiters);	
	readBonusStruct("absoluteLimit", spell->absoluteLimiters);

	const JsonNode & graphicsNode = json["graphics"];

	spell->iconImmune =        graphicsNode["iconImmune"].String();
	spell->iconBook =          graphicsNode["iconBook"].String();
	spell->iconEffect =        graphicsNode["iconEffect"].String();
	spell->iconScenarioBonus = graphicsNode["iconScenarioBonus"].String();
	spell->iconScroll =        graphicsNode["iconScroll"].String();

	const JsonNode & soundsNode = json["sounds"];

	spell->castSound = soundsNode["cast"].String();

	//load level attributes
	const int levelsCount = GameConstants::SPELL_SCHOOL_LEVELS;

	for(int levelIndex = 0; levelIndex < levelsCount; levelIndex++)
	{
		const JsonNode & levelNode = json["levels"][LEVEL_NAMES[levelIndex]];
		
		CSpell::LevelInfo & levelObject = spell->levels[levelIndex];

		const si32 levelPower   = levelNode["power"].Float();		
		
		levelObject.description = levelNode["description"].String();
		levelObject.cost        = levelNode["cost"].Float();
		levelObject.power       = levelPower;
		levelObject.AIValue     = levelNode["aiValue"].Float();
		levelObject.smartTarget = levelNode["targetModifier"]["smart"].Bool();
		levelObject.range       = levelNode["range"].String();
		
		for(const auto & elem : levelNode["effects"].Struct())
		{
			const JsonNode & bonusNode = elem.second;
			Bonus * b = JsonUtils::parseBonus(bonusNode);
			const bool usePowerAsValue = bonusNode["val"].isNull();

			//TODO: make this work. see CSpellHandler::afterLoadFinalization()
			//b->sid = spell->id; //for all

			b->source = Bonus::SPELL_EFFECT;//for all

			if(usePowerAsValue)
				b->val = levelPower;

			levelObject.effects.push_back(*b);
		}
		
	}

	return spell;
}

void CSpellHandler::afterLoadFinalization()
{
	//FIXME: it is a bad place for this code, should refactor loadFromJson to know object id during loading
	for(auto spell: objects)
	{
		for(auto & level: spell->levels)
			for(auto & bonus: level.effects)
				bonus.sid = spell->id;
		spell->setup();
	}
}

void CSpellHandler::beforeValidate(JsonNode & object)
{
	//handle "base" level info
	
	JsonNode& levels = object["levels"];
	JsonNode& base = levels["base"];
	
	auto inheritNode = [&](const std::string & name){
		JsonUtils::inherit(levels[name],base);
	};
	
	inheritNode("none");
	inheritNode("basic");
	inheritNode("advanced");
	inheritNode("expert");
}


CSpellHandler::~CSpellHandler()
{

}

std::vector<bool> CSpellHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedSpells;
	allowedSpells.resize(GameConstants::SPELLS_QUANTITY, true);
	return allowedSpells;
}
