#pragma once

#include "CObjectHandler.h"
#include "CArmedInstance.h"

/*
 * CBank.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class DLL_LINKAGE CBank : public CArmedInstance
{
	public:
	int index; //banks have unusal numbering - see ZCRBANK.txt and initObj()
	BankConfig *bc;
	double multiplier; //for improved banks script
	std::vector<ui32> artifacts; //fixed and deterministic
	ui32 daycounter;

	void initObj() override;
	const std::string & getHoverText() const override;
	void initialize() const;
	void reset(ui16 var1);
	void newTurn() const override;
	bool wasVisited (PlayerColor player) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & index & multiplier & artifacts & daycounter & bc;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};
class DLL_LINKAGE CGPyramid : public CBank
{
public:
	ui16 spell;

	void initObj() override;
	const std::string & getHoverText() const override;
	void newTurn() const override {}; //empty, no reset
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBank&>(*this);
		h & spell;
	}
};
