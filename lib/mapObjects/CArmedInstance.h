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

#include "CGObjectInstance.h"
#include "../CCreatureSet.h"
#include "../bonuses/CBonusProxy.h"
#include "../bonuses/CBonusSystemNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleInfo;
class CGameState;
class JsonSerializeFormat;

class DLL_LINKAGE CArmedInstance: public CGObjectInstance, public CBonusSystemNode, public CCreatureSet, public IConstBonusProvider
{
private:
	CCheckProxy nonEvilAlignmentMix;
	static CSelector nonEvilAlignmentMixSelector;

public:
	BattleInfo *battle; //set to the current battle, if engaged

	void randomizeArmy(FactionID type);
	virtual void updateMoraleBonusFromArmy();

	void armyChanged() override;

	//////////////////////////////////////////////////////////////////////////
	//IConstBonusProvider
	const IBonusBearer* getBonusBearer() const override;
//	int valOfGlobalBonuses(CSelector selector) const; //used only for castle interface								???
	virtual CBonusSystemNode & whereShouldBeAttached(CGameState * gs);
	virtual CBonusSystemNode & whatShouldBeAttached();
	//////////////////////////////////////////////////////////////////////////

	CArmedInstance(IGameCallback *cb);
	CArmedInstance(IGameCallback *cb, bool isHypothetic);

	PlayerColor getOwner() const override
	{
		return this->tempOwner;
	}
	
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCreatureSet&>(*this);
	}
};

VCMI_LIB_NAMESPACE_END
