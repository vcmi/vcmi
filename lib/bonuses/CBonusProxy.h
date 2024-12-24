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

#include "BonusSelector.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CCheckProxy
{
public:
	CCheckProxy(const IBonusBearer * Target, CSelector Selector, const std::string & cachingStr);
	CCheckProxy(const IBonusBearer * Target, BonusType bonusType);
	CCheckProxy(const CCheckProxy & other);
	CCheckProxy& operator= (const CCheckProxy & other) = default;

	bool getHasBonus() const;

private:
	const IBonusBearer * target;
	std::string cachingStr;
	CSelector selector;

	mutable int64_t cachedLast;
	mutable bool hasBonus;
};

VCMI_LIB_NAMESPACE_END
