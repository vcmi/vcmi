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

#include "../../VCMI_Lib.h"
#include "../../constants/StringConstants.h"
#include "../../CCreatureHandler.h"
#include "../../IGameSettings.h"
#include "../../bonuses/Limiters.h"
#include "../../bonuses/Updaters.h"
#include "../../json/JsonBonus.h"
#include "../../json/JsonUtils.h"
#include "../../modding/IdentifierStorage.h"
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

	VLC->generaltexth->registerString(scope, hero->getNameTextID(), node["texts"]["name"]);
	VLC->generaltexth->registerString(scope, hero->getBiographyTextID(), node["texts"]["biography"]);
	VLC->generaltexth->registerString(scope, hero->getSpecialtyNameTextID(), node["texts"]["specialty"]["name"]);
	VLC->generaltexth->registerString(scope, hero->getSpecialtyTooltipTextID(), node["texts"]["specialty"]["tooltip"]);
	VLC->generaltexth->registerString(scope, hero->getSpecialtyDescriptionTextID(), node["texts"]["specialty"]["description"]);

	hero->iconSpecSmall = node["images"]["specialtySmall"].String();
	hero->iconSpecLarge = node["images"]["specialtyLarge"].String();
	hero->portraitSmall = node["images"]["small"].String();
	hero->portraitLarge = node["images"]["large"].String();
	hero->battleImage = AnimationPath::fromJson(node["battleImage"]);

	loadHeroArmy(hero.get(), node);
	loadHeroSkills(hero.get(), node);
	loadHeroSpecialty(hero.get(), node);

	VLC->identifiers()->requestIdentifier("heroClass", node["class"],
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

		VLC->identifiers()->requestIdentifier("creature", source["creature"], [=](si32 creature)
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

			VLC->identifiers()->requestIdentifier("skill", set["skill"], [=](si32 id)
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
		VLC->identifiers()->requestIdentifier("spell", spell,
		[=](si32 spellID)
		{
			hero->spells.insert(SpellID(spellID));
		});
	}
}

/// creates standard H3 hero specialty for creatures
static std::vector<std::shared_ptr<Bonus>> createCreatureSpecialty(CreatureID baseCreatureID)
{
	std::vector<std::shared_ptr<Bonus>> result;
	std::set<CreatureID> targets;
	targets.insert(baseCreatureID);

	// go through entire upgrade chain and collect all creatures to which baseCreatureID can be upgraded
	for (;;)
	{
		std::set<CreatureID> oldTargets = targets;

		for(const auto & upgradeSourceID : oldTargets)
		{
			const CCreature * upgradeSource = upgradeSourceID.toCreature();
			targets.insert(upgradeSource->upgrades.begin(), upgradeSource->upgrades.end());
		}

		if (oldTargets.size() == targets.size())
			break;
	}

	for(CreatureID cid : targets)
	{
		const auto & specCreature = *cid.toCreature();
		int stepSize = specCreature.getLevel() ? specCreature.getLevel() : 5;

		{
			auto bonus = std::make_shared<Bonus>();
			bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, false));
			bonus->type = BonusType::STACKS_SPEED;
			bonus->val = 1;
			result.push_back(bonus);
		}

		{
			auto bonus = std::make_shared<Bonus>();
			bonus->type = BonusType::PRIMARY_SKILL;
			bonus->subtype = BonusSubtypeID(PrimarySkill::ATTACK);
			bonus->val = 0;
			bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, false));
			bonus->updater.reset(new GrowsWithLevelUpdater(specCreature.getAttack(false), stepSize));
			result.push_back(bonus);
		}

		{
			auto bonus = std::make_shared<Bonus>();
			bonus->type = BonusType::PRIMARY_SKILL;
			bonus->subtype = BonusSubtypeID(PrimarySkill::DEFENSE);
			bonus->val = 0;
			bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, false));
			bonus->updater.reset(new GrowsWithLevelUpdater(specCreature.getDefense(false), stepSize));
			result.push_back(bonus);
		}
	}

	return result;
}

void CHeroHandler::beforeValidate(JsonNode & object)
{
	//handle "base" specialty info
	JsonNode & specialtyNode = object["specialty"];
	if(specialtyNode.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		const JsonNode & base = specialtyNode["base"];
		if(!base.isNull())
		{
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
		}
	}
}

void CHeroHandler::afterLoadFinalization()
{
	for(const auto & functor : callAfterLoadFinalization)
		functor();

	callAfterLoadFinalization.clear();
}

void CHeroHandler::loadHeroSpecialty(CHero * hero, const JsonNode & node)
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
		JsonNode creatureNode = specialtyNode["creature"];

		std::function<void()> specialtyLoader = [creatureNode, hero, prepSpec]
		{
			VLC->identifiers()->requestIdentifier("creature", creatureNode, [hero, prepSpec](si32 creature)
			{
				for (const auto & bonus : createCreatureSpecialty(CreatureID(creature)))
					hero->specialty.push_back(prepSpec(bonus));
			});
		};

		callAfterLoadFinalization.push_back(specialtyLoader);
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
	size_t dataSize = VLC->engineSettings()->getInteger(EGameSettings::TEXTS_HERO);

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

VCMI_LIB_NAMESPACE_END
