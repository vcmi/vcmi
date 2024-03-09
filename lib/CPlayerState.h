/*
 * CPlayerState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Player.h>
#include <vcmi/Team.h>

#include "bonuses/Bonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "ResourceSet.h"
#include "TurnTimerInfo.h"
#include "ConstTransitivePtr.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CGDwelling;
struct QuestInfo;

struct DLL_LINKAGE PlayerState : public CBonusSystemNode, public Player
{
	struct VisitedObjectGlobal
	{
		MapObjectID id;
		MapObjectSubID subID;

		bool operator < (const VisitedObjectGlobal & other) const
		{
			if (id != other.id)
				return id < other.id;
			else
				return subID < other.subID;
		}

		template <typename Handler> void serialize(Handler &h)
		{
			h & id;
			subID.serializeIdentifier(h, id);
		}
	};

public:
	PlayerColor color;
	bool human; //true if human controlled player, false for AI
	TeamID team;
	TResources resources;

	/// list of objects that were "destroyed" by player, either via simple pick-up (e.g. resources) or defeated heroes or wandering monsters
	std::set<ObjectInstanceID> destroyedObjects;

	std::set<ObjectInstanceID> visitedObjects; // as a std::set, since most accesses here will be from visited status checks
	std::set<VisitedObjectGlobal> visitedObjectsGlobal;
	std::vector<ConstTransitivePtr<CGHeroInstance> > heroes;
	std::vector<ConstTransitivePtr<CGTownInstance> > towns;
	std::vector<ConstTransitivePtr<CGDwelling> > dwellings; //used for town growth
	std::vector<QuestInfo> quests; //store info about all received quests
	std::vector<Bonus> battleBonuses; //additional bonuses to be added during battle with neutrals

	bool cheated;
	bool enteredWinningCheatCode, enteredLosingCheatCode; //if true, this player has entered cheat codes for loss / victory
	EPlayerStatus status;
	std::optional<ui8> daysWithoutCastle;
	TurnTimerInfo turnTimer;

	PlayerState();
	~PlayerState();

	std::string nodeName() const override;

	PlayerColor getId() const override;
	TeamID getTeam() const override;
	bool isHuman() const override;
	const IBonusBearer * getBonusBearer() const override;
	int getResourceAmount(int type) const override;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;
	void registerIcons(const IconRegistar & cb) const override;

	bool checkVanquished() const
	{
		return heroes.empty() && towns.empty();
	}

	template <typename Handler> void serialize(Handler &h)
	{
		h & color;
		h & human;
		h & team;
		h & resources;
		h & status;
		h & turnTimer;
		h & heroes;
		h & towns;
		h & dwellings;
		h & quests;
		h & visitedObjects;
		h & visitedObjectsGlobal;
		h & status;
		h & daysWithoutCastle;
		h & cheated;
		h & battleBonuses;
		h & enteredLosingCheatCode;
		h & enteredWinningCheatCode;
		h & static_cast<CBonusSystemNode&>(*this);
		if (h.version >= Handler::Version::DESTROYED_OBJECTS)
			h & destroyedObjects;
	}
};

struct DLL_LINKAGE TeamState : public CBonusSystemNode
{
public:
	TeamID id; //position in gameState::teams
	std::set<PlayerColor> players; // members of this team
	//TODO: boost::array, bool if possible
	std::unique_ptr<boost::multi_array<ui8, 3>> fogOfWarMap; //[z][x][y] true - visible, false - hidden

	TeamState();

	template <typename Handler> void serialize(Handler &h)
	{
		h & id;
		h & players;
		h & fogOfWarMap;
		h & static_cast<CBonusSystemNode&>(*this);
	}

};

VCMI_LIB_NAMESPACE_END
