/*
 * EnemyInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/BattleHex.h"

class CStack;

class EnemyInfo
{
public:
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s)
	{}
	void calcDmg(const CStack * ourStack);
	bool operator==(const EnemyInfo& ei) const
	{
		return s == ei.s;
	}
};
