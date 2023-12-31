/*
 * CBonusProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusList.h"
#include "CBonusProxy.h"
#include "IBonusBearer.h"

VCMI_LIB_NAMESPACE_BEGIN

///CBonusProxy
CBonusProxy::CBonusProxy(const IBonusBearer * Target, CSelector Selector):
	bonusListCachedLast(0),
	target(Target),
	selector(std::move(Selector)),
	currentBonusListIndex(0)
{
}

CBonusProxy::CBonusProxy(const CBonusProxy & other):
	bonusListCachedLast(other.bonusListCachedLast),
	target(other.target),
	selector(other.selector),
	currentBonusListIndex(other.currentBonusListIndex)
{
	bonusList[currentBonusListIndex] = other.bonusList[currentBonusListIndex];
}

CBonusProxy::CBonusProxy(CBonusProxy && other) noexcept:
	bonusListCachedLast(0),
	target(other.target),
	currentBonusListIndex(0)
{
	std::swap(bonusListCachedLast, other.bonusListCachedLast);
	std::swap(selector, other.selector);
	std::swap(bonusList, other.bonusList);
	std::swap(currentBonusListIndex, other.currentBonusListIndex);
}

CBonusProxy & CBonusProxy::operator=(const CBonusProxy & other)
{
	boost::lock_guard<boost::mutex> lock(swapGuard);

	selector = other.selector;
	swapBonusList(other.bonusList[other.currentBonusListIndex]);
	bonusListCachedLast = other.bonusListCachedLast;

	return *this;
}

CBonusProxy & CBonusProxy::operator=(CBonusProxy && other) noexcept
{
	std::swap(bonusListCachedLast, other.bonusListCachedLast);
	std::swap(selector, other.selector);
	std::swap(bonusList, other.bonusList);
	std::swap(currentBonusListIndex, other.currentBonusListIndex);

	return *this;
}

void CBonusProxy::swapBonusList(TConstBonusListPtr other) const
{
	// The idea here is to avoid changing active bonusList while it can be read by a different thread.
	// Because such use of shared ptr is not thread safe
	// So to avoid this we change the second offline instance and swap active index
	auto newCurrent = 1 - currentBonusListIndex;
	bonusList[newCurrent] = std::move(other);
	currentBonusListIndex = newCurrent;
}

TConstBonusListPtr CBonusProxy::getBonusList() const
{
	auto needUpdateBonusList = [&]() -> bool
	{
		return target->getTreeVersion() != bonusListCachedLast || !bonusList[currentBonusListIndex];
	};

	// avoid locking if everything is up-to-date
	if(needUpdateBonusList())
	{
		boost::lock_guard<boost::mutex>lock(swapGuard);

		if(needUpdateBonusList())
		{
			//TODO: support limiters
			swapBonusList(target->getAllBonuses(selector, Selector::all));
			bonusListCachedLast = target->getTreeVersion();
		}
	}

	return bonusList[currentBonusListIndex];
}

const BonusList * CBonusProxy::operator->() const
{
	return getBonusList().get();
}

CTotalsProxy::CTotalsProxy(const IBonusBearer * Target, CSelector Selector, int InitialValue):
	CBonusProxy(Target, std::move(Selector)),
	initialValue(InitialValue),
	meleeCachedLast(0),
	meleeValue(0),
	rangedCachedLast(0),
	rangedValue(0)
{
}

CTotalsProxy::CTotalsProxy(const CTotalsProxy & other)
	: CBonusProxy(other),
	initialValue(other.initialValue),
	meleeCachedLast(other.meleeCachedLast),
	meleeValue(other.meleeValue),
	rangedCachedLast(other.rangedCachedLast),
	rangedValue(other.rangedValue)
{
}

int CTotalsProxy::getValue() const
{
	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != valueCachedLast)
	{
		auto bonuses = getBonusList();

		value = initialValue + bonuses->totalValue();
		valueCachedLast = treeVersion;
	}
	return value;
}

int CTotalsProxy::getValueAndList(TConstBonusListPtr & outBonusList) const
{
	const auto treeVersion = target->getTreeVersion();
	outBonusList = getBonusList();

	if(treeVersion != valueCachedLast)
	{
		value = initialValue + outBonusList->totalValue();
		valueCachedLast = treeVersion;
	}
	return value;
}

int CTotalsProxy::getMeleeValue() const
{
	static const auto limit = Selector::effectRange()(BonusLimitEffect::NO_LIMIT).Or(Selector::effectRange()(BonusLimitEffect::ONLY_MELEE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != meleeCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		meleeValue = initialValue + bonuses->totalValue();
		meleeCachedLast = treeVersion;
	}

	return meleeValue;
}

int CTotalsProxy::getRangedValue() const
{
	static const auto limit = Selector::effectRange()(BonusLimitEffect::NO_LIMIT).Or(Selector::effectRange()(BonusLimitEffect::ONLY_DISTANCE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != rangedCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		rangedValue = initialValue + bonuses->totalValue();
		rangedCachedLast = treeVersion;
	}

	return rangedValue;
}

///CCheckProxy
CCheckProxy::CCheckProxy(const IBonusBearer * Target, CSelector Selector):
	target(Target),
	selector(std::move(Selector)),
	cachedLast(0),
	hasBonus(false)
{
}

//This constructor should be placed here to avoid side effects
CCheckProxy::CCheckProxy(const CCheckProxy & other) = default;

bool CCheckProxy::getHasBonus() const
{
	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != cachedLast)
	{
		hasBonus = target->hasBonus(selector);
		cachedLast = treeVersion;
	}

	return hasBonus;
}

VCMI_LIB_NAMESPACE_END