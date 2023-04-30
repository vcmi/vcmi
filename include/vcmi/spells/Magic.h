/*
 * spells/Magic.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct MetaString;

namespace battle
{
	class Unit;
	class Destination;
}

namespace spells
{
class Caster;
class Spell;
class Mechanics;
class BattleCast;
using Destination = battle::Destination;

using Target = std::vector<Destination>;

enum class Mode
{
	//ACTIVE, //todo: use
	HERO, //deprecated
	MAGIC_MIRROR,
	CREATURE_ACTIVE, //deprecated
	ENCHANTER,
	SPELL_LIKE_ATTACK,
	PASSIVE//f.e. opening battle spells
};

enum class AimType
{
	NO_TARGET,
	CREATURE,
	OBSTACLE,
	LOCATION
};

class DLL_LINKAGE Problem
{
public:
	using Severity = int;

	enum ESeverity
	{
		LOWEST = std::numeric_limits<Severity>::min(),
		NORMAL = 0,
		CRITICAL = std::numeric_limits<Severity>::max()
	};

	virtual ~Problem() = default;

	virtual void add(MetaString && description, Severity severity = CRITICAL) = 0;

	virtual void getAll(std::vector<std::string> & target) const = 0;
};

}

VCMI_LIB_NAMESPACE_END
