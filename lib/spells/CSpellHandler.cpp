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

#include "../CGeneralTextHandler.h"
#include "../filesystem/Filesystem.h"

#include "../CModHandler.h"
#include "../StringConstants.h"

#include "../battle/BattleInfo.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/Unit.h"

#include "../mapObjects/CGHeroInstance.h" //todo: remove
#include "../serializer/CSerializer.h"

#include "ISpellMechanics.h"

namespace SpellConfig
{
static const std::string LEVEL_NAMES[] = {"none", "basic", "advanced", "expert"};

static const spells::SchoolInfo SCHOOL[4] =
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

//order as described in http://bugs.vcmi.eu/view.php?id=91
static const ESpellSchool SCHOOL_ORDER[4] =
{
	ESpellSchool::AIR,  //=0
	ESpellSchool::FIRE, //=1
	ESpellSchool::EARTH,//=3(!)
	ESpellSchool::WATER //=2(!)
};
} //namespace SpellConfig

///CSpell::LevelInfo
CSpell::LevelInfo::LevelInfo()
	: description(""),
	cost(0),
	power(0),
	AIValue(0),
	smartTarget(true),
	clearTarget(false),
	clearAffected(false),
	range("0")
{

}

CSpell::LevelInfo::~LevelInfo()
{

}

///CSpell
CSpell::CSpell():
	id(SpellID::NONE),
	level(0),
	power(0),
	combat(false),
	creatureAbility(false),
	positiveness(ESpellPositiveness::NEUTRAL),
	defaultProbability(0),
	rising(false),
	damage(false),
	offensive(false),
	special(true),
	targetType(spells::AimType::NO_TARGET),
	mechanics(),
	adventureMechanics()
{
	levels.resize(GameConstants::SPELL_SCHOOL_LEVELS);
}

CSpell::~CSpell()
{

}

bool CSpell::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	assert(env);

	if(!adventureMechanics.get())
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
		logGlobal->error("CSpell::getLevelInfo: invalid school level %d", level);
		return levels.at(0);
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

void CSpell::forEachSchool(const std::function<void(const spells::SchoolInfo &, bool &)>& cb) const
{
	bool stop = false;
	for(ESpellSchool iter : SpellConfig::SCHOOL_ORDER)
	{
		const spells::SchoolInfo & cnf = SpellConfig::SCHOOL[(ui8)iter];
		if(school.at(cnf.id))
		{
			cb(cnf, stop);

			if(stop)
				break;
		}
	}
}

SpellID CSpell::getId() const
{
	return id;
}

const std::string & CSpell::getName() const
{
	return name;
}

const std::string & CSpell::getJsonKey() const
{
	return identifier;
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

const std::string & CSpell::getIconImmune() const
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

const std::string & CSpell::getCastSound() const
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

const std::string & CSpell::getLevelDescription(const int32_t skillLevel) const
{
	return getLevelInfo(skillLevel).description;
}

si32 CSpell::getProbability(const TFaction factionId) const
{
	if(!vstd::contains(probabilities,factionId))
	{
		return defaultProbability;
	}
	return probabilities.at(factionId);
}

void CSpell::getEffects(std::vector<Bonus> & lst, const int level, const bool cumulative, const si32 duration, boost::optional<si32 *> maxDuration) const
{
	if(level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->error("invalid school level %d", level);
		return;
	}

	const auto & levelObject = levels.at(level);

	if(levelObject.effects.empty() && levelObject.cumulativeEffects.empty())
	{
		logGlobal->error("This spell (%s) has no effects for level %d", name, level);
		return;
	}

	const auto & effects = cumulative ? levelObject.cumulativeEffects : levelObject.effects;

	lst.reserve(lst.size() + effects.size());

	for(const auto b : effects)
	{
		Bonus nb(*b);

		//use configured duration if present
		if(nb.turnsRemain == 0)
			nb.turnsRemain = duration;
		if(maxDuration)
			vstd::amax(*(maxDuration.get()), nb.turnsRemain);

		lst.push_back(nb);
	}
}

int64_t CSpell::adjustRawDamage(const spells::Caster * caster, const battle::Unit * affectedCreature, int64_t rawDamage) const
{
	auto ret = rawDamage;
	//affected creature-specific part
	if(nullptr != affectedCreature)
	{
		auto bearer = affectedCreature;
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		forEachSchool([&](const spells::SchoolInfo & cnf, bool & stop)
		{
			if(bearer->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id))
			{
				ret *= 100 - bearer->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, (ui8)cnf.id);
				ret /= 100;
				stop = true;//only bonus from one school is used
			}
		});

		CSelector selector = Selector::type()(Bonus::SPELL_DAMAGE_REDUCTION).And(Selector::subtype()(-1));

		//general spell dmg reduction
		if(bearer->hasBonus(selector))
		{
			ret *= 100 - bearer->valOfBonuses(selector);
			ret /= 100;
		}

		//dmg increasing
		if(bearer->hasBonusOfType(Bonus::MORE_DAMAGE_FROM_SPELL, id))
		{
			ret *= 100 + bearer->valOfBonuses(Bonus::MORE_DAMAGE_FROM_SPELL, id.toEnum());
			ret /= 100;
		}
	}
	ret = caster->getSpellBonus(this, ret, affectedCreature);
	return ret;
}

