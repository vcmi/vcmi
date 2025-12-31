/*
 * CHeroClassHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroClassHandler.h"

#include "CHeroClass.h"

#include "../faction/CTown.h"
#include "../faction/CTownHandler.h"

#include "../../CSkillHandler.h"
#include "../../IGameSettings.h"
#include "../../GameLibrary.h"
#include "../../constants/StringConstants.h"
#include "../../json/JsonNode.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../modding/IdentifierStorage.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/CLegacyConfigParser.h"

VCMI_LIB_NAMESPACE_BEGIN

void CHeroClassHandler::fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill pSkill) const
{
	const auto & skillName = NPrimarySkill::names[pSkill.getNum()];
	auto currentPrimarySkillValue = static_cast<int>(node["primarySkills"][skillName].Integer());
	int primarySkillLegalMinimum = LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, pSkill.getNum());

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

	LIBRARY->generaltexth->registerString(scope, heroClass->getNameTextID(), node["name"].String());

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
		LIBRARY->identifiers()->requestIdentifierIfFound(skillPair.second.getModScope(), "skill", skillPair.first, [heroClass, probability](si32 skillID) {
			heroClass->secSkillProbability[skillID] = probability;
		});
	}

	LIBRARY->identifiers()->requestIdentifier ("creature", node["commander"], [=](si32 commanderID) {
			heroClass->commander = CreatureID(commanderID);
		});

	heroClass->defaultTavernChance = static_cast<ui32>(node["defaultTavern"].Float());
	for(const auto & tavern : node["tavern"].Struct())
	{
		int value = static_cast<int>(tavern.second.Float());

		LIBRARY->identifiers()->requestIdentifierIfFound(tavern.second.getModScope(), "faction", tavern.first, [=](si32 factionID) {
			heroClass->selectionProbability[FactionID(factionID)] = value;
		});
	}

	LIBRARY->identifiers()->requestIdentifier("faction", node["faction"], [=](si32 factionID) {
		heroClass->faction.setNum(factionID);
	});

	LIBRARY->identifiers()->requestIdentifier(scope, "object", "hero", [=](si32 index) {
		JsonNode classConf = node["mapObject"];
		classConf["heroClass"].String() = identifier;
		classConf["heroClass"].setModScope(scope);
		if (!node["compatibilityIdentifiers"].isNull())
			classConf["compatibilityIdentifiers"] = node["compatibilityIdentifiers"];
		LIBRARY->objtypeh->loadSubObject(identifier, classConf, index, heroClass->getIndex());
	});

	return heroClass;
}

std::vector<JsonNode> CHeroClassHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_HERO_CLASS);

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
		for(auto & faction : LIBRARY->townh->objects)
		{
			if (!faction->town)
				continue;
			if (heroClass->selectionProbability.count(faction->getId()))
				continue;

			auto chance = static_cast<float>(heroClass->defaultTavernChance * faction->town->defaultTavernChance);
			heroClass->selectionProbability[faction->getId()] = static_cast<int>(sqrt(chance) + 0.5); //FIXME: replace with std::round once MVS supports it
		}

		// set default probabilities for gaining secondary skills where not loaded previously
		for(int skillID = 0; skillID < LIBRARY->skillh->size(); skillID++)
		{
			if(heroClass->secSkillProbability.count(skillID) == 0)
			{
				const CSkill * skill = (*LIBRARY->skillh)[SecondarySkill(skillID)];
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
			LIBRARY->objtypeh->getHandlerFor(Obj::HERO, hc->getIndex())->addTemplate(templ);
		}
	}
}

CHeroClassHandler::~CHeroClassHandler() = default;

VCMI_LIB_NAMESPACE_END
