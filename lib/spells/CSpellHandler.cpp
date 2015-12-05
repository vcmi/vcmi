/*
 * CSpellHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include <cctype>

#include "CSpellHandler.h"

#include "../CGeneralTextHandler.h"
#include "../filesystem/Filesystem.h"

#include "../JsonNode.h"

#include "../BattleHex.h"
#include "../CModHandler.h"
#include "../StringConstants.h"

#include "../BattleState.h"
#include "../CBattleCallback.h"
#include "../CGameState.h" //todo: remove

#include "../NetPacks.h" //todo: remove

#include "ISpellMechanics.h"

namespace SpellConfig
{
	static const std::string LEVEL_NAMES[] = {"none", "basic", "advanced", "expert"};

	static const SpellSchoolInfo SCHOOL[4] =
	{
		{
			ESpellSchool::AIR,
			Bonus::AIR_SPELL_DMG_PREMY,
			Bonus::AIR_IMMUNITY,
			"air",
			SecondarySkill::AIR_MAGIC,
			Bonus::AIR_SPELLS
		},
		{
			ESpellSchool::FIRE,
			Bonus::FIRE_SPELL_DMG_PREMY,
			Bonus::FIRE_IMMUNITY,
			"fire",
			SecondarySkill::FIRE_MAGIC,
			Bonus::FIRE_SPELLS
		},
		{
			ESpellSchool::WATER,
			Bonus::WATER_SPELL_DMG_PREMY,
			Bonus::WATER_IMMUNITY,
			"water",
			SecondarySkill::WATER_MAGIC,
			Bonus::WATER_SPELLS
		},
		{
			ESpellSchool::EARTH,
			Bonus::EARTH_SPELL_DMG_PREMY,
			Bonus::EARTH_IMMUNITY,
			"earth",
			SecondarySkill::EARTH_MAGIC,
			Bonus::EARTH_SPELLS
		}
	};
}

///CSpell::LevelInfo
CSpell::LevelInfo::LevelInfo()
	:description(""),cost(0),power(0),AIValue(0),smartTarget(true), clearTarget(false), clearAffected(false), range("0")
{

}

CSpell::LevelInfo::~LevelInfo()
{

}

///CSpell
CSpell::CSpell():
	id(SpellID::NONE), level(0),
	combatSpell(false), creatureAbility(false),
	positiveness(ESpellPositiveness::NEUTRAL),
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

void CSpell::applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const
{
	mechanics->applyBattle(battle, packet);
}

bool CSpell::adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
	assert(env);

	return mechanics->adventureCast(env, parameters);
}

void CSpell::battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const
{	
	assert(env);
	if(parameters.destinations.size()<1)
	{
		env->complain("Spell must have at least one destination");
		return;
	}
	mechanics->battleCast(env, parameters);
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

ui32 CSpell::calculateDamage(const ISpellCaster * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower) const
{
	//check if spell really does damage - if not, return 0
	if(!isDamageSpell())
		return 0;
	return adjustRawDamage(caster, affectedCreature, calculateRawEffectValue(spellSchoolLevel, usedSpellPower));	
}

ESpellCastProblem::ESpellCastProblem CSpell::canBeCast(const CBattleInfoCallback * cb, PlayerColor player) const
{
	return mechanics->canBeCast(cb, player);
}

std::vector<BattleHex> CSpell::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes) const
{
	return mechanics->rangeInHexes(centralHex,schoolLvl,side,outDroppedHexes);
}

std::set<const CStack *> CSpell::getAffectedStacks(const CBattleInfoCallback * cb, ECastingMode::ECastingMode mode, PlayerColor casterColor, int spellLvl, BattleHex destination, const ISpellCaster * caster) const
{
	ISpellMechanics::SpellTargetingContext ctx(this, cb,mode,casterColor,spellLvl,destination);

	std::set<const CStack* > attackedCres = mechanics->getAffectedStacks(ctx);

	//now handle immunities
	auto predicate = [&, this](const CStack * s)->bool
	{
		bool hitDirectly = ctx.ti.alwaysHitDirectly && s->coversPos(destination);
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

void CSpell::forEachSchool(const std::function<void(const SpellSchoolInfo &, bool &)>& cb) const
{
	bool stop = false;
	for(const SpellSchoolInfo & cnf : SpellConfig::SCHOOL)
	{
		if(school.at(cnf.id))
		{
			cb(cnf, stop);

			if(stop)
				break;
		}
	}
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

const std::string & CSpell::getIconImmune() const
{
	return iconImmune;
}

const std::string & CSpell::getCastSound() const
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

si32 CSpell::getProbability(const TFaction factionId) const
{
	if(!vstd::contains(probabilities,factionId))
	{
		return defaultProbability;
	}
	return probabilities.at(factionId);
}

void CSpell::getEffects(std::vector<Bonus> & lst, const int level) const
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

ESpellCastProblem::ESpellCastProblem CSpell::isImmuneAt(const CBattleInfoCallback * cb, const ISpellCaster * caster, ECastingMode::ECastingMode mode, BattleHex destination) const
{
	// Get all stacks at destination hex. only alive if not rising spell
	TStacks stacks = cb->battleGetStacksIf([=](const CStack * s){
		return s->coversPos(destination) && (isRisingSpell() || s->alive());
	});

	if(!stacks.empty())
	{
		bool allImmune = true;

		ESpellCastProblem::ESpellCastProblem problem = ESpellCastProblem::INVALID;

		for(auto s : stacks)
		{
			ESpellCastProblem::ESpellCastProblem res = isImmuneByStack(caster,s);

			if(res == ESpellCastProblem::OK)
			{
				allImmune = false;
			}
			else
			{
				problem = res;
			}
		}

		if(allImmune)
			return problem;
	}
	else //no target stack on this tile
	{
		if(getTargetType() == CSpell::CREATURE)
		{
			if(caster && mode == ECastingMode::HERO_CASTING) //TODO why???
			{
				const CSpell::TargetInfo ti(this, caster->getSpellSchoolLevel(this), mode);

				if(!ti.massive)
					return ESpellCastProblem::WRONG_SPELL_TARGET;
			}
			else
			{
 				return ESpellCastProblem::WRONG_SPELL_TARGET;
			}
		}
	}

	return ESpellCastProblem::OK;
}

int CSpell::adjustRawDamage(const ISpellCaster * caster, const CStack * affectedCreature, int rawDamage) const
{
	int ret = rawDamage;
	//affected creature-specific part
	if(nullptr != affectedCreature)
	{
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		forEachSchool([&](const SpellSchoolInfo & cnf, bool & stop)
		{
			if(affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id))
			{
				ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id);
				ret /= 100;
				stop = true;//only bonus from one school is used
			}
		});

		//general spell dmg reduction
		if(affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, -1))
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
	if(caster != nullptr)
		ret = caster->getSpellBonus(this, ret, affectedCreature);
	return ret;
}

int CSpell::calculateRawEffectValue(int effectLevel, int effectPower) const
{
	return effectPower * power + getPower(effectLevel);	
}

ESpellCastProblem::ESpellCastProblem CSpell::internalIsImmune(const ISpellCaster * caster, const CStack *obj) const
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

	{
		//spell-based spell immunity (only ANTIMAGIC in OH3) is treated as absolute
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::LEVEL_SPELL_IMMUNITY << "source_" << Bonus::SPELL_EFFECT;

		TBonusListPtr levelImmunitiesFromSpell = obj->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY).And(Selector::sourceType(Bonus::SPELL_EFFECT)), cachingStr.str());	

		if(levelImmunitiesFromSpell->size() > 0  &&  levelImmunitiesFromSpell->totalValue() >= level  &&  level)
		{
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
	}
	{
		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << id.toEnum() << "addInfo_1";
		if(obj->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, id.toEnum(), 1), cachingStr.str()))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}	

	//check receptivity
	if (isPositive() && obj->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
		return ESpellCastProblem::OK;
		
	//3. Check negation
	//Orb of vulnerability
	//FIXME: Orb of vulnerability mechanics is not such trivial (issue 1791)
	const bool battleWideNegation = obj->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 0);
	const bool heroNegation = obj->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 1);
	//anyone can cast on artifact holder`s stacks
	if(heroNegation) 
		return ESpellCastProblem::NOT_DECIDED;
	//this stack is from other player
	//todo: check that caster is always present (not trivial is this case)
	//todo: NEGATE_ALL_NATURAL_IMMUNITIES special cases: dispell, chain lightning
	else if(battleWideNegation && caster)
	{
		if(obj->owner != caster->getOwner())
			return ESpellCastProblem::NOT_DECIDED;
	}

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

	//6. Check elemental immunities

	ESpellCastProblem::ESpellCastProblem tmp = ESpellCastProblem::NOT_DECIDED;

	forEachSchool([&](const SpellSchoolInfo & cnf, bool & stop)
	{
		auto element = cnf.immunityBonus;

		if(obj->hasBonusOfType(element, 0)) //always resist if immune to all spells altogether
		{
			tmp = ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
			stop = true;
		}
		else if(!isPositive()) //negative or indifferent
		{
			if((isDamageSpell() && obj->hasBonusOfType(element, 2)) || obj->hasBonusOfType(element, 1))
			{
				tmp = ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
				stop = true;
			}
		}
	});

	if(tmp != ESpellCastProblem::NOT_DECIDED)
		return tmp;

	TBonusListPtr levelImmunities = obj->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));

	if(obj->hasBonusOfType(Bonus::SPELL_IMMUNITY, id)
		|| ( levelImmunities->size() > 0  &&  levelImmunities->totalValue() >= level  &&  level))
	{
		return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
	}

	return ESpellCastProblem::NOT_DECIDED;
}

ESpellCastProblem::ESpellCastProblem CSpell::isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const
{
	const auto immuneResult = mechanics->isImmuneByStack(caster,obj);

	if (ESpellCastProblem::NOT_DECIDED != immuneResult)
		return immuneResult;
	return ESpellCastProblem::OK;
}

void CSpell::prepareBattleLog(const CBattleInfoCallback * cb,  const BattleSpellCast * packet, std::vector<std::string> & logLines) const
{
	bool displayDamage = true;
	
	std::string casterName("Something"); //todo: localize
	
	if(packet->castByHero)
		casterName = cb->battleGetHeroInfo(packet->side).name;

	{
		const auto casterStackID = packet->casterStack;

		if(casterStackID > 0)
		{
			const CStack * casterStack = cb->battleGetStackByID(casterStackID);
			if(casterStack != nullptr)
				casterName = casterStack->type->namePl;
		}
	}
	
	if(packet->affectedCres.size() == 1)
	{
		const CStack * attackedStack = cb->battleGetStackByID(*packet->affectedCres.begin(), false);
		
		const std::string attackedNamePl = attackedStack->getCreature()->namePl;
		
		if(packet->castByHero)
		{
			const std::string fmt = VLC->generaltexth->allTexts[195];
			logLines.push_back(boost::to_string(boost::format(fmt) % casterName % this->name % attackedNamePl));
		}
		else
		{
			mechanics->battleLogSingleTarget(logLines, packet, casterName, attackedStack, displayDamage);		
		}
	}
	else
	{
		boost::format text(VLC->generaltexth->allTexts[196]);
		text % casterName % this->name;
		logLines.push_back(text.str());		
	}
	
	
	if(packet->dmgToDisplay > 0 && displayDamage)
	{
		boost::format dmgInfo(VLC->generaltexth->allTexts[376]);
		dmgInfo % this->name % packet->dmgToDisplay;
		logLines.push_back(dmgInfo.str());
	}
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
}

void CSpell::setupMechanics()
{
	if(nullptr != mechanics)
	{
		logGlobal->errorStream() << "Spell " << this->name << ": mechanics already set";
		delete mechanics;
	}

	mechanics = ISpellMechanics::createMechanics(this);
}

///CSpell::AnimationInfo
CSpell::AnimationItem::AnimationItem()
	:resourceName(""),verticalPosition(VerticalPosition::TOP),pause(0)
{
	
}


///CSpell::AnimationInfo
CSpell::AnimationInfo::AnimationInfo()
{

}

CSpell::AnimationInfo::~AnimationInfo()
{

}

std::string CSpell::AnimationInfo::selectProjectile(const double angle) const
{
	std::string res;
	double maximum = 0.0;

	for(const auto & info : projectile)
	{
		if(info.minimumAngle < angle && info.minimumAngle > maximum)
		{
			maximum = info.minimumAngle;
			res = info.resourceName;
		}
	}

	return res;
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
	clearAffected = levelInfo.clearAffected;
	clearTarget = levelInfo.clearTarget;
}

bool DLL_LINKAGE isInScreenRange(const int3 & center, const int3 & pos)
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

	auto readSchool = [&](JsonMap & schools, const std::string & name)
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

			for(size_t i = 0; i < GameConstants::F_NUMBER; i++){
				chances[ETownType::names[i]].Float() = parser.readNumber();
			}

			auto AIVals = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			std::vector<std::string> descriptions;
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
				descriptions.push_back(parser.readString());

			parser.readString(); //ignore attributes. All data present in JSON

			//save parsed level specific data
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
			{
				auto& level = getLevel(i);
				level["description"].String() = descriptions[i];
				level["cost"].Float() = costs[i];
				level["power"].Float() = powers[i];
				level["aiValue"].Float() = AIVals[i];
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

CSpell * CSpellHandler::loadFromJson(const JsonNode & json)
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

	for(const SpellSchoolInfo & info : SpellConfig::SCHOOL)
	{
		spell->school[info.id] = schoolNames[info.jsonName].Bool();
	}

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

	auto targetType = json["targetType"].String();

	if(targetType == "NO_TARGET")
		spell->targetType = CSpell::NO_TARGET;
	else if(targetType == "CREATURE")
		spell->targetType = CSpell::CREATURE;
	else if(targetType == "OBSTACLE")
		spell->targetType = CSpell::OBSTACLE;
	else if(targetType == "LOCATION")
		spell->targetType = CSpell::LOCATION;
	else
		logGlobal->warnStream() << "Spell " << spell->name << ": target type " << (targetType.empty() ? "empty" : "unknown ("+targetType+")") << ", assumed NO_TARGET.";

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
		logGlobal->errorStream() << "Spell " << spell->name << ": no positiveness specified, assumed NEUTRAL.";
	}

	spell->isSpecial = flags["special"].Bool();

	auto findBonus = [&](std::string name, std::vector<Bonus::BonusType> & vec)
	{
		auto it = bonusNameMap.find(name);
		if(it == bonusNameMap.end())
		{
			logGlobal->errorStream() << "Spell " << spell->name << ": invalid bonus name " << name;
		}
		else
		{
			vec.push_back((Bonus::BonusType)it->second);
		}
	};

	auto readBonusStruct = [&](std::string name, std::vector<Bonus::BonusType> & vec)
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

	spell->iconImmune = graphicsNode["iconImmune"].String();
	spell->iconBook = graphicsNode["iconBook"].String();
	spell->iconEffect = graphicsNode["iconEffect"].String();
	spell->iconScenarioBonus = graphicsNode["iconScenarioBonus"].String();
	spell->iconScroll = graphicsNode["iconScroll"].String();

	const JsonNode & animationNode = json["animation"];

	auto loadAnimationQueue = [&](const std::string & jsonName, CSpell::TAnimationQueue & q)
	{
		auto queueNode = animationNode[jsonName].Vector();
		for(const JsonNode & item : queueNode)
		{
			CSpell::TAnimation newItem;

			if(item.getType() == JsonNode::DATA_STRING)
				newItem.resourceName = item.String();
			else if(item.getType() == JsonNode::DATA_STRUCT)
			{
				newItem.resourceName = item["defName"].String();

				auto vPosStr = item["verticalPosition"].String();
				if("bottom" == vPosStr)
					newItem.verticalPosition = VerticalPosition::BOTTOM;
			}
			else if(item.getType() == JsonNode::DATA_FLOAT)
			{
				newItem.pause = item.Float();
			}
			
			q.push_back(newItem);
		}
	};

	loadAnimationQueue("affect", spell->animationInfo.affect);
	loadAnimationQueue("cast", spell->animationInfo.cast);
	loadAnimationQueue("hit", spell->animationInfo.hit);

	const JsonVector & projectile = animationNode["projectile"].Vector();

	for(const JsonNode & item : projectile)
	{
		CSpell::ProjectileInfo info;
		info.resourceName = item["defName"].String();
		info.minimumAngle = item["minimumAngle"].Float();

		spell->animationInfo.projectile.push_back(info);
	}

	const JsonNode & soundsNode = json["sounds"];
	spell->castSound = soundsNode["cast"].String();

	//load level attributes
	const int levelsCount = GameConstants::SPELL_SCHOOL_LEVELS;

	for(int levelIndex = 0; levelIndex < levelsCount; levelIndex++)
	{
		const JsonNode & levelNode = json["levels"][LEVEL_NAMES[levelIndex]];

		CSpell::LevelInfo & levelObject = spell->levels[levelIndex];

		const si32 levelPower     = levelObject.power = levelNode["power"].Float();

		levelObject.description   = levelNode["description"].String();
		levelObject.cost          = levelNode["cost"].Float();
		levelObject.AIValue       = levelNode["aiValue"].Float();
		levelObject.smartTarget   = levelNode["targetModifier"]["smart"].Bool();
		levelObject.clearTarget   = levelNode["targetModifier"]["clearTarget"].Bool();
		levelObject.clearAffected = levelNode["targetModifier"]["clearAffected"].Bool();
		levelObject.range         = levelNode["range"].String();

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

			levelObject.effectsTmp.push_back(b);
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
		{
			for(Bonus * bonus : level.effectsTmp)
			{
				level.effects.push_back(*bonus);
				delete bonus;
			}
			level.effectsTmp.clear();
			
			for(auto & bonus: level.effects)
				bonus.sid = spell->id;	
		}
		spell->setup();		
	}
}

void CSpellHandler::beforeValidate(JsonNode & object)
{
	//handle "base" level info

	JsonNode & levels = object["levels"];
	JsonNode & base = levels["base"];

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
