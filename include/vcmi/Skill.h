/*
 * Skill.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"

VCMI_LIB_NAMESPACE_BEGIN

class SecondarySkill;

class DLL_LINKAGE Skill : public EntityT<SecondarySkill>
{
public:
	virtual std::string getDescriptionTextID(int level) const = 0;
	virtual std::string getDescriptionTranslated(int level) const = 0;
};


VCMI_LIB_NAMESPACE_END
