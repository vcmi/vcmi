/*
* ArmyManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AIUtility.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CBuildingHandler.h"
#include "VCAI.h"

struct SlotInfo
{
	const CCreature * creature;
	int count;
	uint64_t power;
};

class DLL_EXPORT IArmyManager //: public: IAbstractManager
{
public:
	virtual void init(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;
	virtual void update() = 0;
	virtual bool canGetArmy(const CArmedInstance * target, const CArmedInstance * source) const = 0;
	virtual ui64 howManyReinforcementsCanBuy(const CCreatureSet * target, const CGDwelling * source) const = 0;
	virtual ui64 howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const = 0;
	virtual std::vector<SlotInfo> getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const = 0;
	virtual std::vector<SlotInfo>::iterator getWeakestCreature(std::vector<SlotInfo> & army) const = 0;
	virtual std::vector<SlotInfo> getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const = 0;
	virtual std::vector<creInfo> getArmyAvailableToBuy(const CCreatureSet * hero, const CGDwelling * dwelling) const = 0;
	virtual uint64_t evaluateStackPower(const CCreature * creature, int count) const = 0;
	virtual SlotInfo getTotalCreaturesAvailable(CreatureID creatureID) const = 0;
};

class DLL_EXPORT ArmyManager : public IArmyManager
{
private:
	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;
	std::map<CreatureID, SlotInfo> totalArmy;

public:
	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
	void update() override;

	bool canGetArmy(const CArmedInstance * target, const CArmedInstance * source) const override;
	ui64 howManyReinforcementsCanBuy(const CCreatureSet * target, const CGDwelling * source) const override;
	ui64 howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo> getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo>::iterator getWeakestCreature(std::vector<SlotInfo> & army) const override;
	std::vector<SlotInfo> getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<creInfo> getArmyAvailableToBuy(const CCreatureSet * hero, const CGDwelling * dwelling) const override;
	uint64_t evaluateStackPower(const CCreature * creature, int count) const override;
	SlotInfo getTotalCreaturesAvailable(CreatureID creatureID) const override;
};
