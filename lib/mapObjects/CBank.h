/*
 * CBank.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CObjectHandler.h"
#include "CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BankConfig;
class CBankInstanceConstructor;

class DLL_LINKAGE CBank : public CArmedInstance
{
	std::unique_ptr<BankConfig> bc;
	ui32 daycounter;
	ui32 resetDuration;

	void setPropertyDer(ui8 what, ui32 val) override;
	void doVisit(const CGHeroInstance * hero) const;

public:
	CBank();
	~CBank() override;

	void setConfig(const BankConfig & bc);

	void initObj(CRandomGenerator & rand) override;
	std::string getHoverText(PlayerColor player) const override;
	void newTurn(CRandomGenerator & rand) const override;
	bool wasVisited (PlayerColor player) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & daycounter;
		h & bc;
		h & resetDuration;
	}

	friend class CBankInstanceConstructor;
};

VCMI_LIB_NAMESPACE_END
