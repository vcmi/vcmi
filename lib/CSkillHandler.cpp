/*
 * CSkillHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include <cctype>

#include "CSkillHandler.h"

#include "constants/StringConstants.h"
#include "filesystem/Filesystem.h"
#include "json/JsonBonus.h"
#include "json/JsonUtils.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModUtility.h"
#include "modding/ModScope.h"
#include "texts/CGeneralTextHandler.h"
#include "texts/CLegacyConfigParser.h"
#include "texts/TextOperations.h"
#include "VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

CSkill::CSkill(const SecondarySkill & id, std::string identifier, bool obligatoryMajor, bool obligatoryMinor):
	id(id),
	identifier(std::move(identifier)),
	obligatoryMajor(obligatoryMajor),
	obligatoryMinor(obligatoryMinor)
{
	gainChance[0] = gainChance[1] = 0; //affects CHeroClassHandler::afterLoadFinalization()
	levels.resize(NSecondarySkill::levels.size() - 1);
}

int32_t CSkill::getIndex() const
{
	return id.num;
}

int32_t CSkill::getIconIndex() const
{
	return getIndex(); //TODO: actual value with skill level
}

std::string CSkill::getNameTextID() const
{
	TextIdentifier id("skill", modScope, identifier, "name");
	return id.get();
}

std::string CSkill::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CSkill::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CSkill::getModScope() const
{
	return modScope;
}

std::string CSkill::getDescriptionTextID(int level) const
{
	TextIdentifier id("skill", modScope, identifier, "description", NSecondarySkill::levels[level]);
	return id.get();
}

std::string CSkill::getDescriptionTranslated(int level) const
{
	return VLC->generaltexth->translate(getDescriptionTextID(level));
}

void CSkill::registerIcons(const IconRegistar & cb) const
{
	for(int level = 1; level <= 3; level++)
	{
		int frame = 2 + level + 3 * id.getNum();
		const LevelInfo & skillAtLevel = at(level);
		cb(frame, 0, "SECSK32", skillAtLevel.iconSmall);
		cb(frame, 0, "SECSKILL", skillAtLevel.iconMedium);
		cb(frame, 0, "SECSK82", skillAtLevel.iconLarge);
	}
}

SecondarySkill CSkill::getId() const
{
	return id;
}

void CSkill::addNewBonus(const std::shared_ptr<Bonus> & b, int level)
{
	b->source = BonusSource::SECONDARY_SKILL;
	b->sid = BonusSourceID(id);
	b->duration = BonusDuration::PERMANENT;
	b->description.appendTextID(getNameTextID());
	b->description.appendRawString(" %+d");
	levels[level-1].effects.push_back(b);
}

const CSkill::LevelInfo & CSkill::at(int level) const
{
	assert(1 <= level && level < NSecondarySkill::levels.size());
	return levels[level - 1];
}

CSkill::LevelInfo & CSkill::at(int level)
{
	assert(1 <= level && level < NSecondarySkill::levels.size());
	return levels[level - 1];
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill::LevelInfo & info)
{
	for(int i=0; i < info.effects.size(); i++)
		out << (i ? "," : "") << info.effects[i]->Description();
	return out << "])";
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill & skill)
{
	out << "Skill(" << skill.id.getNum() << "," << skill.identifier << "): [";
	for(int i=0; i < skill.levels.size(); i++)
		out << (i ? "," : "") << skill.levels[i];
	return out << "]";
}

std::string CSkill::toString() const
{
	std::ostringstream ss;
	ss << *this;
	return ss.str();
}

void CSkill::updateFrom(const JsonNode & data)
{

}

void CSkill::serializeJson(JsonSerializeFormat & handler)
{

}

///CSkillHandler
std::vector<JsonNode> CSkillHandler::loadLegacyData()
{
	CLegacyConfigParser parser(TextPath::builtin("DATA/SSTRAITS.TXT"));

	//skip header
	parser.endLine();
	parser.endLine();

	std::vector<std::string> skillNames;
	std::vector<std::vector<std::string>> skillInfoTexts;
	do
	{
		skillNames.push_back(parser.readString());
		skillInfoTexts.emplace_back();
		for(int i = 0; i < 3; i++)
			skillInfoTexts.back().push_back(parser.readString());
	}
	while (parser.endLine());

	assert(skillNames.size() == GameConstants::SKILL_QUANTITY);

	//store & construct JSON
	std::vector<JsonNode> legacyData;
	for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
	{
		JsonNode skillNode;
		skillNode["name"].String() = skillNames[id];
		for(int level = 1; level < NSecondarySkill::levels.size(); level++)
		{
			std::string & desc = skillInfoTexts[id][level-1];
			auto & levelNode = skillNode[NSecondarySkill::levels[level]].Struct();
			levelNode["description"].String() = desc;
			levelNode["effects"].Struct(); // create empty effects objects
		}
		legacyData.push_back(skillNode);
	}
	objects.resize(legacyData.size());
	return legacyData;
}

const std::vector<std::string> & CSkillHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "skill", "secondarySkill" };
	return typeNames;
}

std::shared_ptr<CSkill> CSkillHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());
	bool major;
	bool minor;

	major = json["obligatoryMajor"].Bool();
	minor = json["obligatoryMinor"].Bool();
	auto skill = std::make_shared<CSkill>(SecondarySkill(index), identifier, major, minor);
	skill->modScope = scope;

	skill->onlyOnWaterMap = json["onlyOnWaterMap"].Bool();

	VLC->generaltexth->registerString(scope, skill->getNameTextID(), json["name"].String());
	switch(json["gainChance"].getType())
	{
	case JsonNode::JsonType::DATA_INTEGER:
		skill->gainChance[0] = static_cast<si32>(json["gainChance"].Integer());
		skill->gainChance[1] = static_cast<si32>(json["gainChance"].Integer());
		break;
	case JsonNode::JsonType::DATA_STRUCT:
		skill->gainChance[0] = static_cast<si32>(json["gainChance"]["might"].Integer());
		skill->gainChance[1] = static_cast<si32>(json["gainChance"]["magic"].Integer());
		break;
	default:
		break;
	}
	for(int level = 1; level < NSecondarySkill::levels.size(); level++)
	{
		const std::string & levelName = NSecondarySkill::levels[level]; // basic, advanced, expert
		const JsonNode & levelNode = json[levelName];
		// parse bonus effects
		for(const auto & b : levelNode["effects"].Struct())
		{
			auto bonus = JsonUtils::parseBonus(b.second);
			skill->addNewBonus(bonus, level);
		}
		CSkill::LevelInfo & skillAtLevel = skill->at(level);
		VLC->generaltexth->registerString(scope, skill->getDescriptionTextID(level), levelNode["description"].String());
		skillAtLevel.iconSmall = levelNode["images"]["small"].String();
		skillAtLevel.iconMedium = levelNode["images"]["medium"].String();
		skillAtLevel.iconLarge = levelNode["images"]["large"].String();
	}
	logMod->debug("loaded secondary skill %s(%d)", identifier, skill->id.getNum());

	return skill;
}

void CSkillHandler::afterLoadFinalization()
{
}

void CSkillHandler::beforeValidate(JsonNode & object)
{
	//handle "base" level info
	JsonNode & base = object["base"];

	auto inheritNode = [&](const std::string & name){
		JsonUtils::inherit(object[name], base);
	};

	inheritNode("basic");
	inheritNode("advanced");
	inheritNode("expert");
}

std::set<SecondarySkill> CSkillHandler::getDefaultAllowed() const
{
	std::set<SecondarySkill> result;

	for (auto const & skill : objects)
		result.insert(skill->getId());

	return result;
}

VCMI_LIB_NAMESPACE_END