int64_t CSpell::calculateRawEffectValue(int32_t effectLevel, int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const
{
	return (int64_t)basePowerMultiplier * getBasePower() + levelPowerMultiplier * getLevelPower(effectLevel);
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

#define BONUS_NAME(x) { Bonus::x, #x },
	static const std::map<Bonus::BonusType, std::string> bonusNameRMap = { BONUS_LIST };
#undef BONUS_NAME

	JsonNode res;

	auto convertVector = [&](const std::string & targetName, const BTVector & source, const std::string & value)
	{
		for(auto bonusType : source)
		{
			auto iter = bonusNameRMap.find(bonusType);
			if(iter != bonusNameRMap.end())
			{
				auto fullId = CModHandler::makeFullIdentifier("", "bonus", iter->second);
				res[targetName][fullId].String() = value;
			}
			else
			{
				logGlobal->error("Invalid bonus type %d", static_cast<int32_t>(bonusType));
			}
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

std::unique_ptr<spells::Mechanics> CSpell::battleMechanics(const spells::IBattleCast * event) const
{
	return mechanics->create(event);
}

void CSpell::registerIcons(const IconRegistar & cb) const
{
	cb(getIndex(), "SPELLS", iconBook);
	cb(getIndex()+1, "SPELLINT", iconEffect);
	cb(getIndex(), "SPELLBON", iconScenarioBonus);
	cb(getIndex(), "SPELLSCR", iconScroll);
}

void CSpell::updateFrom(const JsonNode & data)
{
	//todo:CSpell::updateFrom
}

void CSpell::serializeJson(JsonSerializeFormat & handler)
{

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
CSpell::TargetInfo::TargetInfo(const CSpell * spell, const int level, spells::Mode mode)
	: type(spell->getTargetType()),
	smart(false),
	massive(false),
	clearAffected(false),
	clearTarget(false)
{
	auto & levelInfo = spell->getLevelInfo(level);

	smart = levelInfo.smartTarget;
	massive = levelInfo.range == "X";
	clearAffected = levelInfo.clearAffected;
	clearTarget = levelInfo.clearTarget;

	if(mode == spells::Mode::CREATURE_ACTIVE)
	{
		massive = false;//FIXME: find better solution for Commander spells
	}
}

bool DLL_LINKAGE isInScreenRange(const int3 & center, const int3 & pos)
{
	int3 diff = pos - center;
	return diff.x >= -9 && diff.x <= 9 && diff.y >= -8 && diff.y <= 8;
}

///CSpellHandler
CSpellHandler::CSpellHandler() = default;

CSpellHandler::~CSpellHandler() = default;

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

	auto read = [&](bool combat, bool ability)
	{
		do
		{
			JsonNode lineNode(JsonNode::JsonType::DATA_STRUCT);

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

			for(size_t i = 0; i < GameConstants::F_NUMBER; i++)
				chances[ETownType::names[i]].Integer() = static_cast<si64>(parser.readNumber());

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
				level["cost"].Integer() = costs[i];
				level["power"].Integer() = powers[i];
				level["aiValue"].Integer() = AIVals[i];
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

CSpell * CSpellHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	using namespace SpellConfig;

	SpellID id(static_cast<si32>(index));

	CSpell * spell = new CSpell();
	spell->id = id;
	spell->identifier = identifier;

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

	spell->name = json["name"].String();

	logMod->trace("%s: loading spell %s", __FUNCTION__, spell->name);

	const auto schoolNames = json["school"];

	for(const spells::SchoolInfo & info : SpellConfig::SCHOOL)
	{
		spell->school[info.id] = schoolNames[info.jsonName].Bool();
	}

	spell->level = static_cast<si32>(json["level"].Integer());
	spell->power = static_cast<si32>(json["power"].Integer());

	spell->defaultProbability = static_cast<si32>(json["defaultGainChance"].Integer());

	for(const auto & node : json["gainChance"].Struct())
	{
		const int chance = static_cast<int>(node.second.Integer());

		VLC->modh->identifiers.requestIdentifier(node.second.meta, "faction", node.first, [=](si32 factionID)
		{
			spell->probabilities[factionID] = chance;
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
		logMod->warn("Spell %s: target type %s - assumed NO_TARGET.", spell->name, (targetType.empty() ? "empty" : "unknown ("+targetType+")"));

	for(const auto & counteredSpell: json["counters"].Struct())
	{
		if(counteredSpell.second.Bool())
		{
			VLC->modh->identifiers.requestIdentifier(json.meta, counteredSpell.first, [=](si32 id)
			{
				spell->counteredSpells.push_back(SpellID(id));
			});
		}
	}

	//TODO: more error checking - f.e. conflicting flags
	const auto flags = json["flags"];

	//by default all flags are set to false in constructor

	spell->damage = flags["damage"].Bool(); //do this before "offensive"

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
		logMod->error("Spell %s: no positiveness specified, assumed NEUTRAL.", spell->name);
	}

	spell->special = flags["special"].Bool();

	auto findBonus = [&](std::string name, std::vector<Bonus::BonusType> & vec)
	{
		auto it = bonusNameMap.find(name);
		if(it == bonusNameMap.end())
		{
			logMod->error("Spell %s: invalid bonus name %s", spell->name, name);
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
			logMod->warn("Spell %s has old target condition format. Expected configuration: ", spell->name);
			spell->targetCondition = spell->convertTargetCondition(immunities, absoluteImmunities, limiters, absoluteLimiters);
			logMod->warn("\n\"targetCondition\" : %s", spell->targetCondition.toJson());
		}
	}
	else
	{
		spell->targetCondition = json["targetCondition"];

		//TODO: could this be safely merged instead of discarding?
		if(!json["immunity"].isNull())
			logMod->warn("Spell %s 'immunity' field mixed with 'targetCondition' discarded", spell->name);
		if(!json["absoluteImmunity"].isNull())
			logMod->warn("Spell %s 'absoluteImmunity' field mixed with 'targetCondition' discarded", spell->name);
		if(!json["limit"].isNull())
			logMod->warn("Spell %s 'limit' field mixed with 'targetCondition' discarded", spell->name);
		if(!json["absoluteLimit"].isNull())
			logMod->warn("Spell %s 'absoluteLimit' field mixed with 'targetCondition' discarded", spell->name);
	}

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

			if(item.getType() == JsonNode::JsonType::DATA_STRING)
				newItem.resourceName = item.String();
			else if(item.getType() == JsonNode::JsonType::DATA_STRUCT)
			{
				newItem.resourceName = item["defName"].String();

				auto vPosStr = item["verticalPosition"].String();
				if("bottom" == vPosStr)
					newItem.verticalPosition = VerticalPosition::BOTTOM;
			}
			else if(item.isNumber())
			{
				newItem.pause = static_cast<int>(item.Float());
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

		const si32 levelPower     = levelObject.power = static_cast<si32>(levelNode["power"].Integer());

		levelObject.description   = levelNode["description"].String();
		levelObject.cost          = static_cast<si32>(levelNode["cost"].Integer());
		levelObject.AIValue       = static_cast<si32>(levelNode["aiValue"].Integer());
		levelObject.smartTarget   = levelNode["targetModifier"]["smart"].Bool();
		levelObject.clearTarget   = levelNode["targetModifier"]["clearTarget"].Bool();
		levelObject.clearAffected = levelNode["targetModifier"]["clearAffected"].Bool();
		levelObject.range         = levelNode["range"].String();

		for(const auto & elem : levelNode["effects"].Struct())
		{
			const JsonNode & bonusNode = elem.second;
			auto b = JsonUtils::parseBonus(bonusNode);
			const bool usePowerAsValue = bonusNode["val"].isNull();

			b->sid = spell->id; //for all
			b->source = Bonus::SPELL_EFFECT;//for all

			if(usePowerAsValue)
				b->val = levelPower;

			levelObject.effects.push_back(b);
		}

		for(const auto & elem : levelNode["cumulativeEffects"].Struct())
		{
			const JsonNode & bonusNode = elem.second;
			auto b = JsonUtils::parseBonus(bonusNode);
			const bool usePowerAsValue = bonusNode["val"].isNull();

			b->sid = spell->id; //for all
			b->source = Bonus::SPELL_EFFECT;//for all

			if(usePowerAsValue)
				b->val = levelPower;

			levelObject.cumulativeEffects.push_back(b);
		}

		if(levelNode["battleEffects"].getType() == JsonNode::JsonType::DATA_STRUCT && !levelNode["battleEffects"].Struct().empty())
		{
			levelObject.battleEffects = levelNode["battleEffects"];

			if(!levelObject.cumulativeEffects.empty() || !levelObject.effects.empty() || spell->isOffensive())
				logGlobal->error("Mixing %s special effects with old format effects gives unpredictable result", spell->name);
		}
	}
	return spell;
}

void CSpellHandler::afterLoadFinalization()
{
	for(auto spell : objects)
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

std::vector<bool> CSpellHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedSpells;
	allowedSpells.reserve(objects.size());

	for(const CSpell * s : objects)
	{
		allowedSpells.push_back( !(s->isSpecial() || s->isCreatureAbility()));
	}

	return allowedSpells;
}

void CSpellHandler::update780()
{
    static_assert(MINIMAL_SERIALIZATION_VERSION < 780, "No longer needed CSpellHandler::update780");

	auto spellsContent = (*VLC->modh->content)["spells"];

	const ContentTypeHandler::ModInfo & coreData = spellsContent.modData.at("core");

	const JsonNode & coreSpells = coreData.modData;

	const int levelsCount = GameConstants::SPELL_SCHOOL_LEVELS;

	for(CSpell * spell : objects)
	{
		auto identifier = spell->identifier;
		size_t colonPos = identifier.find(':');
		if(colonPos != std::string::npos)
			continue;

		const JsonNode & actualConfig = coreSpells[spell->identifier];

		if(actualConfig.getType() != JsonNode::JsonType::DATA_STRUCT)
		{
			logGlobal->error("Spell not found %s", spell->identifier);
			continue;
		}

		if(actualConfig["targetCondition"].getType() == JsonNode::JsonType::DATA_STRUCT && !actualConfig["targetCondition"].Struct().empty())
		{
			spell->targetCondition = actualConfig["targetCondition"];
		}

		for(int levelIndex = 0; levelIndex < levelsCount; levelIndex++)
		{
			const JsonNode & levelNode = actualConfig["levels"][SpellConfig::LEVEL_NAMES[levelIndex]];

			logGlobal->debug(levelNode.toJson());

			CSpell::LevelInfo & levelObject = spell->levels[levelIndex];

			if(levelNode["battleEffects"].getType() == JsonNode::JsonType::DATA_STRUCT && !levelNode["battleEffects"].Struct().empty())
			{
				levelObject.battleEffects = levelNode["battleEffects"];

				logGlobal->trace("Updated special effects for level %d of spell %s", levelIndex, spell->identifier);
			}
		}

	}
}
