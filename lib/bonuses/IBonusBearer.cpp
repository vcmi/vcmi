/*
 * IBonusBearer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CBonusSystemNode.h"
#include "BonusList.h"

VCMI_LIB_NAMESPACE_BEGIN

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	TConstBonusListPtr hlp = getAllBonuses(selector, nullptr, nullptr, cachingStr);
	return hlp->totalValue();
}

bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr) const
{
	//TODO: We don't need to count all bonuses and could break on first matching
	return !getBonuses(selector, cachingStr)->empty();
}

bool IBonusBearer::hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return !getBonuses(selector, limit, cachingStr)->empty();
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	return getAllBonuses(selector, nullptr, nullptr, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getAllBonuses(selector, limit, nullptr, cachingStr);
}

int IBonusBearer::valOfBonuses(BonusType type) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type));

	CSelector s = Selector::type()(type);

	return valOfBonuses(s, cachingStr);
}

bool IBonusBearer::hasBonusOfType(BonusType type) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type));

	CSelector s = Selector::type()(type);

	return hasBonus(s, cachingStr);
}

int IBonusBearer::valOfBonuses(BonusType type, int subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::typeSubtype(type, subtype);

	return valOfBonuses(s, cachingStr);
}

bool IBonusBearer::hasBonusOfType(BonusType type, int subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::typeSubtype(type, subtype);

	return hasBonus(s, cachingStr);
}

bool IBonusBearer::hasBonusFrom(BonusSource source, ui32 sourceID) const
{
	boost::format fmt("source_%did_%d");
	fmt % static_cast<int>(source) % sourceID;

	return hasBonus(Selector::source(source,sourceID), fmt.str());
}
std::shared_ptr<const Bonus> IBonusBearer::getBonus(const CSelector &selector) const
{
	auto bonuses = getAllBonuses(selector, Selector::all);
	return bonuses->getFirst(Selector::all);
}

VCMI_LIB_NAMESPACE_END