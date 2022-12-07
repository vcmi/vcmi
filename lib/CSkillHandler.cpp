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

#include "CGeneralTextHandler.h"
#include "filesystem/Filesystem.h"

#include "JsonNode.h"

#include "CModHandler.h"
#include "StringConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

///CSkill
CSkill::LevelInfo::LevelInfo()
{
}

CSkill::LevelInfo::~LevelInfo()
{
}

CSkill::CSkill(SecondarySkill id, std::string identifier)
	: id(id), identifier(identifier)
{
	gainChance[0] = gainChance[1] = 0; //affects CHeroClassHandler::afterLoadFinalization()
	levels.resize(NSecondarySkill::levels.size() - 1);
}

CSkill::~CSkill()
{
}

int32_t CSkill::getIndex() const
{
	return id.num;
}

int32_t CSkill::getIconIndex() const
{
	return getIndex(); //TODO: actual value with skill level
}

const std::string & CSkill::getName() const
{
	return name;
}

const std::string & CSkill::getJsonKey() const
{
	return identifier;
}

void CSkill::registerIcons(const IconRegistar & cb) const
{
	for(int level = 1; level <= 3; level++)
	{
		int frame = 2 + level + 3 * id;
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
	b->source = Bonus::SECONDARY_SKILL;
	b->sid = id;
	b->duration = Bonus::PERMANENT;
	b->description = name;
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
	out << "(\"" << info.description << "\", [";
	for(int i=0; i < info.effects.size(); i++)
		out << (i ? "," : "") << info.effects[i]->Description();
	return out << "])";
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill & skill)
{
	out << "Skill(" << (int)skill.id << "," << skill.identifier << "): [";
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
CSkillHandler::CSkillHandler()
{
}

std::vector<JsonNode> CSkillHandler::loadLegacyData(size_t dataSize)
{
	CLegacyConfigParser parser("DATA/SSTRAITS.TXT");

	//skip header
	parser.endLine();
	parser.endLine();

	std::vector<std::string> skillNames;
	std::vector<std::vector<std::string>> skillInfoTexts;
	do
	{
		skillNames.push_back(parser.readString());
		skillInfoTexts.push_back(std::vector<std::string>());
		for(int i = 0; i < 3; i++)
			skillInfoTexts.back().push_back(parser.readString());
	}
	while (parser.endLine());

	assert(skillNames.size() == GameConstants::SKILL_QUANTITY);

	//store & construct JSON
	std::vector<JsonNode> legacyData;
	for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
	{
		JsonNode skillNode(JsonNode::JsonType::DATA_STRUCT);
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

const std::string & CSkillHandler::skillInfo(int skill, int level) const
{
	return objects[skill]->at(level).description;
}

const std::string & CSkillHandler::skillName(int skill) const
{
	return objects[skill]->name;
}

CSkill * CSkillHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	CSkill * skill = new CSkill(SecondarySkill((si32)index), identifier);

	skill->name = json["name"].String();
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
		for(auto b : levelNode["effects"].Struct())
		{
			auto bonus = JsonUtils::parseBonus(b.second);
			skill->addNewBonus(bonus, level);
		}
		CSkill::LevelInfo & skillAtLevel = skill->at(level);
		skillAtLevel.description = levelNode["description"].String();
		skillAtLevel.iconSmall = levelNode["images"]["small"].String();
		skillAtLevel.iconMedium = levelNode["images"]["medium"].String();
		skillAtLevel.iconLarge = levelNode["images"]["large"].String();
	}
	logMod->debug("loaded secondary skill %s(%d)", identifier, (int)skill->id);
	logMod->trace("%s", skill->toString());

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

CSkillHandler::~CSkillHandler()
{
}

std::vector<bool> CSkillHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedSkills(objects.size(), true);
	return allowedSkills;
}

si32 CSkillHandler::decodeSkill(const std::string & identifier)
{
	auto rawId = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), "skill", identifier);
	if(rawId)
		return rawId.get();
	else
		return -1;
}

std::string CSkillHandler::encodeSkill(const si32 index)
{
	return (*VLC->skillh)[SecondarySkill(index)]->identifier;
}

std::string CSkillHandler::encodeSkillWithType(const si32 index)
{
	return CModHandler::makeFullIdentifier("", "skill", encodeSkill(index));
}

VCMI_LIB_NAMESPACE_END
