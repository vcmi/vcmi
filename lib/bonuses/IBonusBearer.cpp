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

int IBonusBearer::valOfBonuses(Bonus::BonusType type, int subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::type()(type);
	if(subtype != -1)
		s = s.And(Selector::subtype()(subtype));

	return valOfBonuses(s, cachingStr);
}

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	CSelector limit = nullptr;
	TConstBonusListPtr hlp = getAllBonuses(selector, limit, nullptr, cachingStr);
	return hlp->totalValue();
}
bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr) const
{
	//TODO: We don't need to count all bonuses and could break on first matching
	return getBonuses(selector, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getBonuses(selector, limit, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonusOfType(Bonus::BonusType type, int subtype) const
{
	//This part is performance-ciritcal
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::type()(type);
	if(subtype != -1)
		s = s.And(Selector::subtype()(subtype));

	return hasBonus(s, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	return getAllBonuses(selector, nullptr, nullptr, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getAllBonuses(selector, limit, nullptr, cachingStr);
}

bool IBonusBearer::hasBonusFrom(Bonus::BonusSource source, ui32 sourceID) const
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