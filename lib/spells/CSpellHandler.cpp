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
#include "Problem.h"

#include <vcmi/spells/Caster.h>

#include "../filesystem/Filesystem.h"

#include "../constants/StringConstants.h"

#include "../CBonusTypeHandler.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/Unit.h"
#include "../json/JsonBonus.h"
#include "../json/JsonUtils.h"
#include "../GameLibrary.h"
#include "../modding/IdentifierStorage.h"
#include "../texts/CLegacyConfigParser.h"
#include "../texts/CGeneralTextHandler.h"

#include "ISpellMechanics.h"
#include "bonuses/BonusSelector.h"
#include "spells/SpellSchoolHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

static constexpr std::array LEVEL_NAMES = {"none", "basic", "advanced", "expert"};

///CSpell
CSpell::CSpell():
	id(SpellID::NONE),
	level(0),
	power(0),
	combat(false),
	creatureAbility(false),
	castOnSelf(false),
	castOnlyOnSelf(false),
	castWithoutSkip(false),
	positiveness(ESpellPositiveness::NEUTRAL),
	defaultProbability(0),
	rising(false),
	damage(false),
	offensive(false),
	special(true),
	nonMagical(false),
	targetType(spells::AimType::NO_TARGET)
{
	levels.resize(GameConstants::SPELL_SCHOOL_LEVELS);
}

//must be instantiated in .cpp file for access to complete types of all member fields
CSpell::~CSpell() = default;

bool CSpell::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	assert(env);

	if(!adventureMechanics)
	{
		env->complain("Invalid adventure spell cast attempt!");
		return false;
	}
	return adventureMechanics->adventureCast(env, parameters);
}

const CSpell::LevelInfo & CSpell::getLevelInfo(const int32_t level) const
{
	if(level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->error("CSpell::getLevelInfo: invalid school mastery level %d", level);
		return levels.at(MasteryLevel::EXPERT);
	}

	return levels.at(level);
}

int64_t CSpell::calculateDamage(const spells::Caster * caster) const
{
	//check if spell really does damage - if not, return 0
	if(!isDamage())
		return 0;
	auto rawDamage = calculateRawEffectValue(caster->getEffectLevel(this), caster->getEffectPower(this), 1);

	return caster->getSpellBonus(this, rawDamage, nullptr);
}

bool CSpell::hasSchool(SpellSchool which) const
{
	return schools.count(which);
}

bool CSpell::canBeCast(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const
{
	//if caller do not interested in description just discard it and do not pollute even debug log
	spells::detail::ProblemImpl problem;
	return canBeCast(problem, cb, mode, caster);
}

bool CSpell::canBeCast(spells::Problem & problem, const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const
{
	spells::BattleCast event(cb, caster, mode, this);
	auto mechanics = battleMechanics(&event);

	return mechanics->canBeCast(problem);
}

spells::AimType CSpell::getTargetType() const
{
	return targetType;
}

void CSpell::forEachSchool(const std::function<void(const SpellSchool &, bool &)>& cb) const
{
	bool stop = false;
	for(auto schoolID : LIBRARY->spellSchoolHandler->getAllObjects())
	{
		if(schools.count(schoolID))
		{
			cb(schoolID, stop);
			if(stop)
				break;
		}
	}
}

SpellID CSpell::getId() const
{
	return id;
}

std::string CSpell::getNameTextID() const
{
	TextIdentifier id("spell", modScope, identifier, "name");
	return id.get();
}

std::string CSpell::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CSpell::getDescriptionTextID(int32_t level) const
{
	TextIdentifier textID("spell", modScope, identifier, "description", LEVEL_NAMES[level]);
	return textID.get();
}

std::string CSpell::getDescriptionTranslated(int32_t level) const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID(level));
}

std::string CSpell::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CSpell::getModScope() const
{
	return modScope;
}

int32_t CSpell::getIndex() const
{
	return id.toEnum();
}

int32_t CSpell::getIconIndex() const
{
	return getIndex();
}

int32_t CSpell::getLevel() const
{
	return level;
}

bool CSpell::isCombat() const
{
	return combat;
}

bool CSpell::isAdventure() const
{
	return !combat;
}

