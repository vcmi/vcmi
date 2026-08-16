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

#include "callback/GameCallbackHolder.h"
#include "ResourceSet.h"
#include "TurnTimerInfo.h"
#include "bonuses/Bonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "mapObjects/CGObjectInstance.h"
#include "mapping/MapTilesStorage.h"
#include "gameState/ScenarioEventJournalEntry.h"

class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class CGDwelling;
struct QuestInfo;

class DLL_LINKAGE PlayerState : public CBonusSystemNode, public Player, public GameCallbackHolder
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

	std::vector<ObjectInstanceID> ownedObjects;

	template<typename T>
	std::vector<T> getObjectsOfType() const;

public:
	PlayerColor color;
	bool human = false; //true if human controlled player, false for AI
	TeamID team;
	TResources resources;

	/// list of objects that were "destroyed" by player, either via simple pick-up (e.g. resources) or defeated heroes or wandering monsters
	std::set<ObjectInstanceID> destroyedObjects;
	std::set<ObjectInstanceID> visitedObjects; // as a std::set, since most accesses here will be from visited status checks
	std::set<VisitedObjectGlobal> visitedObjectsGlobal;
	std::vector<QuestInfo> quests; //store info about all received quests
	std::vector<ScenarioEventJournalEntry> scenarioEventJournal;
	std::vector<Bonus> battleBonuses; //additional bonuses to be added during battle with neutrals
	std::map<uint32_t, std::map<ArtifactPosition, ArtifactID>> costumesArtifacts;
	std::unique_ptr<JsonNode> playerLocalSettings; // Json with client-defined data, such as order of heroes or current hero paths. Not used by client/lib

	bool cheated;
	bool enteredWinningCheatCode, enteredLosingCheatCode; //if true, this player has entered cheat codes for loss / victory
	EPlayerStatus status;
	std::optional<ui8> daysWithoutCastle;
	TurnTimerInfo turnTimer;

	PlayerState(IGameInfoCallback *cb);
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
	std::string getModScope() const override;
	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;
	void registerIcons(const IconRegistar & cb) const override;

	std::vector<const CGHeroInstance* > getHeroes() const;
	std::vector<const CGTownInstance* > getTowns() const;
	std::vector<CGHeroInstance* > getHeroes();
	std::vector<CGTownInstance* > getTowns();

	std::vector<const CGObjectInstance* > getOwnedObjects() const;

	void addOwnedObject(CGObjectInstance * object);
	void removeOwnedObject(CGObjectInstance * object);
	void markObjectControlled(ObjectInstanceID objectID);
	bool hasEverControlled(ObjectInstanceID objectID) const;

	/// True once a hero of this player has visited a keymaster tent of the given colour.
	bool wasKeymasterVisited(MapObjectSubID keymasterColor) const;

	bool checkVanquished() const
	{
		return getHeroes().empty() && getTowns().empty();
	}

	template <typename Handler> void serialize(Handler &h)
	{
		h & color;
		h & human;
		h & team;
		h & resources;
		h & status;
		h & turnTimer;
		h & *playerLocalSettings;
		h & ownedObjects;

		h & quests;
		if(h.hasFeature(Handler::Version::SCENARIO_EVENT_JOURNAL))
			h & scenarioEventJournal;
		h & visitedObjects;
		h & visitedObjectsGlobal;
		h & status;
		h & daysWithoutCastle;
		h & cheated;
		h & battleBonuses;
		h & costumesArtifacts;
		h & enteredLosingCheatCode;
		h & enteredWinningCheatCode;
		h & static_cast<CBonusSystemNode&>(*this);
		h & destroyedObjects;
		if(h.hasFeature(Handler::Version::CONTROL_LOSS_TRACKING))
			h & everControlledObjects;
	}

	private:
		std::set<ObjectInstanceID> everControlledObjects;
};

struct DLL_LINKAGE TeamState : public CBonusSystemNode
{
public:
	TeamID id; //position in gameState::teams
	std::set<PlayerColor> players; // members of this team
	//TODO: bool if possible
	MapTilesStorage<uint8_t> fogOfWarMap; //true - visible, false - hidden

	std::set<ObjectInstanceID> scoutedObjects;

	TeamState();

	template <typename Handler> void serialize(Handler &h)
	{
		h & id;
		h & players;
		h & fogOfWarMap;
		h & static_cast<CBonusSystemNode&>(*this);
		h & scoutedObjects;
	}

};
