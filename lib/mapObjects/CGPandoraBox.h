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

VCMI_LIB_NAMESPACE_BEGIN

struct InfoWindow;

class DLL_LINKAGE CGPandoraBox : public CRewardableObject
{
public:
	using CRewardableObject::CRewardableObject;

	MetaString message;

	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CRewardableObject&>(*this);
		h & message;
	}
protected:
	void grantRewardWithMessage(const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const override;
	
	virtual void init();
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
{
public:
	using CGPandoraBox::CGPandoraBox;

	bool removeAfterVisit = false; //true if event is removed after occurring
	std::set<PlayerColor> availableFor; //players whom this event is available for
	bool computerActivate = false; //true if computer player can activate this event
	bool humanActivate = false; //true if human player can activate this event

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGPandoraBox &>(*this);
		h & removeAfterVisit;
		h & availableFor;
		h & computerActivate;
		h & humanActivate;
	}

	void onHeroVisit(const CGHeroInstance * h) const override;
protected:
	void grantRewardWithMessage(const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const override;
	
	void init() override;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
private:
	void activated(const CGHeroInstance * h) const;
};

VCMI_LIB_NAMESPACE_END