bool CSpell::isCreatureAbility() const
{
	return creatureAbility;
}

bool CSpell::isMagical() const
{
	return !nonMagical;
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

boost::logic::tribool CSpell::getPositiveness() const
{
	switch (positiveness)
	{
	case CSpell::POSITIVE:
		return true;
	case CSpell::NEGATIVE:
		return false;
	default:
		return boost::logic::indeterminate;
	}
}

bool CSpell::isDamage() const
{
	return damage;
}

bool CSpell::isOffensive() const
{
	return offensive;
}

bool CSpell::isSpecial() const
{
	return special;
}

bool CSpell::hasEffects() const
{
	return !levels[0].effects.empty() || !levels[0].cumulativeEffects.empty();
}

bool CSpell::hasBattleEffects() const
{
	return levels[0].battleEffects.getType() == JsonNode::JsonType::DATA_STRUCT && !levels[0].battleEffects.Struct().empty();
}

bool CSpell::canCastOnSelf() const
{
	return castOnSelf;
}

bool CSpell::canCastOnlyOnSelf() const
{
	return castOnlyOnSelf;
}

bool CSpell::canCastWithoutSkip() const
{
	return castWithoutSkip;
}

const ImagePath & CSpell::getIconImmune() const
{
	return iconImmune;
}

const std::string & CSpell::getIconBook() const
{
	return iconBook;
}

const std::string & CSpell::getIconEffect() const
{
	return iconEffect;
}

const std::string & CSpell::getIconScenarioBonus() const
{
	return iconScenarioBonus;
}

const std::string & CSpell::getIconScroll() const
{
	return iconScroll;
}

const AudioPath & CSpell::getCastSound() const
{
	return castSound;
}

int32_t CSpell::getCost(const int32_t skillLevel) const
{
	return getLevelInfo(skillLevel).cost;
}

int32_t CSpell::getBasePower() const
{
	return power;
}

int32_t CSpell::getLevelPower(const int32_t skillLevel) const
{
	return getLevelInfo(skillLevel).power;
}

si32 CSpell::getProbability(const FactionID & factionId) const
{
	if(!vstd::contains(probabilities, factionId))
	{
		return defaultProbability;
	}
	return probabilities.at(factionId);
}

void CSpell::getEffects(std::vector<Bonus> & lst, const int level, const bool cumulative, const si32 duration, std::optional<si32 *> maxDuration) const
{
	if(level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->error("invalid school level %d", level);
		return;
	}

	const auto & levelObject = levels.at(level);

	if(levelObject.effects.empty() && levelObject.cumulativeEffects.empty())
	{
		logGlobal->error("This spell (%s) has no effects for level %d", getNameTranslated(), level);
		return;
	}

	const auto & effects = cumulative ? levelObject.cumulativeEffects : levelObject.effects;

	lst.reserve(lst.size() + effects.size());

	for(const auto& b : effects)
	{
		Bonus nb(*b);

		//use configured duration if present
		if(nb.turnsRemain == 0)
			nb.turnsRemain = duration;
		if(maxDuration)
			vstd::amax(*(maxDuration.value()), nb.turnsRemain);

		lst.push_back(nb);
	}
}

int64_t CSpell::adjustRawDamage(const spells::Caster * caster, const battle::Unit * affectedCreature, int64_t rawDamage) const
{
	auto ret = rawDamage;
	//affected creature-specific part
	if(nullptr != affectedCreature)
	{
		const auto * bearer = affectedCreature->getBonusBearer();
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		forEachSchool([&](const SpellSchool & cnf, bool & stop)
		{
			if(bearer->hasBonusOfType(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(cnf)))
			{
				ret *= 100 - bearer->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(cnf));
				ret /= 100;
				stop = true; //only bonus from one school is used
			}
		});

		CSelector selector = Selector::typeSubtype(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(SpellSchool::ANY));
		auto cachingStr = "type_SPELL_DAMAGE_REDUCTION_s_ANY";

		//general spell dmg reduction, works only on magical effects
		if(bearer->hasBonus(selector, cachingStr) && isMagical())
		{
			ret *= 100 - bearer->valOfBonuses(selector, cachingStr);
			ret /= 100;
		}

		//dmg increasing
		if(bearer->hasBonusOfType(BonusType::MORE_DAMAGE_FROM_SPELL, BonusSubtypeID(id)))
		{
			ret *= 100 + bearer->valOfBonuses(BonusType::MORE_DAMAGE_FROM_SPELL, BonusSubtypeID(id));
			ret /= 100;
		}

		//invincible
		if(affectedCreature->isInvincible())
			ret = 0;
	}
	ret = caster->getSpellBonus(this, ret, affectedCreature);
	return ret;
}

