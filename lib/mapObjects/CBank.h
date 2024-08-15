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

#include "CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BankConfig;
class CBankInstanceConstructor;

class DLL_LINKAGE CBank : public CArmedInstance
{
	std::unique_ptr<BankConfig> bankConfig;
	ui32 daycounter;
	ui32 resetDuration;
	bool coastVisitable;
	bool regularUnitPlacement;

	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
	void doVisit(const CGHeroInstance * hero) const;

public:
	CBank(IGameCallback *cb);
	~CBank() override;

	void setConfig(const BankConfig & bc);

	void initObj(vstd::RNG & rand) override;
	std::string getHoverText(PlayerColor player) const override;
	void newTurn(vstd::RNG & rand) const override;
	bool wasVisited (PlayerColor player) const override;
	bool isCoastVisitable() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;

	std::vector<Component> getPopupComponents(PlayerColor player) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & daycounter;
		h & bankConfig;
		h & resetDuration;
		h & coastVisitable;
		if (h.version >= Handler::Version::BANK_UNIT_PLACEMENT)
			h & regularUnitPlacement;
		else if (!h.saving)
			regularUnitPlacement = false;
	}

	friend class CBankInstanceConstructor;
};

VCMI_LIB_NAMESPACE_END
