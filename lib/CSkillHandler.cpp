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

#include "CStack.h"
#include "battle/BattleInfo.h"
#include "battle/CBattleInfoCallback.h"

///CSkill
CSkill::LevelInfo::LevelInfo() : description("")
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
	for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
	{
		CSkill * skill = new CSkill(SecondarySkill(id));
		for(int level = 1; level < NSecondarySkill::levels.size(); level++)
			for (auto bonus : defaultBonus(SecondarySkill(id), level))
				skill->addNewBonus(bonus, level);
		objects.push_back(skill);
	}
}

std::vector<JsonNode> CSkillHandler::loadLegacyData(size_t dataSize)
{
	std::vector<JsonNode> legacyData;
	/* problem: CGI is client-side only
	for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
	{
		CSkill & skill = *objects[id];
		JsonNode skillNode(JsonNode::DATA_STRUCT);
		for(int level = 1; level < NSecondarySkill::levels.size(); level++)
		{
			//only "real" legacy data is skill description
			std::string desc = CGI->generaltexth->skillInfoTexts[skill.id][level-1];
			//update both skill & JSON
			skill.setDescription(desc, level);
			auto & levelNode = skillNode[NSecondarySkill::levels[level]].Struct();
			levelNode["description"].String() = desc;
		}
		legacyData.push_back(skillNode);
	}
	*/
	return legacyData;
}

const std::string CSkillHandler::getTypeName() const
{
	return "skill";
}

CSkill * CSkillHandler::loadFromJson(const JsonNode & json, const std::string & identifier)
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
		logGlobal->error("unknown secondary skill %s", identifier);
		throw std::runtime_error("invalid skill");
	}

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
		// parse skill description - tracked separately
		if(vstd::contains(levelNode.Struct(), "description") && !levelNode["description"].isNull())
			skill->setDescription(levelNode["description"].String(), level);
	}
	logMod->debug("loaded secondary skill %s(%d)", identifier, (int)skill->id);
	logMod->trace("%s", skill->toString());

	return skill;
}

void CSkillHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto type_name = getTypeName();
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));

	if(object->id == SecondarySkill::DEFAULT) // new skill - no index identified
	{
		object->id = SecondarySkill(objects.size());
		objects.push_back(object);
	}
	else
		objects[object->id] = object;

	registerObject(scope, type_name, name, object->id);
}

void CSkillHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto type_name = getTypeName();
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));

	assert(object->id == index);
	objects[index] = object;

	registerObject(scope, type_name, name, object->id);
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

// HMM3 default bonus provided by secondary skill
std::vector<std::shared_ptr<Bonus>> CSkillHandler::defaultBonus(SecondarySkill skill, int level) const
{
	std::vector<std::shared_ptr<Bonus>> result;

	// add bonus based on current values - useful for adding multiple bonuses easily
	auto addBonus = [=, &result](int bonusVal, Bonus::BonusType bonusType = Bonus::SECONDARY_SKILL_PREMY, int subtype = 0)
	{
		if(bonusType == Bonus::SECONDARY_SKILL_PREMY || bonusType == Bonus::SECONDARY_SKILL_VAL2)
			subtype = skill;
		result.push_back(std::make_shared<Bonus>(Bonus::PERMANENT, bonusType, Bonus::SECONDARY_SKILL, bonusVal, skill, subtype, Bonus::BASE_NUMBER));
	};

	switch(skill)
	{
	case SecondarySkill::PATHFINDING:
		addBonus(25 * level);
		break;
	case SecondarySkill::ARCHERY:
		addBonus(5 + 5 * level * level);
		break;
	case SecondarySkill::LOGISTICS:
		addBonus(10 * level);
		break;
	case SecondarySkill::SCOUTING:
		addBonus(level, Bonus::SIGHT_RADIOUS);
		break;
	case SecondarySkill::DIPLOMACY:
		addBonus(level);
		addBonus(20 * level, Bonus::SURRENDER_DISCOUNT);
		break;
	case SecondarySkill::NAVIGATION:
		addBonus(50 * level);
		break;
	case SecondarySkill::LEADERSHIP:
		addBonus(level, Bonus::MORALE);
		break;
	case SecondarySkill::LUCK:
		addBonus(level, Bonus::LUCK);
		break;
	case SecondarySkill::BALLISTICS:
		addBonus(100, Bonus::MANUAL_CONTROL, CreatureID::CATAPULT);
		addBonus(level);
		break;
	case SecondarySkill::EAGLE_EYE:
		addBonus(30 + 10 * level);
		addBonus(1 + level, Bonus::SECONDARY_SKILL_VAL2);
		break;
	case SecondarySkill::NECROMANCY:
		addBonus(10 * level);
		break;
	case SecondarySkill::ESTATES:
		addBonus(125 << (level-1));
		break;
	case SecondarySkill::FIRE_MAGIC:
	case SecondarySkill::AIR_MAGIC:
	case SecondarySkill::WATER_MAGIC:
	case SecondarySkill::EARTH_MAGIC:
		addBonus(level);
		break;
	case SecondarySkill::SCHOLAR:
		addBonus(1 + level);
		break;
	case SecondarySkill::TACTICS:
		addBonus(1 + 2 * level);
		break;
	case SecondarySkill::ARTILLERY:
		addBonus(100, Bonus::MANUAL_CONTROL, CreatureID::BALLISTA);
		addBonus(25 + 25 * level);
		if(level > 1) // extra attack
			addBonus(1, Bonus::SECONDARY_SKILL_VAL2);
		 break;
	case SecondarySkill::LEARNING:
		addBonus(5 * level);
		break;
	case SecondarySkill::OFFENCE:
		addBonus(10 * level);
		break;
	case SecondarySkill::ARMORER:
		addBonus(5 * level);
		break;
	case SecondarySkill::INTELLIGENCE:
		addBonus(25 << (level-1));
		break;
	case SecondarySkill::SORCERY:
		addBonus(5 * level);
		break;
	case SecondarySkill::RESISTANCE:
		addBonus(5 << (level-1));
		break;
	case SecondarySkill::FIRST_AID:
		addBonus(100, Bonus::MANUAL_CONTROL, CreatureID::FIRST_AID_TENT);
		addBonus(25 + 25 * level);
		break;
	default:
		addBonus(level);
		break;
	}

	return result;
}
