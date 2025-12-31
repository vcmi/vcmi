/*
 * CHeroHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroHandler.h"

#include "CHero.h"

#include "../../GameLibrary.h"
#include "../../constants/StringConstants.h"
#include "../../CCreatureHandler.h"
#include "../../IGameSettings.h"
#include "../../bonuses/Limiters.h"
#include "../../bonuses/Updaters.h"
#include "../../json/JsonBonus.h"
#include "../../json/JsonUtils.h"
#include "../../modding/IdentifierStorage.h"
#include "../../CSkillHandler.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/CLegacyConfigParser.h"

VCMI_LIB_NAMESPACE_BEGIN

CHeroHandler::~CHeroHandler() = default;

CHeroHandler::CHeroHandler()
{
	loadExperience();
}

const std::vector<std::string> & CHeroHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "hero" };
	return typeNames;
}

std::shared_ptr<CHero> CHeroHandler::loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	auto hero = std::make_shared<CHero>();
	hero->ID = HeroTypeID(index);
	hero->identifier = identifier;
	hero->modScope = scope;
	hero->gender = node["female"].Bool() ? EHeroGender::FEMALE : EHeroGender::MALE;
	hero->special = node["special"].Bool();
	//Default - both false
	hero->onlyOnWaterMap = node["onlyOnWaterMap"].Bool();
	hero->onlyOnMapWithoutWater = node["onlyOnMapWithoutWater"].Bool();

	LIBRARY->generaltexth->registerString(scope, hero->getNameTextID(), node["texts"]["name"]);
	LIBRARY->generaltexth->registerString(scope, hero->getBiographyTextID(), node["texts"]["biography"]);
	LIBRARY->generaltexth->registerString(scope, hero->getSpecialtyNameTextID(), node["texts"]["specialty"]["name"]);
	LIBRARY->generaltexth->registerString(scope, hero->getSpecialtyTooltipTextID(), node["texts"]["specialty"]["tooltip"]);
	LIBRARY->generaltexth->registerString(scope, hero->getSpecialtyDescriptionTextID(), node["texts"]["specialty"]["description"]);

	hero->iconSpecSmall = node["images"]["specialtySmall"].String();
	hero->iconSpecLarge = node["images"]["specialtyLarge"].String();
	hero->portraitSmall = node["images"]["small"].String();
	hero->portraitLarge = node["images"]["large"].String();
	hero->battleImage = AnimationPath::fromJson(node["battleImage"]);

	loadHeroArmy(hero.get(), node);
	loadHeroSkills(hero.get(), node);
	loadHeroSpecialty(hero.get(), node);

	LIBRARY->identifiers()->requestIdentifier("heroClass", node["class"],
	[=](si32 classID)
	{
		hero->heroClass = HeroClassID(classID).toHeroClass();
	});

	return hero;
}

void CHeroHandler::loadHeroArmy(CHero * hero, const JsonNode & node) const
{
	assert(node["army"].Vector().size() <= 3); // anything bigger is useless - army initialization uses up to 3 slots

	hero->initialArmy.resize(node["army"].Vector().size());

	for (size_t i=0; i< hero->initialArmy.size(); i++)
	{
		const JsonNode & source = node["army"].Vector()[i];

		hero->initialArmy[i].minAmount = static_cast<ui32>(source["min"].Float());
		hero->initialArmy[i].maxAmount = static_cast<ui32>(source["max"].Float());

		if (hero->initialArmy[i].minAmount > hero->initialArmy[i].maxAmount)
		{
			logMod->error("Hero %s has minimal army size (%d) greater than maximal size (%d)!", hero->getJsonKey(), hero->initialArmy[i].minAmount, hero->initialArmy[i].maxAmount);
			std::swap(hero->initialArmy[i].minAmount, hero->initialArmy[i].maxAmount);
		}

		LIBRARY->identifiers()->requestIdentifier("creature", source["creature"], [=](si32 creature)
		{
			hero->initialArmy[i].creature = CreatureID(creature);
		});
	}
}

void CHeroHandler::loadHeroSkills(CHero * hero, const JsonNode & node) const
{
	for(const JsonNode &set : node["skills"].Vector())
	{
		int skillLevel = static_cast<int>(boost::range::find(NSecondarySkill::levels, set["level"].String()) - std::begin(NSecondarySkill::levels));
		if (skillLevel < MasteryLevel::LEVELS_SIZE)
		{
			size_t currentIndex = hero->secSkillsInit.size();
			hero->secSkillsInit.emplace_back(SecondarySkill(-1), skillLevel);

			LIBRARY->identifiers()->requestIdentifier("skill", set["skill"], [=](si32 id)
			{
				hero->secSkillsInit[currentIndex].first = SecondarySkill(id);
			});
		}
		else
		{
			logMod->error("Unknown skill level: %s", set["level"].String());
		}
	}

	// spellbook is considered present if hero have "spellbook" entry even when this is an empty set (0 spells)
	hero->haveSpellBook = !node["spellbook"].isNull();

	for(const JsonNode & spell : node["spellbook"].Vector())
	{
		LIBRARY->identifiers()->requestIdentifier("spell", spell,
		[=](si32 spellID)
		{
			hero->spells.insert(SpellID(spellID));
		});
	}
}

std::vector<std::shared_ptr<Bonus>> CHeroHandler::createCreatureSpecialty(CreatureID cid, int fixedLevel, int growthPerStep) const
{
	std::vector<std::shared_ptr<Bonus>> result;
	const auto & specCreature = *cid.toCreature();

	if (fixedLevel == 0)
		fixedLevel = specCreature.getLevel();

	if (fixedLevel == 0)
	{
		fixedLevel = 5;
		logMod->warn("Creature '%s' of level 0 has hero with generic specialty! Please specify level explicitly or give creature non-zero level", specCreature.getJsonKey());
	}

	if (growthPerStep == 0)
		growthPerStep = LIBRARY->engineSettings()->getInteger(EGameSettings::HEROES_SPECIALTY_CREATURE_GROWTH);

	{
		auto bonus = std::make_shared<Bonus>();
		bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, true));
		bonus->type = BonusType::STACKS_SPEED;
		bonus->val = 1;
		result.push_back(bonus);
	}

	for (const auto & skill : { PrimarySkill::ATTACK, PrimarySkill::DEFENSE})
	{
		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::PRIMARY_SKILL;
		bonus->subtype = BonusSubtypeID(skill);
		bonus->val = growthPerStep;
		bonus->valType = BonusValueType::PERCENT_TO_TARGET_TYPE;
		bonus->targetSourceType = BonusSource::CREATURE_ABILITY;
		bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, true));
		bonus->updater.reset(new TimesHeroLevelUpdater(fixedLevel));
		result.push_back(bonus);
	}
	return result;
}

std::vector<std::shared_ptr<Bonus>> CHeroHandler::createSecondarySkillSpecialty(SecondarySkill skillID, int growthPerStep) const
{
	std::vector<std::shared_ptr<Bonus>> result;
	const auto & skillPtr = LIBRARY->skillh->objects[skillID.getNum()];

	if (skillPtr->specialtyTargetBonuses.empty())
		logMod->warn("Skill '%s' does not supports generic specialties!", skillPtr->getJsonKey());

	if (growthPerStep == 0)
		growthPerStep = LIBRARY->engineSettings()->getInteger(EGameSettings::HEROES_SPECIALTY_SECONDARY_SKILL_GROWTH);

	for (const auto & bonus : skillPtr->specialtyTargetBonuses)
	{
		auto bonusCopy = std::make_shared<Bonus>(*bonus);
		bonusCopy->val = growthPerStep;
		result.push_back(bonusCopy);
	}

	return result;
}

void CHeroHandler::beforeValidate(JsonNode & object)
{
	//handle "base" specialty info
	JsonNode & specialtyNode = object["specialty"];
	if(specialtyNode.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		if(specialtyNode.Struct().count("base") != 0)
		{
			const JsonNode & base = specialtyNode["base"];
			if(specialtyNode["bonuses"].isNull())
			{
				logMod->warn("specialty has base without bonuses");
			}
			else
			{
				JsonMap & bonuses = specialtyNode["bonuses"].Struct();
				for(std::pair<std::string, JsonNode> keyValue : bonuses)
					JsonUtils::inherit(bonuses[keyValue.first], base);
			}
			specialtyNode.Struct().erase("base");
		}
	}
}

void CHeroHandler::loadHeroSpecialty(CHero * hero, const JsonNode & node) const
{
	auto prepSpec = [=](std::shared_ptr<Bonus> bonus)
	{
		bonus->duration = BonusDuration::PERMANENT;
		bonus->source = BonusSource::HERO_SPECIAL;
		bonus->sid = BonusSourceID(hero->getId());
		return bonus;
	};

	//new format, using bonus system
	const JsonNode & specialtyNode = node["specialty"];
	if(specialtyNode.getType() != JsonNode::JsonType::DATA_STRUCT)
	{
		logMod->error("Unsupported speciality format for hero %s!", hero->getNameTranslated());
		return;
	}

	//creature specialty - alias for simplicity
	if(!specialtyNode["creature"].isNull())
	{
		const JsonNode & creatureNode = specialtyNode["creature"];
		int targetLevel = specialtyNode["creatureLevel"].Integer();
		int stepSize = specialtyNode["stepSize"].Integer();

		LIBRARY->identifiers()->requestIdentifier("creature", creatureNode, [this, hero, prepSpec, targetLevel, stepSize](si32 creature)
		{
			for (const auto & bonus : createCreatureSpecialty(CreatureID(creature), targetLevel, stepSize))
				hero->specialty.push_back(prepSpec(bonus));
		});
	}

	//secondary skill specialty - alias for simplicity
	if(!specialtyNode["secondary"].isNull())
	{
		const JsonNode & skillNode = specialtyNode["secondary"];
		int stepSize = specialtyNode["stepSize"].Integer();

		LIBRARY->identifiers()->requestIdentifier("secondarySkill", skillNode, [this, hero, stepSize](si32 skill)
		{
			skillSpecialtiesToGenerate.push_back({hero->ID, SecondarySkill(skill), stepSize});
		});
	}

	for(const auto & keyValue : specialtyNode["bonuses"].Struct())
		hero->specialty.push_back(prepSpec(JsonUtils::parseBonus(keyValue.second)));
}

void CHeroHandler::loadExperience()
{
	expPerLevel.push_back(0);
	expPerLevel.push_back(1000);
	expPerLevel.push_back(2000);
	expPerLevel.push_back(3200);
	expPerLevel.push_back(4600);
	expPerLevel.push_back(6200);
	expPerLevel.push_back(8000);
	expPerLevel.push_back(10000);
	expPerLevel.push_back(12200);
	expPerLevel.push_back(14700);
	expPerLevel.push_back(17500);
	expPerLevel.push_back(20600);
	expPerLevel.push_back(24320);
	expPerLevel.push_back(28784);
	expPerLevel.push_back(34140);

	for (;;)
	{
		auto i = expPerLevel.size() - 1;
		auto currExp = expPerLevel[i];
		auto prevExp = expPerLevel[i-1];
		auto prevDiff = currExp - prevExp;
		auto nextDiff = prevDiff + prevDiff / 5;
		auto maxExp = std::numeric_limits<decltype(currExp)>::max();

		if (currExp > maxExp - nextDiff)
			break; // overflow point reached

		expPerLevel.push_back (currExp + nextDiff);
	}
}

/// convert h3-style ID (e.g. Gobin Wolf Rider) to vcmi (e.g. goblinWolfRider)
static std::string genRefName(std::string input)
{
	boost::algorithm::replace_all(input, " ", ""); //remove spaces
	input[0] = std::tolower(input[0]); // to camelCase
	return input;
}

std::vector<JsonNode> CHeroHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_HERO);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser specParser(TextPath::builtin("DATA/HEROSPEC.TXT"));
	CLegacyConfigParser bioParser(TextPath::builtin("DATA/HEROBIOS.TXT"));
	CLegacyConfigParser parser(TextPath::builtin("DATA/HOTRAITS.TXT"));

	parser.endLine(); //ignore header
	parser.endLine();

	specParser.endLine(); //ignore header
	specParser.endLine();

	for (int i=0; i<GameConstants::HEROES_QUANTITY; i++)
	{
		JsonNode heroData;

		heroData["texts"]["name"].String() = parser.readString();
		heroData["texts"]["biography"].String() = bioParser.readString();
		heroData["texts"]["specialty"]["name"].String() = specParser.readString();
		heroData["texts"]["specialty"]["tooltip"].String() = specParser.readString();
		heroData["texts"]["specialty"]["description"].String() = specParser.readString();

		for(int x=0;x<3;x++)
		{
			JsonNode armySlot;
			armySlot["min"].Float() = parser.readNumber();
			armySlot["max"].Float() = parser.readNumber();
			armySlot["creature"].String() = genRefName(parser.readString());

			heroData["army"].Vector().push_back(armySlot);
		}
		parser.endLine();
		specParser.endLine();
		bioParser.endLine();

		h3Data.push_back(heroData);
	}
	return h3Data;
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	size_t index = objects.size();
	static const int specialFramesCount = 2; // reserved for 2 special frames
	auto object = loadFromJson(scope, data, name, index);
	object->imageIndex = static_cast<si32>(index) + specialFramesCount;

	objects.emplace_back(object);

	registerObject(scope, "hero", name, object->getIndex());

	for(const auto & compatID : data["compatibilityIdentifiers"].Vector())
		registerObject(scope, "hero", compatID.String(), object->getIndex());
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, name, index);
	object->imageIndex = static_cast<si32>(index);

	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = object;

	registerObject(scope, "hero", name, object->getIndex());
	for(const auto & compatID : data["compatibilityIdentifiers"].Vector())
		registerObject(scope, "hero", compatID.String(), object->getIndex());
}

ui32 CHeroHandler::level (TExpType experience) const
{
	return static_cast<ui32>(boost::range::upper_bound(expPerLevel, experience) - std::begin(expPerLevel));
}

TExpType CHeroHandler::reqExp (ui32 level) const
{
	if(!level)
		return 0;

	if (level <= expPerLevel.size())
	{
		return expPerLevel[level-1];
	}
	else
	{
		logGlobal->warn("A hero has reached unsupported amount of experience");
		return expPerLevel[expPerLevel.size()-1];
	}
}

ui32 CHeroHandler::maxSupportedLevel() const
{
	return expPerLevel.size();
}

std::set<HeroTypeID> CHeroHandler::getDefaultAllowed() const
{
	std::set<HeroTypeID> result;

	for(auto & hero : objects)
		if (hero && !hero->special)
			result.insert(hero->getId());

	return result;
}

void CHeroHandler::afterLoadFinalization()
{
	auto prepSpec = [](HeroTypeID hero, std::shared_ptr<Bonus> bonus)
	{
		bonus->duration = BonusDuration::PERMANENT;
		bonus->source = BonusSource::HERO_SPECIAL;
		bonus->sid = BonusSourceID(hero);
		return bonus;
	};

	// Workaround for loading order issue
	// To load secondary skill specialty, bonus ID's must be loaded first
	// However, identifier request only guarantee that requested object itself is loaded
	// Meaning, it is possible for skill ID to be resolved before bonus ID is resolved,
	// leading to createSecondarySkillSpecialty creating copy of incomplete bonus
	for (const auto & specialty : skillSpecialtiesToGenerate)
	{
		for (const auto & bonus : createSecondarySkillSpecialty(specialty.skill, specialty.stepSize))
			objects.at(specialty.hero.getNum())->specialty.push_back(prepSpec(specialty.hero, bonus));
	}
}

VCMI_LIB_NAMESPACE_END
