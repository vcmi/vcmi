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

#include "IBonusBearer.h"
#include "BonusList.h"

VCMI_LIB_NAMESPACE_BEGIN

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr, int baseValue) const
{
	TConstBonusListPtr hlp = getAllBonuses(selector, nullptr, cachingStr);
	return hlp->totalValue(baseValue);
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
	return getAllBonuses(selector, nullptr, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getAllBonuses(selector, limit, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonusesFrom(BonusSource source) const
{
	std::string cachingStr = "source_" + std::to_string(static_cast<int>(source));
	CSelector s = Selector::sourceTypeSel(source);
	return getBonuses(s, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonusesOfType(BonusType type) const
{
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type));
	CSelector s = Selector::type()(type);
	return getBonuses(s, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonusesOfType(BonusType type, BonusSubtypeID subtype) const
{
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + subtype.toString();
	CSelector s = Selector::typeSubtype(type, subtype);
	return getBonuses(s, cachingStr);
}

int IBonusBearer::applyBonuses(BonusType type, int baseValue) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type));
	CSelector s = Selector::type()(type);
	return valOfBonuses(s, cachingStr, baseValue);
}

int IBonusBearer::valOfBonuses(BonusType type) const
{
	return applyBonuses(type, 0);
}

bool IBonusBearer::hasBonusOfType(BonusType type) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type));

	CSelector s = Selector::type()(type);

	return hasBonus(s, cachingStr);
}

int IBonusBearer::valOfBonuses(BonusType type, BonusSubtypeID subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + subtype.toString();

	CSelector s = Selector::typeSubtype(type, subtype);

	return valOfBonuses(s, cachingStr);
}

bool IBonusBearer::hasBonusOfType(BonusType type, BonusSubtypeID subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + subtype.toString();

	CSelector s = Selector::typeSubtype(type, subtype);

	return hasBonus(s, cachingStr);
}

bool IBonusBearer::hasBonusFrom(BonusSource source, BonusSourceID sourceID) const
{
	std::string cachingStr = "source_" + std::to_string(static_cast<int>(source)) + "_" + std::to_string(sourceID.getNum());
	return hasBonus(Selector::source(source,sourceID), cachingStr);
}

bool IBonusBearer::hasBonusFrom(BonusSource source) const
{
	std::string cachingStr = "source_" + std::to_string(static_cast<int>(source));
	return hasBonus((Selector::sourceTypeSel(source)), cachingStr);
}

std::shared_ptr<const Bonus> IBonusBearer::getBonus(const CSelector &selector) const
{
	auto bonuses = getAllBonuses(selector, Selector::all);
	return bonuses->getFirst(Selector::all);
}

VCMI_LIB_NAMESPACE_END
