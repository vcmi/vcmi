/*
 * MinimalPrimarySkill.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MinimalPrimarySkill.h"
#include "VCMI_Lib.h"
#include "GameSettings.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::vector<int> DEFAULT_MINIMAL_PSKILLS= {0, 0, 1, 1};
const std::map<PrimarySkill, int> PSKILL_INDEX_MAP = {
        {PrimarySkill::ATTACK, 0},
        {PrimarySkill::DEFENSE, 1},
        {PrimarySkill::SPELL_POWER, 2},
        {PrimarySkill::KNOWLEDGE, 3}
};

int getPrimarySkillMinimum(PrimarySkill pSkill)
{
    auto minialPSkills = VLC->settings()->getVector(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS);
    if(minialPSkills.size() != DEFAULT_MINIMAL_PSKILLS.size())
        logGlobal->error("gameConfig.json: heroes/minimalPrimarySkills format error. need a vector with 4 elements.");
    int index = PSKILL_INDEX_MAP.at(pSkill);
    return minialPSkills.size() > index ? minialPSkills[index] : DEFAULT_MINIMAL_PSKILLS[index];
}

VCMI_LIB_NAMESPACE_END