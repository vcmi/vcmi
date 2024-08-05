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

int getPrimarySkillMinimum(PrimarySkill pSkill)
{
    if (pSkill == PrimarySkill::ATTACK)
        return VLC->settings()->getInteger(EGameSettings::HEROES_MINIMAL_ATTACK);
    else if (pSkill == PrimarySkill::DEFENSE)
        return VLC->settings()->getInteger(EGameSettings::HEROES_MINIMAL_DEFENCE);
    else if (pSkill == PrimarySkill::SPELL_POWER)
        return VLC->settings()->getInteger(EGameSettings::HEROES_MINIMAL_SPELL_POWER);
    else
        return VLC->settings()->getInteger(EGameSettings::HEROES_MINIMAL_KNOWLEDGE);
}

VCMI_LIB_NAMESPACE_END