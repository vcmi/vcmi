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
#include "CBonusProxy.h"
#include "IBonusBearer.h"

VCMI_LIB_NAMESPACE_BEGIN

///CCheckProxy
CCheckProxy::CCheckProxy(const IBonusBearer * Target, BonusType bonusType):
	target(Target),
	selector(Selector::type()(bonusType)),
	cachingStr("type_" + std::to_string(static_cast<int>(bonusType))),
	cachedLast(0),
	hasBonus(false)
{

}

CCheckProxy::CCheckProxy(const IBonusBearer * Target, CSelector Selector, const std::string & cachingStr):
	target(Target),
	selector(std::move(Selector)),
	cachedLast(0),
	cachingStr(cachingStr),
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
		hasBonus = target->hasBonus(selector, cachingStr);
		cachedLast = treeVersion;
	}

	return hasBonus;
}

VCMI_LIB_NAMESPACE_END
