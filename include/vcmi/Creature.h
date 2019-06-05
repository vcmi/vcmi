/*
 * Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"

class CreatureID;

class DLL_LINKAGE Creature : public EntityT<CreatureID>
{
public:
	virtual const std::string & getPluralName() const = 0;
	virtual const std::string & getSingularName() const = 0;
	virtual uint32_t getMaxHealth() const = 0;
};
