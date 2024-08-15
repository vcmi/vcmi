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

#include "filesystem/Filesystem.h"
#include "VCMI_Lib.h"
#include "constants/StringConstants.h"
#include "battle/BattleHex.h"
#include "CCreatureHandler.h"
#include "GameSettings.h"
#include "CSkillHandler.h"
#include "BattleFieldHandler.h"
#include "bonuses/Limiters.h"
#include "bonuses/Updaters.h"
#include "entities/faction/CFaction.h"
#include "entities/faction/CTown.h"
#include "entities/faction/CTownHandler.h"
#include "json/JsonBonus.h"
#include "json/JsonUtils.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "modding/IdentifierStorage.h"
#include "texts/CGeneralTextHandler.h"
#include "texts/CLegacyConfigParser.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

CHero::CHero() = default;
CHero::~CHero() = default;

int32_t CHero::getIndex() const
{
	return ID.getNum();
}

int32_t CHero::getIconIndex() const
{
	return imageIndex;
}

std::string CHero::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CHero::getModScope() const
{
	return modScope;
}

HeroTypeID CHero::getId() const
{
	return ID;
}

std::string CHero::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CHero::getBiographyTranslated() const
{
	return VLC->generaltexth->translate(getBiographyTextID());
}

std::string CHero::getSpecialtyNameTranslated() const
{
	return VLC->generaltexth->translate(getSpecialtyNameTextID());
}

std::string CHero::getSpecialtyDescriptionTranslated() const
{
	return VLC->generaltexth->translate(getSpecialtyDescriptionTextID());
}

std::string CHero::getSpecialtyTooltipTranslated() const
{
	return VLC->generaltexth->translate(getSpecialtyTooltipTextID());
}

std::string CHero::getNameTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "name").get();
}

std::string CHero::getBiographyTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "biography").get();
}

std::string CHero::getSpecialtyNameTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "name").get();
}

std::string CHero::getSpecialtyDescriptionTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "description").get();
}

std::string CHero::getSpecialtyTooltipTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "tooltip").get();
}

void CHero::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), 0, "UN32", iconSpecSmall);
	cb(getIconIndex(), 0, "UN44", iconSpecLarge);
	cb(getIconIndex(), 0, "PORTRAITSLARGE", portraitLarge);
	cb(getIconIndex(), 0, "PORTRAITSSMALL", portraitSmall);
}

void CHero::updateFrom(const JsonNode & data)
{
	//todo: CHero::updateFrom
}

void CHero::serializeJson(JsonSerializeFormat & handler)
{

}


SecondarySkill CHeroClass::chooseSecSkill(const std::set<SecondarySkill> & possibles, vstd::RNG & rand) const //picks secondary skill out from given possibilities
{
	assert(!possibles.empty());

	if (possibles.size() == 1)
		return *possibles.begin();

	int totalProb = 0;
	for(const auto & possible : possibles)
		if (secSkillProbability.count(possible) != 0)
			totalProb += secSkillProbability.at(possible);

	if (totalProb == 0) // may trigger if set contains only banned skills (0 probability)
		return *RandomGeneratorUtil::nextItem(possibles, rand);

	auto ran = rand.nextInt(totalProb - 1);
	for(const auto & possible : possibles)
	{
		if (secSkillProbability.count(possible) != 0)
			ran -= secSkillProbability.at(possible);

		if(ran < 0)
			return possible;
	}

	assert(0); // should not be possible
	return *possibles.begin();
}

bool CHeroClass::isMagicHero() const
{
	return affinity == MAGIC;
}

int CHeroClass::tavernProbability(FactionID targetFaction) const
{
	auto it = selectionProbability.find(targetFaction);
	if (it != selectionProbability.end())
		return it->second;
	return 0;
}

EAlignment CHeroClass::getAlignment() const
{
	return VLC->factions()->getById(faction)->getAlignment();
}

int32_t CHeroClass::getIndex() const
{
	return id.getNum();
}

int32_t CHeroClass::getIconIndex() const
{
	return getIndex();
}

std::string CHeroClass::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CHeroClass::getModScope() const
{
	return modScope;
}

HeroClassID CHeroClass::getId() const
{
	return id;
}

void CHeroClass::registerIcons(const IconRegistar & cb) const
{

}

std::string CHeroClass::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CHeroClass::getNameTextID() const
{
	return TextIdentifier("heroClass", modScope, identifier, "name").get();
}

