/*
 * IBonusBearer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE IBonusBearer
{
public:
	//new bonusing node interface
	// * selector is predicate that tests if Bonus matches our criteria
	IBonusBearer() = default;
	virtual ~IBonusBearer() = default;
	virtual TConstBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = {}) const = 0;
	int valOfBonuses(const CSelector &selector, const std::string &cachingStr = {}, int baseValue = 0) const;
	bool hasBonus(const CSelector &selector, const std::string &cachingStr = {}) const;
	bool hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = {}) const;
	TConstBonusListPtr getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = {}) const;
	TConstBonusListPtr getBonuses(const CSelector &selector, const std::string &cachingStr = {}) const;

	std::shared_ptr<const Bonus> getBonus(const CSelector &selector) const; //returns any bonus visible on node that matches (or nullptr if none matches)

	//Optimized interface (with auto-caching)
	int applyBonuses(BonusType type, int baseValue) const; //subtype -> subtype of bonus;
	int valOfBonuses(BonusType type) const; //subtype -> subtype of bonus;
	bool hasBonusOfType(BonusType type) const;//determines if hero has a bonus of given type (and optionally subtype)
	int valOfBonuses(BonusType type, BonusSubtypeID subtype) const; //subtype -> subtype of bonus;
	bool hasBonusOfType(BonusType type, BonusSubtypeID subtype) const;//determines if hero has a bonus of given type (and optionally subtype)
	bool hasBonusFrom(BonusSource source) const;
	bool hasBonusFrom(BonusSource source, BonusSourceID sourceID) const;

	TConstBonusListPtr getBonusesFrom(BonusSource source) const;
	TConstBonusListPtr getBonusesOfType(BonusType type) const;
	TConstBonusListPtr getBonusesOfType(BonusType type, BonusSubtypeID subtype) const;

	virtual int32_t getTreeVersion() const = 0;
};

VCMI_LIB_NAMESPACE_END
