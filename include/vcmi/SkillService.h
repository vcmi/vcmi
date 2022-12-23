/*
 * SkillService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "EntityService.h"

VCMI_LIB_NAMESPACE_BEGIN

class SecondarySkill;
class Skill;

class DLL_LINKAGE SkillService : public EntityServiceT<SecondarySkill, Skill>
{
public:
};

VCMI_LIB_NAMESPACE_END