void CHeroClass::updateFrom(const JsonNode & data)
{
	//TODO: CHeroClass::updateFrom
}

void CHeroClass::serializeJson(JsonSerializeFormat & handler)
{

}

CHeroClass::CHeroClass():
	faction(0),
	affinity(0),
	defaultTavernChance(0)
{
}

void CHeroClassHandler::fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill pSkill) const
{
	const auto & skillName = NPrimarySkill::names[pSkill.getNum()];
	auto currentPrimarySkillValue = static_cast<int>(node["primarySkills"][skillName].Integer());
	int primarySkillLegalMinimum = VLC->settings()->getVector(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS)[pSkill.getNum()];

	if(currentPrimarySkillValue < primarySkillLegalMinimum)
	{
		logMod->error("Hero class '%s' has incorrect initial value '%d' for skill '%s'. Value '%d' will be used instead.",
			heroClass->getNameTranslated(), currentPrimarySkillValue, skillName, primarySkillLegalMinimum);
		currentPrimarySkillValue = primarySkillLegalMinimum;
	}
	heroClass->primarySkillInitial.push_back(currentPrimarySkillValue);
	heroClass->primarySkillLowLevel.push_back(static_cast<int>(node["lowLevelChance"][skillName].Float()));
	heroClass->primarySkillHighLevel.push_back(static_cast<int>(node["highLevelChance"][skillName].Float()));
}

const std::vector<std::string> & CHeroClassHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "heroClass" };
	return typeNames;
}

std::shared_ptr<CHeroClass> CHeroClassHandler::loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	std::string affinityStr[2] = { "might", "magic" };

	auto heroClass = std::make_shared<CHeroClass>();

	heroClass->id = HeroClassID(index);
	heroClass->identifier = identifier;
	heroClass->modScope = scope;
	heroClass->imageBattleFemale = AnimationPath::fromJson(node["animation"]["battle"]["female"]);
	heroClass->imageBattleMale   = AnimationPath::fromJson(node["animation"]["battle"]["male"]);
	//MODS COMPATIBILITY FOR 0.96
	heroClass->imageMapFemale    = node["animation"]["map"]["female"].String();
	heroClass->imageMapMale      = node["animation"]["map"]["male"].String();

	VLC->generaltexth->registerString(scope, heroClass->getNameTextID(), node["name"].String());

	if (vstd::contains(affinityStr, node["affinity"].String()))
	{
		heroClass->affinity = vstd::find_pos(affinityStr, node["affinity"].String());
	}
	else
	{
		logGlobal->error("Mod '%s', hero class '%s': invalid affinity '%s'! Expected 'might' or 'magic'!", scope, identifier, node["affinity"].String());
		heroClass->affinity = CHeroClass::MIGHT;
	}

	fillPrimarySkillData(node, heroClass.get(), PrimarySkill::ATTACK);
	fillPrimarySkillData(node, heroClass.get(), PrimarySkill::DEFENSE);
	fillPrimarySkillData(node, heroClass.get(), PrimarySkill::SPELL_POWER);
	fillPrimarySkillData(node, heroClass.get(), PrimarySkill::KNOWLEDGE);

	auto percentSumm = std::accumulate(heroClass->primarySkillLowLevel.begin(), heroClass->primarySkillLowLevel.end(), 0);
	if(percentSumm <= 0)
		logMod->error("Hero class %s has wrong lowLevelChance values: must be above zero!", heroClass->identifier, percentSumm);

	percentSumm = std::accumulate(heroClass->primarySkillHighLevel.begin(), heroClass->primarySkillHighLevel.end(), 0);
	if(percentSumm <= 0)
		logMod->error("Hero class %s has wrong highLevelChance values: must be above zero!", heroClass->identifier, percentSumm);

	for(auto skillPair : node["secondarySkills"].Struct())
	{
		int probability = static_cast<int>(skillPair.second.Integer());
		VLC->identifiers()->requestIdentifier(skillPair.second.getModScope(), "skill", skillPair.first, [heroClass, probability](si32 skillID)
		{
			heroClass->secSkillProbability[skillID] = probability;
		});
	}

	VLC->identifiers()->requestIdentifier ("creature", node["commander"],
	[=](si32 commanderID)
	{
		heroClass->commander = CreatureID(commanderID);
	});

	heroClass->defaultTavernChance = static_cast<ui32>(node["defaultTavern"].Float());
	for(const auto & tavern : node["tavern"].Struct())
	{
		int value = static_cast<int>(tavern.second.Float());

		VLC->identifiers()->requestIdentifier(tavern.second.getModScope(), "faction", tavern.first,
		[=](si32 factionID)
		{
			heroClass->selectionProbability[FactionID(factionID)] = value;
		});
	}

	VLC->identifiers()->requestIdentifier("faction", node["faction"],
	[=](si32 factionID)
	{
		heroClass->faction.setNum(factionID);
	});

	VLC->identifiers()->requestIdentifier(scope, "object", "hero", [=](si32 index)
	{
		JsonNode classConf = node["mapObject"];
		classConf["heroClass"].String() = identifier;
		if (!node["compatibilityIdentifiers"].isNull())
			classConf["compatibilityIdentifiers"] = node["compatibilityIdentifiers"];
		classConf.setModScope(scope);
		VLC->objtypeh->loadSubObject(identifier, classConf, index, heroClass->getIndex());
	});

	return heroClass;
}

