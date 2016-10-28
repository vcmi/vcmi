/*
 * StackWithBonuses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/HeroBonus.h"

class CStack;

class StackWithBonuses : public IBonusBearer
{
public:
	const CStack *stack;
	mutable std::vector<Bonus> bonusesToAdd;

	virtual const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit,
						  const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
};
