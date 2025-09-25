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

#include "IOwnableObject.h"

#include "army/CArmedInstance.h"

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

class DLL_LINKAGE CGDwelling : public CArmedInstance, public IOwnableObject
{
public:
	using TCreaturesSet = std::vector<std::pair<ui32, std::vector<CreatureID> > >;

	std::optional<CGDwellingRandomizationInfo> randomizationInfo; //random dwelling options; not serialized
	TCreaturesSet creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

	CGDwelling(IGameInfoCallback *cb, BonusNodeType nodeType);
	CGDwelling(IGameInfoCallback *cb);
	~CGDwelling() override;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;
	AnimationPath getKingdomOverviewImage() const;

protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	FactionID randomizeFaction(IGameRandomizer & gameRandomizer);
	int randomizeLevel(vstd::RNG & rand);

	void pickRandomObject(IGameRandomizer & gameRandomizer) override;
	void initObj(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const override;
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;
	bool wasVisited (PlayerColor player) const override;

	void updateGuards(IGameEventCallback & gameEvents) const;
	void heroAcceptsCreatures(IGameEventCallback & gameEvents, const CGHeroInstance *h) const;

public:
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & creatures;
	}
};


VCMI_LIB_NAMESPACE_END
