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

const TBonusListPtr BonusBearerMock::getAllBonuses(const CSelector & selector, const CSelector & limit, const CBonusSystemNode * root, const std::string & cachingStr) const
{
	if(cachedLast != treeVersion)
	{
		bonuses.eliminateDuplicates();
		cachedLast = treeVersion;
	}

	auto ret = std::make_shared<BonusList>();
	bonuses.getBonuses(*ret, selector, limit);
	return ret;
}

int64_t BonusBearerMock::getTreeVersion() const
{
	int64_t ret = treeVersion;
	return ret << 32;
}


