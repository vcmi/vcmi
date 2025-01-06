/*
 * mock_BonusBearer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_BonusBearer.h"

BonusBearerMock::BonusBearerMock()
	: treeVersion(1),
	cachedLast(0)
{
}

BonusBearerMock::~BonusBearerMock() = default;

void BonusBearerMock::addNewBonus(const std::shared_ptr<Bonus> & b)
{
	bonuses.push_back(b);
	treeVersion++;
}

TConstBonusListPtr BonusBearerMock::getAllBonuses(const CSelector & selector, const CSelector & limit, const std::string & cachingStr) const
{
	if(cachedLast != treeVersion)
	{
		bonuses.stackBonuses();
		cachedLast = treeVersion;
	}

	auto ret = std::make_shared<BonusList>();
	bonuses.getBonuses(*ret, selector, limit);
	return ret;
}

int64_t BonusBearerMock::getTreeVersion() const
{
	return treeVersion;
}