std::vector<JsonNode> CHeroClassHandler::loadLegacyData()
{
	size_t dataSize = VLC->settings()->getInteger(EGameSettings::TEXTS_HERO_CLASS);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser parser(TextPath::builtin("DATA/HCTRAITS.TXT"));

	parser.endLine(); // header
	parser.endLine();

	for (size_t i=0; i<dataSize; i++)
	{
		JsonNode entry;

		entry["name"].String() = parser.readString();

		parser.readNumber(); // unused aggression

		for(const auto & name : NPrimarySkill::names)
			entry["primarySkills"][name].Float() = parser.readNumber();

		for(const auto & name : NPrimarySkill::names)
			entry["lowLevelChance"][name].Float() = parser.readNumber();

		for(const auto & name : NPrimarySkill::names)
			entry["highLevelChance"][name].Float() = parser.readNumber();

		for(const auto & name : NSecondarySkill::names)
			entry["secondarySkills"][name].Float() = parser.readNumber();

		for(const auto & name : NFaction::names)
			entry["tavern"][name].Float() = parser.readNumber();

		parser.endLine();
		h3Data.push_back(entry);
	}
	return h3Data;
}

void CHeroClassHandler::afterLoadFinalization()
{
	// for each pair <class, town> set selection probability if it was not set before in tavern entries
	for(auto & heroClass : objects)
	{
		for(auto & faction : VLC->townh->objects)
		{
			if (!faction->town)
				continue;
			if (heroClass->selectionProbability.count(faction->getId()))
				continue;

			auto chance = static_cast<float>(heroClass->defaultTavernChance * faction->town->defaultTavernChance);
			heroClass->selectionProbability[faction->getId()] = static_cast<int>(sqrt(chance) + 0.5); //FIXME: replace with std::round once MVS supports it
		}

		// set default probabilities for gaining secondary skills where not loaded previously
		for(int skillID = 0; skillID < VLC->skillh->size(); skillID++)
		{
			if(heroClass->secSkillProbability.count(skillID) == 0)
			{
				const CSkill * skill = (*VLC->skillh)[SecondarySkill(skillID)];
				logMod->trace("%s: no probability for %s, using default", heroClass->identifier, skill->getJsonKey());
				heroClass->secSkillProbability[skillID] = skill->gainChance[heroClass->affinity];
			}
		}
	}

	for(const auto & hc : objects)
	{
		if(!hc->imageMapMale.empty())
		{
			JsonNode templ;
			templ["animation"].String() = hc->imageMapMale;
			VLC->objtypeh->getHandlerFor(Obj::HERO, hc->getIndex())->addTemplate(templ);
		}
	}
}

CHeroClassHandler::~CHeroClassHandler() = default;

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

	VLC->generaltexth->registerString(scope, hero->getNameTextID(), node["texts"]["name"].String());
	VLC->generaltexth->registerString(scope, hero->getBiographyTextID(), node["texts"]["biography"].String());
	VLC->generaltexth->registerString(scope, hero->getSpecialtyNameTextID(), node["texts"]["specialty"]["name"].String());
	VLC->generaltexth->registerString(scope, hero->getSpecialtyTooltipTextID(), node["texts"]["specialty"]["tooltip"].String());
	VLC->generaltexth->registerString(scope, hero->getSpecialtyDescriptionTextID(), node["texts"]["specialty"]["description"].String());

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
	size_t dataSize = VLC->settings()->getInteger(EGameSettings::TEXTS_HERO);

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
