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

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class CGDwelling;
struct QuestInfo;

class DLL_LINKAGE PlayerState : public CBonusSystemNode, public Player
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

	std::vector<CGObjectInstance*> ownedObjects;

	std::vector<const CGTownInstance*> constOwnedTowns; //not serialized
	std::vector<const CGHeroInstance*> constOwnedHeroes; //not serialized
	std::vector<CGTownInstance*> ownedTowns; //not serialized
	std::vector<CGHeroInstance*> ownedHeroes; //not serialized

	template<typename T>
	std::vector<T> getObjectsOfType() const;

public:
	PlayerColor color;
	bool human; //true if human controlled player, false for AI
	TeamID team;
	TResources resources;

	/// list of objects that were "destroyed" by player, either via simple pick-up (e.g. resources) or defeated heroes or wandering monsters
	std::set<ObjectInstanceID> destroyedObjects;
	std::set<ObjectInstanceID> visitedObjects; // as a std::set, since most accesses here will be from visited status checks
	std::set<VisitedObjectGlobal> visitedObjectsGlobal;
	std::vector<QuestInfo> quests; //store info about all received quests
	std::vector<Bonus> battleBonuses; //additional bonuses to be added during battle with neutrals
	std::map<uint32_t, std::map<ArtifactPosition, ArtifactID>> costumesArtifacts;
	std::unique_ptr<JsonNode> playerLocalSettings; // Json with client-defined data, such as order of heroes or current hero paths. Not used by client/lib

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
	std::string getModScope() const override;
	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;
	void registerIcons(const IconRegistar & cb) const override;

	const std::vector<const CGHeroInstance* > & getHeroes() const;
	const std::vector<const CGTownInstance* > & getTowns() const;
	const std::vector<CGHeroInstance* > & getHeroes();
	const std::vector<CGTownInstance* > & getTowns();

	std::vector<const CGObjectInstance* > getOwnedObjects() const;

	void addOwnedObject(CGObjectInstance * object);
	void removeOwnedObject(CGObjectInstance * object);
	void postDeserialize();

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

		if (!h.saving)
			postDeserialize();
	}
};

struct DLL_LINKAGE TeamState : public CBonusSystemNode
{
public:
	TeamID id; //position in gameState::teams
	std::set<PlayerColor> players; // members of this team
	//TODO: boost::array, bool if possible
	boost::multi_array<ui8, 3> fogOfWarMap; //[z][x][y] true - visible, false - hidden

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

VCMI_LIB_NAMESPACE_END
