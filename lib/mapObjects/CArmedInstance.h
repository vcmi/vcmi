/*
 * CArmedInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CObjectHandler.h"
#include "../CCreatureSet.h"

class BattleInfo;
class CGameState;

class DLL_LINKAGE CArmedInstance: public CGObjectInstance, public CBonusSystemNode, public CCreatureSet
{
public:
	BattleInfo *battle; //set to the current battle, if engaged

	void randomizeArmy(int type);
	virtual void updateMoraleBonusFromArmy();

	void armyChanged() override;

	//////////////////////////////////////////////////////////////////////////
//	int valOfGlobalBonuses(CSelector selector) const; //used only for castle interface								???
	virtual CBonusSystemNode *whereShouldBeAttached(CGameState *gs);
	virtual CBonusSystemNode *whatShouldBeAttached();
	//////////////////////////////////////////////////////////////////////////

	CArmedInstance();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCreatureSet&>(*this);
	}
};
