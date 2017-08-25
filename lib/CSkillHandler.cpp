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

void CSkill::addNewBonus(const std::shared_ptr<Bonus>& b, int level)
{
    b->source = Bonus::SECONDARY_SKILL;
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

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const CSkill::LevelInfo &info)
{
    return out << "(\"" << info.description << "\"," << info.effects << ")";
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const CSkill &skill)
{
    return out << "Skill(" << (int)skill.id << "," << skill.identifier << "): " << skill.levels;
}

///CSkillHandler
CSkillHandler::CSkillHandler()
{
    for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
    {
        CSkill * skill = new CSkill(SecondarySkill(id));
        for(int level = 1; level < NSecondarySkill::levels.size(); level++)
            skill->addNewBonus(defaultBonus(SecondarySkill(id), level), level);
        objects.push_back(skill);
    }
}

std::vector<JsonNode> CSkillHandler::loadLegacyData(size_t dataSize)
{
    // not supported - no legacy data to load
	std::vector<JsonNode> legacyData;
	return legacyData;
}

const std::string CSkillHandler::getTypeName() const
{
	return "skill";
}

CSkill * CSkillHandler::loadFromJson(const JsonNode & json, const std::string & identifier)
{
    CSkill * skill = NULL;

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
        logGlobal->errorStream() << "unknown secondary skill " << identifier;
        throw std::runtime_error("invalid skill");
    }

    for(int level = 1; level < NSecondarySkill::levels.size(); level++)
    {
        const std::string & levelName = NSecondarySkill::levels[level]; // basic, advanced, expert
        const JsonNode & levelNode = json[levelName];
        // parse bonus effects
        for(auto b : levelNode["effects"].Vector())
        {
            auto bonus = JsonUtils::parseBonus(b);
            bonus->sid = skill->id;
            skill->addNewBonus(bonus, level);
        }
        // parse skill description - tracked separately
        if(vstd::contains(levelNode.Struct(), "description") && !levelNode["description"].isNull())
            //CGI->generaltexth->skillInfoTexts[skill->id][level-1] = levelNode["description"].String();
            skill->setDescription(levelNode["description"].String(), level);
    }
    CLogger * logger = CLogger::getLogger(CLoggerDomain(getTypeName()));
    logger->debugStream() << "loaded secondary skill " << identifier << "(" << (int)skill->id << ")";
    logger->traceStream() << *skill;

    return skill;
}

void CSkillHandler::afterLoadFinalization()
{
    CLogger * logger = CLogger::getLogger(CLoggerDomain(getTypeName()));
    logger->traceStream() << "skill handler after load: ";
    for(auto skill : objects)
        logger->traceStream() << *skill;
}

void CSkillHandler::beforeValidate(JsonNode & object)
{
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
const std::shared_ptr<Bonus> CSkillHandler::defaultBonus(SecondarySkill skill, int level) const
{
	Bonus::BonusType bonusType = Bonus::SECONDARY_SKILL_PREMY;
	Bonus::ValueType valueType = Bonus::BASE_NUMBER;
	int bonusVal = level;

	static const int archery_bonus[] = { 10, 25, 50 };
	switch (skill)
	{
	case SecondarySkill::LEADERSHIP:
		bonusType = Bonus::MORALE; break;
	case SecondarySkill::LUCK:
		bonusType = Bonus::LUCK; break;
	case SecondarySkill::DIPLOMACY:
		bonusType = Bonus::SURRENDER_DISCOUNT;
		bonusVal = 20 * level; break;
	case SecondarySkill::ARCHERY:
		bonusVal = archery_bonus[level-1]; break;
	case SecondarySkill::LOGISTICS:
		bonusVal = 10 * level; break;
	case SecondarySkill::NAVIGATION:
		bonusVal = 50 * level; break;
	case SecondarySkill::MYSTICISM:
		bonusVal = level; break;
	case SecondarySkill::EAGLE_EYE:
		bonusVal = 30 + 10 * level; break;
	case SecondarySkill::NECROMANCY:
		bonusVal = 10 * level; break;
	case SecondarySkill::LEARNING:
		bonusVal = 5 * level; break;
	case SecondarySkill::OFFENCE:
		bonusVal = 10 * level; break;
	case SecondarySkill::ARMORER:
		bonusVal = 5 * level; break;
	case SecondarySkill::INTELLIGENCE:
		bonusVal = 25 << (level-1); break;
	case SecondarySkill::SORCERY:
		bonusVal = 5 * level; break;
	case SecondarySkill::RESISTANCE:
		bonusVal = 5 << (level-1); break;
	case SecondarySkill::FIRST_AID:
		bonusVal = 25 + 25 * level; break;
	case SecondarySkill::ESTATES:
		bonusVal = 125 << (level-1); break;
	default:
		valueType = Bonus::INDEPENDENT_MIN; break;
	}

	return std::make_shared<Bonus>(Bonus::PERMANENT, bonusType, Bonus::SECONDARY_SKILL, bonusVal, skill, skill, valueType);
}
