/*
 * CGDwelling.h, part of VCMI engine
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

class CGDwelling;

class DLL_LINKAGE CGDwellingRandomizationInfo
{
public:
	std::set<FactionID> allowedFactions;

	std::string instanceId;//vcmi map instance identifier
	int32_t identifier = 0;//h3m internal identifier

	uint8_t minLevel = 1;
	uint8_t maxLevel = 7; //minimal and maximal level of creature in dwelling: <1, 7>

	void serializeJson(JsonSerializeFormat & handler);
};

class DLL_LINKAGE CGDwelling : public CArmedInstance
{
public:
	typedef std::vector<std::pair<ui32, std::vector<CreatureID> > > TCreaturesSet;

	std::optional<CGDwellingRandomizationInfo> randomizationInfo; //random dwelling options; not serialized
	TCreaturesSet creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

	CGDwelling(IGameCallback *cb);
	~CGDwelling() override;

protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	FactionID randomizeFaction(CRandomGenerator & rand);
	int randomizeLevel(CRandomGenerator & rand);

	void pickRandomObject(CRandomGenerator & rand) override;
	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void newTurn(CRandomGenerator & rand) const override;
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;

	void updateGuards() const;
	void heroAcceptsCreatures(const CGHeroInstance *h) const;

public:
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & creatures;
	}
};


VCMI_LIB_NAMESPACE_END
