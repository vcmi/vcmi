/*
 * spells/Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class SpellID;

namespace spells
{
class Spell;

class DLL_LINKAGE Service
{
public:
	virtual ~Service() = default;

 	virtual const Spell * getSpell(const SpellID & spellID) const = 0;
};

}
