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
CSkill::CSkill()
{
    BonusList emptyList;
    for(auto level : NSecondarySkill::levels)
        bonusByLevel.push_back(emptyList);
}

CSkill::~CSkill()
{
}

void CSkill::addNewBonus(const std::shared_ptr<Bonus>& b, int level)
{
    b->source = Bonus::SECONDARY_SKILL;
    b->duration = Bonus::PERMANENT;
    b->description = identifier;
    bonusByLevel[level].push_back(b);
}

BonusList CSkill::getBonus(int level)
{
    return bonusByLevel[level];
}

///CSkillHandler
CSkillHandler::CSkillHandler()
{
}

std::vector<JsonNode> CSkillHandler::loadLegacyData(size_t dataSize)
{
    // not supported - no legacy data to load
	std::vector<JsonNode> legacyData;
	return legacyData;
}

const std::string CSkillHandler::getTypeName() const
{
    return "secondarySkill";
}

CSkill * CSkillHandler::loadFromJson(const JsonNode & json, const std::string & identifier)
{
    CSkill * skill = new CSkill();
    skill->identifier = identifier;

    skill->id = SecondarySkill::DEFAULT;
    for(int id = 0; id < GameConstants::SKILL_QUANTITY; id++)
    {
        if(NSecondarySkill::names[id].compare(identifier) == 0)
        {
            skill->id = SecondarySkill(id);
            break;
        }
    }

    for(int level = 1; level < NSecondarySkill::levels.size(); level++)
    {
        const std::string & levelName = NSecondarySkill::levels[level]; // basic, advanced, expert
        for(auto b : json[levelName].Vector())
        {
            auto bonus = JsonUtils::parseBonus(b);
            bonus->sid = skill->id;
            skill->addNewBonus(bonus, level);
        }
    }

    return skill;
}

void CSkillHandler::afterLoadFinalization()
{
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
