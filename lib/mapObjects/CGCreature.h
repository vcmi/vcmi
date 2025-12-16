/*
 * CGCreature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "army/CArmedInstance.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CGCreature : public CArmedInstance //creatures on map
{
public:
	using CArmedInstance::CArmedInstance;

	enum Action {
		FIGHT = -2, FLEE = -1, JOIN_FOR_FREE = 0 //values > 0 mean gold price
	};

	enum Character {
		COMPLIANT = 0, FRIENDLY = 1, AGGRESSIVE = 2, HOSTILE = 3, SAVAGE = 4
	};

	ui32 identifier = -1; //unique code for this monster (used in missions)
	si8 character = 0; //character of this set of creatures (0 - the most friendly, 4 - the most hostile) => on init changed to -4 (compliant) ... 10 value (savage)
	MetaString message; //message printed for attacking hero
	TResources resources; // resources given to hero that has won with monsters
	ArtifactID gainedArtifact; //ID of artifact gained to hero, -1 if none
	bool neverFlees = false; //if true, the troops will never flee
	bool notGrowingTeam = false; //if true, number of units won't grow
	int64_t temppower = 0; //used to handle fractional stack growth for tiny stacks
	int64_t stacksCount = -1; // the split stack count specified in a HotA 1.7 map (0 - one more, -1 - default, -2 one less, -3 average)

	bool refusedJoining = false;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;
	void pickRandomObject(IGameRandomizer & gameRandomizer) override;
	void newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;
	CreatureID getCreatureID() const;
	const CCreature * getCreature() const;
	TQuantity getJoiningAmount() const;

	//stack formation depends on position,
	bool containsUpgradedStack() const;
	int getNumberOfStacks(const CGHeroInstance *hero) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & identifier;
		h & character;
		h & message;
		h & resources;
		h & gainedArtifact;
		h & neverFlees;
		h & notGrowingTeam;
		h & temppower;
		h & refusedJoining;
		h & formation;
		if(h.version >= Handler::Version::HOTA_MAP_STACK_COUNT)
			h & stacksCount;
	}
protected:
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	void fight(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;
	void flee(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;
	void fleeDecision(IGameEventCallback & gameEvents, const CGHeroInstance * h, ui32 pursue) const;
	void joinDecision(IGameEventCallback & gameEvents, const CGHeroInstance * h, int cost, ui32 accept) const;

	int takenAction(const CGHeroInstance *h, bool allowJoin=true) const; //action on confrontation: -2 - fight, -1 - flee, >=0 - will join for given value of gold (may be 0)
	void giveReward(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;
	std::string getMonsterLevelText() const;
	int getDefaultNumberOfStacks(const CGHeroInstance * hero) const;
	int getNumberOfStacksFromBonus(const CGHeroInstance * hero) const;
	ui32 hashByPosition() const;
};

VCMI_LIB_NAMESPACE_END
