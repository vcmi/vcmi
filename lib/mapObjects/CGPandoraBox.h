/*
 * CGPandoraBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CRewardableObject.h"
#include "../ResourceSet.h"
#include "../mapping/MapDifficulty.h"

VCMI_LIB_NAMESPACE_BEGIN

struct InfoWindow;

class DLL_LINKAGE CGPandoraBox : public CRewardableObject
{
public:
	MapDifficultySet presentOnDifficulties;

	using CRewardableObject::CRewardableObject;

	MetaString message;

	void initObj(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CRewardableObject&>(*this);
		h & message;
		if(h.version >= Handler::Version::HOTA_MAP_FORMAT_EXTENSIONS)
			h & presentOnDifficulties;
	}
protected:
	void grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const override;
	
	virtual void init();
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
{
public:
	CGEvent(IGameInfoCallback *cb);

	//players whom this event is available for
	std::set<PlayerColor> availableFor;

	//true if event is removed after occurring
	bool removeAfterVisit = false;
	//true if computer player can activate this event
	bool computerActivate = false;
	//true if human player can activate this event
	bool humanActivate = false;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGPandoraBox &>(*this);
		h & removeAfterVisit;
		h & availableFor;
		h & computerActivate;
		h & humanActivate;
	}

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
protected:
	void grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const override;
	
	void init() override;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
private:
	void activated(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;
};

VCMI_LIB_NAMESPACE_END
