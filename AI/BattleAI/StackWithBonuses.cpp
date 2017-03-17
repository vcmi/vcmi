/*
 * StackWithBonuses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StackWithBonuses.h"
#include "../../lib/CStack.h"


const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector &selector, const CSelector &limit,
						    const CBonusSystemNode *root /*= nullptr*/, const std::string &cachingStr /*= ""*/) const
{
	TBonusListPtr ret = std::make_shared<BonusList>();
	const TBonusListPtr originalList = stack->getAllBonuses(selector, limit, root, cachingStr);
	range::copy(*originalList, std::back_inserter(*ret));
	for(auto &bonus : bonusesToAdd)
	{
		auto b = std::make_shared<Bonus>(bonus);
		if(selector(b.get())  &&  (!limit || !limit(b.get())))
			ret->push_back(b);
	}
	//TODO limiters?
	return ret;
}
