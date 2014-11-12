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


///CSpellMechanics
CSpellMechanics::CSpellMechanics(CSpell * s):
	owner(s)
{
	
}

CSpellMechanics::~CSpellMechanics()
{
	
}

ESpellCastProblem::ESpellCastProblem CSpellMechanics::isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj)
{
	//by default use general algorithm
	return owner->isImmuneBy(obj);
}

namespace
{
	class CloneMechnics: public CSpellMechanics
	{
	public:
		CloneMechnics(CSpell * s): CSpellMechanics(s){};
		ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;
	};
	
	class DispellHelpfulMechanics: public CSpellMechanics
	{
	public:
		DispellHelpfulMechanics(CSpell * s): CSpellMechanics(s){};
		ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;	
	};
	
	class HypnotizeMechanics: public CSpellMechanics
	{
	public:
		HypnotizeMechanics(CSpell * s): CSpellMechanics(s){};	
		ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;	
	};
	
	///all rising spells
	class RisingSpellMechanics: public CSpellMechanics
	{
	public:
		RisingSpellMechanics(CSpell * s): CSpellMechanics(s){};		
		
	};
	
	///all rising spells but SACRIFICE
	class SpecialRisingSpellMechanics: public RisingSpellMechanics
	{
	public:
		SpecialRisingSpellMechanics(CSpell * s): RisingSpellMechanics(s){};
		ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, ECastingMode::ECastingMode mode, const CStack * obj) override;						
	};
	
	class SacrificeMechanics: public RisingSpellMechanics
	{
	public:
		SacrificeMechanics(CSpell * s): RisingSpellMechanics(s){};		
	};
	
	///CloneMechanics
	ESpellCastProblem::ESpellCastProblem CloneMechnics::isImmuneByStack(const CGHeroInstance* caster, ECastingMode::ECastingMode mode, const CStack * obj)
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
		return CSpellMechanics::isImmuneByStack(caster,mode,obj);	
	}
	
	///DispellHelpfulMechanics
	ESpellCastProblem::ESpellCastProblem DispellHelpfulMechanics::isImmuneByStack(const CGHeroInstance* caster, ECastingMode::ECastingMode mode, const CStack* obj)
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
		return CSpellMechanics::isImmuneByStack(caster,mode,obj);	
	}
	
	///HypnotizeMechanics
	ESpellCastProblem::ESpellCastProblem HypnotizeMechanics::isImmuneByStack(const CGHeroInstance* caster, ECastingMode::ECastingMode mode, const CStack* obj)
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
		return CSpellMechanics::isImmuneByStack(caster,mode,obj);
	}
	
	
	///SpecialRisingSpellMechanics
	ESpellCastProblem::ESpellCastProblem SpecialRisingSpellMechanics::isImmuneByStack(const CGHeroInstance* caster, ECastingMode::ECastingMode mode, const CStack* obj)
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
		
		return CSpellMechanics::isImmuneByStack(caster,mode,obj);	
	}
	
	
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

		if(air)
			ret *= (100.0 + caster->valOfBonuses(Bonus::AIR_SPELL_DMG_PREMY)) / 100.0;
		else if(fire) //only one type of bonus for Magic Arrow
			ret *= (100.0 + caster->valOfBonuses(Bonus::FIRE_SPELL_DMG_PREMY)) / 100.0;
		else if(water)
			ret *= (100.0 + caster->valOfBonuses(Bonus::WATER_SPELL_DMG_PREMY)) / 100.0;
		else if(earth)
			ret *= (100.0 + caster->valOfBonuses(Bonus::EARTH_SPELL_DMG_PREMY)) / 100.0;

		if (affectedCreature && affectedCreature->getCreature()->level) //Hero specials like Solmyr, Deemer
			ret *= (100. + ((caster->valOfBonuses(Bonus::SPECIAL_SPELL_LEV, id.toEnum()) * caster->level) / affectedCreature->getCreature()->level)) / 100.0;
	}
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

CSpell::ETargetType CSpell::getTargetType() const
{
	return targetType;
}

const CSpell::TargetInfo CSpell::getTargetInfo(const int level) const
{
	TargetInfo info;

	auto & levelInfo = getLevelInfo(level);

	info.type = getTargetType();
	info.smart = levelInfo.smartTarget;
	info.massive = levelInfo.range == "X";
	info.onlyAlive = !isRisingSpell();

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
	//0. check receptivity
	if (isPositive() && obj->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
		return ESpellCastProblem::OK;	
	
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
	if(fire)
	{
		if(battleTestElementalImmunity(Bonus::FIRE_IMMUNITY))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	if(water)
	{
		if(battleTestElementalImmunity(Bonus::WATER_IMMUNITY))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	if(earth)
	{
		if(battleTestElementalImmunity(Bonus::EARTH_IMMUNITY))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}
	if(air)
	{
		if(battleTestElementalImmunity(Bonus::AIR_IMMUNITY))
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

ESpellCastProblem::ESpellCastProblem CSpell::isImmuneByStack(const CGHeroInstance* caster, ECastingMode::ECastingMode mode, const CStack* obj) const
{
	const auto immuneResult = mechanics->isImmuneByStack(caster,mode,obj);
	
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
		mechanics = new CloneMechnics(this);
		break;
	case SpellID::DISPEL_HELPFUL_SPELLS:
		mechanics = new DispellHelpfulMechanics(this);
		break;
	case SpellID::SACRIFICE:
		mechanics = new SacrificeMechanics(this);
		break;
	default:		
		if(isRisingSpell())
			mechanics = new SpecialRisingSpellMechanics(this);
		else	
			mechanics = new CSpellMechanics(this);
		break;
	}
	
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
		spell->setupMechanics();
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
