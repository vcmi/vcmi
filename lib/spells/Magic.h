/*
 * Magic.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

/**
 * High-level interface for spells subsystem
 */


class CSpell;
class PlayerColor;
struct MetaString;
struct CPackForClient;

namespace battle
{
	class Unit;
	class Destination;
}

namespace spells
{

class Mechanics;
class BattleCast;
using Destination = ::battle::Destination;

using Target = std::vector<Destination>;

struct SchoolInfo;

enum class Mode
{
	//ACTIVE, //todo: use
	HERO, //deprecated
	AFTER_ATTACK,
	BEFORE_ATTACK,
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

class DLL_LINKAGE PacketSender
{
public:
	virtual ~PacketSender(){};
	virtual void sendAndApply(CPackForClient * info) const = 0;
	virtual void complain(const std::string & problem) const = 0;
};

class DLL_LINKAGE Problem
{
public:
	typedef int Severity;

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

class DLL_LINKAGE Spell
{
public:
	virtual ~Spell() = default;

	virtual int32_t getIndex() const = 0;

	virtual void forEachSchool(const std::function<void (const SchoolInfo &, bool &)> & cb) const = 0;
};

class DLL_LINKAGE Caster
{
public:
	virtual ~Caster() = default;

	/// returns level on which given spell would be cast by this(0 - none, 1 - basic etc);
	/// caster may not know this spell at all
	/// optionally returns number of selected school by arg - 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic
	virtual ui8 getSpellSchoolLevel(const Spell * spell, int * outSelectedSchool = nullptr) const = 0;

	///default spell school level for effect calculation
	virtual int getEffectLevel(const Spell * spell) const = 0;

	///applying sorcery secondary skill etc
	virtual int64_t getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const = 0;

	///only bonus for particular spell
	virtual int64_t getSpecificSpellBonus(const Spell * spell, int64_t base) const = 0;

	///default spell-power for damage/heal calculation
	virtual int getEffectPower(const Spell * spell) const = 0;

	///default spell-power for timed effects duration
	virtual int getEnchantPower(const Spell * spell) const = 0;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	virtual int64_t getEffectValue(const Spell * spell) const = 0;

	virtual const PlayerColor getOwner() const = 0;

	///only name substitution
	virtual void getCasterName(MetaString & text) const = 0;

	///full default text
	virtual void getCastDescription(const Spell * spell, MetaString & text) const = 0;
	virtual void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const = 0;

	virtual void spendMana(const PacketSender * server, const int spellCost) const = 0;
};

}
