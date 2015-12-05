#pragma once

/*
 * CPlayerState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "HeroBonus.h"

class CGHeroInstance;
class CGTownInstance;
class CGDwelling;

struct DLL_LINKAGE PlayerState : public CBonusSystemNode
{
public:
	PlayerColor color;
	bool human; //true if human controlled player, false for AI
	TeamID team;
	TResources resources;
	std::set<ObjectInstanceID> visitedObjects; // as a std::set, since most accesses here will be from visited status checks
	std::vector<ConstTransitivePtr<CGHeroInstance> > heroes;
	std::vector<ConstTransitivePtr<CGTownInstance> > towns;
	std::vector<ConstTransitivePtr<CGHeroInstance> > availableHeroes; //heroes available in taverns
	std::vector<ConstTransitivePtr<CGDwelling> > dwellings; //used for town growth
	std::vector<QuestInfo> quests; //store info about all received quests

	bool enteredWinningCheatCode, enteredLosingCheatCode; //if true, this player has entered cheat codes for loss / victory
	EPlayerStatus::EStatus status;
	boost::optional<ui8> daysWithoutCastle;

	PlayerState();
	std::string nodeName() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & color & human & team & resources & status;
		h & heroes & towns & availableHeroes & dwellings & quests & visitedObjects;
		h & getBonusList(); //FIXME FIXME FIXME
		h & status & daysWithoutCastle;
		h & enteredLosingCheatCode & enteredWinningCheatCode;
		h & static_cast<CBonusSystemNode&>(*this);
	}
};

struct DLL_LINKAGE TeamState : public CBonusSystemNode
{
public:
	TeamID id; //position in gameState::teams
	std::set<PlayerColor> players; // members of this team
	std::vector<std::vector<std::vector<ui8> > >  fogOfWarMap; //true - visible, false - hidden

	TeamState();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & players & fogOfWarMap;
		h & static_cast<CBonusSystemNode&>(*this);
	}

};
