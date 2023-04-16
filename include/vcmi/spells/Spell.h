/*
 * spells/Spell.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../Entity.h"

VCMI_LIB_NAMESPACE_BEGIN

class SpellID;

namespace spells
{
struct SchoolInfo;
class Caster;

class DLL_LINKAGE Spell: public EntityT<SpellID>
{
public:
	using SchoolCallback = std::function<void(const SchoolInfo &, bool &)>;

	///calculate spell damage on stack taking caster`s secondary skills into account
	virtual int64_t calculateDamage(const Caster * caster) const = 0;

	virtual int32_t getLevel() const = 0;
	virtual boost::logic::tribool getPositiveness() const = 0;
	virtual bool isAdventure() const = 0;
	virtual bool isCombat() const = 0;
	virtual bool isCreatureAbility() const = 0;
	virtual bool isPositive() const = 0;
	virtual bool isNegative() const = 0;
	virtual bool isNeutral() const = 0;

	virtual bool isDamage() const = 0;
	virtual bool isOffensive() const = 0;
	virtual bool isSpecial() const = 0;
	virtual bool isMagical() const = 0; //Should this spell considered as magical effect or as ability (like dendroid's bind)

	virtual void forEachSchool(const SchoolCallback & cb) const = 0;
	virtual const std::string & getCastSound() const = 0;
	virtual int32_t getCost(const int32_t skillLevel) const = 0;

	virtual int32_t getBasePower() const = 0;
	/**
	 * Returns spell level power, base power ignored
	 */
	virtual int32_t getLevelPower(const int32_t skillLevel) const = 0;

	virtual std::string getDescriptionTextID(int32_t level) const  = 0;
	virtual std::string getDescriptionTranslated(int32_t level) const  = 0;
};

}

VCMI_LIB_NAMESPACE_END
