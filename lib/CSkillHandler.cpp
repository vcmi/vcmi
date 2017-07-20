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

///CSkill
CSkill::LevelInfo::LevelInfo()
{
}

CSkill::LevelInfo::~LevelInfo()
{
}

CSkill::CSkill(SecondarySkill id) : id(id)
{
	if(id == SecondarySkill::DEFAULT)
		identifier = "default";
	else
		identifier = NSecondarySkill::names[id];
	// init levels
	LevelInfo emptyLevel;
	for(int level = 1; level < NSecondarySkill::levels.size(); level++)
		levels.push_back(emptyLevel);
}

CSkill::~CSkill()
{
}

void CSkill::addNewBonus(const std::shared_ptr<Bonus> & b, int level)
{
	b->source = Bonus::SECONDARY_SKILL;
	b->sid = id;
	b->duration = Bonus::PERMANENT;
	b->description = identifier;
	levels[level-1].effects.push_back(b);
}

void CSkill::setDescription(const std::string & desc, int level)
{
	levels[level-1].description = desc;
}

const std::vector<std::shared_ptr<Bonus>> & CSkill::getBonus(int level) const
{
	return levels[level-1].effects;
}

const std::string & CSkill::getDescription(int level) const
{
	return levels[level-1].description;
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

const std::string CSkillHandler::getTypeName() const
{
	return "skill";
}

const std::string & CSkillHandler::skillInfo(int skill, int level) const
{
	return objects[skill]->getDescription(level);
}

const std::string & CSkillHandler::skillName(int skill) const
{
	return objects[skill]->name;
}

CSkill * CSkillHandler::loadFromJson(const JsonNode & json, const std::string & identifier, size_t index)
{
	CSkill * skill = nullptr;

	for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
	{
		if(NSecondarySkill::names[id].compare(identifier) == 0)
		{
			skill = new CSkill(SecondarySkill(id));
			break;
		}
	}

	if(!skill)
	{
		logMod->error("unknown secondary skill %s", identifier);
		throw std::runtime_error("invalid skill");
	}

	skill->name = json["name"].String();
	for(int level = 1; level < NSecondarySkill::levels.size(); level++)
	{
		const std::string & levelName = NSecondarySkill::levels[level]; // basic, advanced, expert
		const JsonNode & levelNode = json[levelName];
		// parse bonus effects
		for(auto b : levelNode["effects"].Struct())
		{
			auto bonus = JsonUtils::parseBonus(b.second);
			bonus->sid = skill->id;
			skill->addNewBonus(bonus, level);
		}
		skill->setDescription(levelNode["description"].String(), level);
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