int64_t CSpell::calculateRawEffectValue(int32_t effectLevel, int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const
{
	return static_cast<int64_t>(basePowerMultiplier) * getBasePower() + levelPowerMultiplier * getLevelPower(effectLevel);
}

void CSpell::setIsOffensive(const bool val)
{
	offensive = val;

	if(val)
	{
		positiveness = CSpell::NEGATIVE;
		damage = true;
	}
}

void CSpell::setIsRising(const bool val)
{
	rising = val;

	if(val)
	{
		positiveness = CSpell::POSITIVE;
	}
}

JsonNode CSpell::convertTargetCondition(const BTVector & immunity, const BTVector & absImmunity, const BTVector & limit, const BTVector & absLimit) const
{
	static const std::string CONDITION_NORMAL = "normal";
	static const std::string CONDITION_ABSOLUTE = "absolute";

	JsonNode res;

	auto convertVector = [&](const std::string & targetName, const BTVector & source, const std::string & value)
	{
		for(auto bonusType : source)
		{
			std::string bonusName = LIBRARY->bth->bonusToString(bonusType);
			res[targetName][bonusName].String() = value;
		}
	};

	auto convertSection = [&](const std::string & targetName, const BTVector & normal, const BTVector & absolute)
	{
		convertVector(targetName, normal, CONDITION_NORMAL);
		convertVector(targetName, absolute, CONDITION_ABSOLUTE);
	};

	convertSection("allOf", limit, absLimit);
	convertSection("noneOf", immunity, absImmunity);

	return res;
}

void CSpell::setupMechanics()
{
	mechanics = spells::ISpellMechanicsFactory::get(this);
	adventureMechanics = IAdventureSpellMechanics::createMechanics(this);
}

const IAdventureSpellMechanics & CSpell::getAdventureMechanics() const
{
	return *adventureMechanics;
}

std::unique_ptr<spells::Mechanics> CSpell::battleMechanics(const spells::IBattleCast * event) const
{
	return mechanics->create(event);
}

void CSpell::registerIcons(const IconRegistar & cb) const
{
	cb(getIndex(), 0, "SPELLS", iconBook);
	cb(getIndex()+1, 0, "SPELLINT", iconEffect);
	cb(getIndex(), 0, "SPELLBON", iconScenarioBonus);
	cb(getIndex(), 0, "SPELLSCR", iconScroll);
}

void CSpell::updateFrom(const JsonNode & data)
{
	//todo:CSpell::updateFrom
}

void CSpell::serializeJson(JsonSerializeFormat & handler)
{

}

///CSpell::AnimationInfo
CSpell::AnimationItem::AnimationItem() :
	verticalPosition(VerticalPosition::TOP),
	transparency(1),
	pause(0)
{

}

///CSpell::AnimationInfo
AnimationPath CSpell::AnimationInfo::selectProjectile(const double angle) const
{
	AnimationPath res;
	double maximum = 0.0;

	for(const auto & info : projectile)
	{
		if(info.minimumAngle < angle && info.minimumAngle >= maximum)
		{
			maximum = info.minimumAngle;
			res = info.resourceName;
		}
	}

	return res;
}

///CSpell::TargetInfo
CSpell::TargetInfo::TargetInfo(const CSpell * spell, const int level, spells::Mode mode)
	: type(spell->getTargetType()),
	smart(false),
	massive(false),
	clearAffected(false)
{
	const auto & levelInfo = spell->getLevelInfo(level);

	smart = levelInfo.smartTarget;
	massive = levelInfo.range.empty();
	clearAffected = levelInfo.clearAffected;
}

///CSpellHandler
std::vector<JsonNode> CSpellHandler::loadLegacyData()
{
	std::vector<JsonNode> legacyData;

	CLegacyConfigParser parser(TextPath::builtin("DATA/SPTRAITS.TXT"));

	auto readSchool = [&](JsonMap & schools, const std::string & name)
	{
		if (parser.readString() == "x")
		{
			schools[name].Bool() = true;
		}
	};

	auto read = [&](bool combat, bool ability)
	{
		do
		{
			JsonNode lineNode;

			const auto id = legacyData.size();

			lineNode["index"].Integer() = id;
			lineNode["type"].String() = ability ? "ability" : (combat ? "combat" : "adventure");

			lineNode["name"].String() = parser.readString();

			parser.readString(); //ignored unused abbreviated name
			lineNode["level"].Integer() = static_cast<si64>(parser.readNumber());

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
			lineNode["power"].Integer() = static_cast<si64>(parser.readNumber());
			auto powers = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			auto & chances = lineNode["gainChance"].Struct();

			for(const auto & name : NFaction::names)
				chances[name].Integer() = static_cast<si64>(parser.readNumber());

			// Unused, AI values
			parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			std::vector<std::string> descriptions;
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
				descriptions.push_back(parser.readString());

			parser.readString(); //ignore attributes. All data present in JSON

			//save parsed level specific data
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
			{
				auto& level = getLevel(i);
				level["description"].String() = descriptions[i];
				level["cost"].Integer() = costs[i];
				level["power"].Integer() = powers[i];
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
	temp["index"].Integer() = SpellID::ACID_BREATH_DAMAGE;
	legacyData.push_back(temp);

	objects.resize(legacyData.size());

	return legacyData;
}

const std::vector<std::string> & CSpellHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "spell" };
	return typeNames;
}

std::vector<int> CSpellHandler::spellRangeInHexes(std::string input) const
{
	BattleHexArray ret;
	std::string rng = input + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 2 && std::tolower(rng[0]) != 'x') //there is at least one hex in range (+artificial comma)
	{
		std::string number1;
		std::string number2;
		int beg = 0;
		int end = 0;
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
					beg = std::stoi(number1);
					number1 = "";
				}
				else
				{
					end = std::stoi(number2);
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					ret.insert(beg);
				}
				else
				{
					for(int i = beg; i <= end; ++i)
						ret.insert(i);
				}
			}
			else if(elem == '-') //dash
			{
				beg = std::stoi(number1);
				number1 = "";
				readingFirst = false;
			}
		}
	}

	std::vector<int> result;
	result.reserve(ret.size());

	std::transform(ret.begin(), ret.end(), std::back_inserter(result),
		[](const BattleHex & hex) { return hex.toInt(); }
	);

	return result;
}

