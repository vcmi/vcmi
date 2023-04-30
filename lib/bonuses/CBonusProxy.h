/*
 * CBonusProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CBonusProxy
{
public:
	CBonusProxy(const IBonusBearer * Target, CSelector Selector);
	CBonusProxy(const CBonusProxy & other);
	CBonusProxy(CBonusProxy && other) noexcept;

	CBonusProxy & operator=(CBonusProxy && other) noexcept;
	CBonusProxy & operator=(const CBonusProxy & other);
	const BonusList * operator->() const;
	TConstBonusListPtr getBonusList() const;

protected:
	CSelector selector;
	const IBonusBearer * target;
	mutable int64_t bonusListCachedLast;
	mutable TConstBonusListPtr bonusList[2];
	mutable int currentBonusListIndex;
	mutable boost::mutex swapGuard;
	void swapBonusList(TConstBonusListPtr other) const;
};

class DLL_LINKAGE CTotalsProxy : public CBonusProxy
{
public:
	CTotalsProxy(const IBonusBearer * Target, CSelector Selector, int InitialValue);
	CTotalsProxy(const CTotalsProxy & other);
	CTotalsProxy(CTotalsProxy && other) = delete;

	CTotalsProxy & operator=(const CTotalsProxy & other) = default;
	CTotalsProxy & operator=(CTotalsProxy && other) = delete;

	int getMeleeValue() const;
	int getRangedValue() const;
	int getValue() const;
	/**
	Returns total value of all selected bonuses and sets bonusList as a pointer to the list of selected bonuses
	@param bonusList is the out list of all selected bonuses
	@return total value of all selected bonuses and 0 otherwise
	*/
	int getValueAndList(TConstBonusListPtr & bonusList) const;

private:
	int initialValue;

	mutable int64_t valueCachedLast = 0;
	mutable int value = 0;

	mutable int64_t meleeCachedLast;
	mutable int meleeValue;

	mutable int64_t rangedCachedLast;
	mutable int rangedValue;
};

class DLL_LINKAGE CCheckProxy
{
public:
	CCheckProxy(const IBonusBearer * Target, CSelector Selector);
	CCheckProxy(const CCheckProxy & other);
	CCheckProxy& operator= (const CCheckProxy & other) = default;

	bool getHasBonus() const;

private:
	const IBonusBearer * target;
	CSelector selector;

	mutable int64_t cachedLast;
	mutable bool hasBonus;
};

VCMI_LIB_NAMESPACE_END