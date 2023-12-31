/*
 * mock_BonusBearer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/bonuses/BonusList.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/IBonusBearer.h"


class BonusBearerMock : public IBonusBearer
{
public:
	BonusBearerMock();
	virtual ~BonusBearerMock();

	void addNewBonus(const std::shared_ptr<Bonus> & b);

	TConstBonusListPtr getAllBonuses(const CSelector & selector, const CSelector & limit, const CBonusSystemNode * root = nullptr, const std::string & cachingStr = "") const override;

	int64_t getTreeVersion() const override;
private:
	mutable BonusList bonuses;

	mutable int64_t cachedLast;
	int32_t treeVersion;
};