std::shared_ptr<CSpell> CSpellHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	SpellID id(static_cast<si32>(index));

	auto spell = std::make_shared<CSpell>();
	spell->id = id;
	spell->identifier = identifier;
	spell->modScope = scope;

	const auto type = json["type"].String();

	if(type == "ability")
	{
		spell->creatureAbility = true;
		spell->combat = true;
	}
	else
	{
		spell->creatureAbility = false;
		spell->combat = type == "combat";
	}

	LIBRARY->generaltexth->registerString(scope, spell->getNameTextID(), json["name"]);

	logMod->trace("%s: loading spell %s", __FUNCTION__, spell->getNameTranslated());

	for(const auto & schoolJson : json["school"].Struct())
	{
		if (schoolJson.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(schoolJson.second.getModScope(), "spellSchool", schoolJson.first, [spell](si32 schoolID)
			{
				spell->schools.emplace(schoolID);
			});
		}
	}

	spell->castOnSelf = json["canCastOnSelf"].Bool();
	spell->castOnlyOnSelf = json["canCastOnlyOnSelf"].Bool();
	spell->castWithoutSkip = json["canCastWithoutSkip"].Bool();
	spell->level = static_cast<si32>(json["level"].Integer());
	spell->power = static_cast<si32>(json["power"].Integer());

	spell->defaultProbability = static_cast<si32>(json["defaultGainChance"].Integer());

	for(const auto & node : json["gainChance"].Struct())
	{
		const int chance = static_cast<int>(node.second.Integer());

		LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "faction", node.first, [=](si32 factionID)
		{
			spell->probabilities[FactionID(factionID)] = chance;
		});
	}

	auto targetType = json["targetType"].String();

	if(targetType == "NO_TARGET")
		spell->targetType = spells::AimType::NO_TARGET;
	else if(targetType == "CREATURE")
		spell->targetType = spells::AimType::CREATURE;
	else if(targetType == "OBSTACLE")
		spell->targetType = spells::AimType::OBSTACLE;
	else if(targetType == "LOCATION")
		spell->targetType = spells::AimType::LOCATION;
	else
		logMod->warn("Spell %s: target type %s - assumed NO_TARGET.", spell->getNameTranslated(), (targetType.empty() ? "empty" : "unknown ("+targetType+")"));

	for(const auto & counteredSpell: json["counters"].Struct())
	{
		if(counteredSpell.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(counteredSpell.second.getModScope(), "spell", counteredSpell.first, [=](si32 id)
			{
				spell->counteredSpells.emplace_back(id);
			});
		}
	}

	//TODO: more error checking - f.e. conflicting flags
	const auto flags = json["flags"];

	//by default all flags are set to false in constructor

	spell->damage = flags["damage"].Bool(); //do this before "offensive"

	spell->nonMagical = flags["nonMagical"].Bool();

	if(flags["offensive"].Bool())
	{
		spell->setIsOffensive(true);
	}

	if(flags["rising"].Bool())
	{
		spell->setIsRising(true);
	}

	const bool implicitPositiveness = spell->offensive || spell->rising; //(!) "damage" does not mean NEGATIVE  --AVS

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
		logMod->error("Spell %s: no positiveness specified, assumed NEUTRAL.", spell->getNameTranslated());
	}

	spell->special = flags["special"].Bool();

	spell->onlyOnWaterMap = json["onlyOnWaterMap"].Bool();

	auto readBonusStruct = [&](const std::string & name, std::vector<BonusType> & vec)
	{
		for(auto bonusData: json[name].Struct())
		{
			if(!bonusData.second.Bool())
				continue;

			LIBRARY->identifiers()->requestIdentifier(bonusData.second.getModScope(), "bonus", bonusData.first, [&vec](si32 bonusID)
			{
				vec.push_back(static_cast<BonusType>(bonusID));
			});
		}
	};

	if(json["targetCondition"].isNull())
	{
		CSpell::BTVector immunities;
		CSpell::BTVector absoluteImmunities;
		CSpell::BTVector limiters;
		CSpell::BTVector absoluteLimiters;

		readBonusStruct("immunity", immunities);
		readBonusStruct("absoluteImmunity", absoluteImmunities);
		readBonusStruct("limit", limiters);
		readBonusStruct("absoluteLimit", absoluteLimiters);

		if(!(immunities.empty() && absoluteImmunities.empty() && limiters.empty() && absoluteLimiters.empty()))
		{
			logMod->warn("Spell %s has old target condition format. Expected configuration: ", spell->getNameTranslated());
			spell->targetCondition = spell->convertTargetCondition(immunities, absoluteImmunities, limiters, absoluteLimiters);
			logMod->warn("\n\"targetCondition\" : %s", spell->targetCondition.toString());
		}
	}
	else
	{
		spell->targetCondition = json["targetCondition"];

		//TODO: could this be safely merged instead of discarding?
		if(!json["immunity"].isNull())
			logMod->warn("Spell %s 'immunity' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["absoluteImmunity"].isNull())
			logMod->warn("Spell %s 'absoluteImmunity' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["limit"].isNull())
			logMod->warn("Spell %s 'limit' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["absoluteLimit"].isNull())
			logMod->warn("Spell %s 'absoluteLimit' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
	}

	const JsonNode & graphicsNode = json["graphics"];

	spell->iconImmune = ImagePath::fromJson(graphicsNode["iconImmune"]);
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

			if(item.getType() == JsonNode::JsonType::DATA_STRING)
				newItem.resourceName = AnimationPath::fromJson(item);
			else if(item.getType() == JsonNode::JsonType::DATA_STRUCT)
			{
				newItem.resourceName = AnimationPath::fromJson(item["defName"]);
				newItem.effectName   = item["effectName"].String();

				auto vPosStr = item["verticalPosition"].String();
				if("bottom" == vPosStr)
					newItem.verticalPosition = VerticalPosition::BOTTOM;

				if (item["transparency"].isNumber())
					newItem.transparency = item["transparency"].Float();
				else
					newItem.transparency = 1.0;
			}
			else if(item.isNumber())
			{
				newItem.pause = item.Integer();
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
		info.resourceName = AnimationPath::fromJson(item["defName"]);
		info.minimumAngle = item["minimumAngle"].Float();

		spell->animationInfo.projectile.push_back(info);
	}

	const JsonNode & soundsNode = json["sounds"];
	spell->castSound = AudioPath::fromJson(soundsNode["cast"]);

	//load level attributes
	const int levelsCount = GameConstants::SPELL_SCHOOL_LEVELS;

	for(int levelIndex = 0; levelIndex < levelsCount; levelIndex++)
	{
		const JsonNode & levelNode = json["levels"][LEVEL_NAMES[levelIndex]];

		CSpell::LevelInfo & levelObject = spell->levels[levelIndex];

		const si32 levelPower     = levelObject.power = static_cast<si32>(levelNode["power"].Integer());

		if (!spell->isCreatureAbility())
			LIBRARY->generaltexth->registerString(scope, spell->getDescriptionTextID(levelIndex), levelNode["description"]);

		levelObject.cost          = static_cast<si32>(levelNode["cost"].Integer());
		levelObject.smartTarget   = levelNode["targetModifier"]["smart"].Bool();
		levelObject.clearAffected = levelNode["targetModifier"]["clearAffected"].Bool();
		levelObject.range         = spellRangeInHexes(levelNode["range"].String());

		for(const auto & elem : levelNode["effects"].Struct())
		{
			const JsonNode & bonusNode = elem.second;
			auto b = JsonUtils::parseBonus(bonusNode);
			const bool usePowerAsValue = bonusNode["val"].isNull();

			b->sid = BonusSourceID(spell->id); //for all
			b->source = BonusSource::SPELL_EFFECT;//for all

			if(usePowerAsValue)
				b->val = levelPower;

			levelObject.effects.push_back(b);
		}

		for(const auto & elem : levelNode["cumulativeEffects"].Struct())
		{
			const JsonNode & bonusNode = elem.second;
			auto b = JsonUtils::parseBonus(bonusNode);
			const bool usePowerAsValue = bonusNode["val"].isNull();

			b->sid = BonusSourceID(spell->id); //for all
			b->source = BonusSource::SPELL_EFFECT;//for all

			if(usePowerAsValue)
				b->val = levelPower;

			levelObject.cumulativeEffects.push_back(b);
		}

		levelObject.adventureEffect = levelNode["adventureEffect"];

		if(!levelNode["battleEffects"].Struct().empty())
		{
			levelObject.battleEffects = levelNode["battleEffects"];

			if(!levelObject.cumulativeEffects.empty() || !levelObject.effects.empty() || spell->isOffensive())
				logGlobal->error("Mixing %s special effects with old format effects gives unpredictable result", spell->getNameTranslated());
		}
	}
	return spell;
}

void CSpellHandler::afterLoadFinalization()
{
	for(auto & spell : objects)
	{
		spell->setupMechanics();
	}
}

void CSpellHandler::beforeValidate(JsonNode & object)
{
	//handle "base" level info

	JsonNode & levels = object["levels"];
	JsonNode & base = levels["base"];

	auto inheritNode = [&](const std::string & name)
	{
		JsonUtils::inherit(levels[name],base);
	};

	inheritNode("none");
	inheritNode("basic");
	inheritNode("advanced");
	inheritNode("expert");
}

std::set<SpellID> CSpellHandler::getDefaultAllowed() const
{
	std::set<SpellID> allowedSpells;

	for(auto const & s : objects)
		if (!s->isSpecial() && !s->isCreatureAbility())
			allowedSpells.insert(s->getId());

	return allowedSpells;
}

VCMI_LIB_NAMESPACE_END
