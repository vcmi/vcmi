#pragma once

#include "CObjectHandler.h"
#include "CArmedInstance.h"
#include "../ResourceSet.h"

/*
 * MiscObjects.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class DLL_LINKAGE CPlayersVisited: public CGObjectInstance
{
public:
	std::set<PlayerColor> players; //players that visited this object

	bool wasVisited(PlayerColor player) const override;
	bool wasVisited(TeamID team) const;
	void setPropertyDer(ui8 what, ui32 val) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & players;
	}

	static const int OBJPROP_VISITED = 10;
};

class DLL_LINKAGE CGCreature : public CArmedInstance //creatures on map
{
public:
	enum Action {
		FIGHT = -2, FLEE = -1, JOIN_FOR_FREE = 0 //values > 0 mean gold price
	};

	enum Character {
		COMPLIANT = 0, FRIENDLY = 1, AGRESSIVE = 2, HOSTILE = 3, SAVAGE = 4
	};

	ui32 identifier; //unique code for this monster (used in missions)
	si8 character; //character of this set of creatures (0 - the most friendly, 4 - the most hostile) => on init changed to -4 (compliant) ... 10 value (savage)
	std::string message; //message printed for attacking hero
	TResources resources; // resources given to hero that has won with monsters
	ArtifactID gainedArtifact; //ID of artifact gained to hero, -1 if none
	bool neverFlees; //if true, the troops will never flee
	bool notGrowingTeam; //if true, number of units won't grow
	ui64 temppower; //used to handle fractional stack growth for tiny stacks

	bool refusedJoining;

	void onHeroVisit(const CGHeroInstance * h) const override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	void initObj() override;
	void newTurn() const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	//stack formation depends on position,
	bool containsUpgradedStack() const;
	int getNumberOfStacks(const CGHeroInstance *hero) const;

	struct DLL_LINKAGE formationInfo // info about merging stacks after battle back into one
	{
		si32 basicType;
		ui8 upgrade; //random seed used to determine number of stacks and is there's upgraded stack
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & basicType & upgrade;
		}
	} formation;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & identifier & character & message & resources & gainedArtifact & neverFlees & notGrowingTeam & temppower;
		h & refusedJoining & formation;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
private:

	void fight(const CGHeroInstance *h) const;
	void flee( const CGHeroInstance * h ) const;
	void fleeDecision(const CGHeroInstance *h, ui32 pursue) const;
	void joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const;

	int takenAction(const CGHeroInstance *h, bool allowJoin=true) const; //action on confrontation: -2 - fight, -1 - flee, >=0 - will join for given value of gold (may be 0)
	void giveReward(const CGHeroInstance * h) const;

};


class DLL_LINKAGE CGSignBottle : public CGObjectInstance //signs and ocean bottles
{
public:
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & message;
	}
};

class DLL_LINKAGE CGWitchHut : public CPlayersVisited
{
public:
	std::vector<si32> allowedAbilities;
	ui32 ability;

	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
		h & allowedAbilities & ability;
	}
};

class DLL_LINKAGE CGScholar : public CGObjectInstance
{
public:
	enum EBonusType {PRIM_SKILL, SECONDARY_SKILL, SPELL, RANDOM = 255};
	EBonusType bonusType;
	ui16 bonusID; //ID of skill/spell

//	void giveAnyBonus(const CGHeroInstance * h) const; //TODO: remove
	CGScholar() : bonusType(EBonusType::RANDOM){};
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & bonusType & bonusID;
	}
};

class DLL_LINKAGE CGGarrison : public CArmedInstance
{
public:
	bool removableUnits;

	bool passableFor(PlayerColor color) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & removableUnits;
	}
};

class DLL_LINKAGE CGArtifact : public CArmedInstance
{
public:
	CArtifactInstance *storedArtifact;
	std::string message;

	CGArtifact() : CArmedInstance() {storedArtifact = nullptr;};

	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	std::string getObjectName() const override;

	void pick( const CGHeroInstance * h ) const;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & storedArtifact;
	}
};

class DLL_LINKAGE CGResource : public CArmedInstance
{
public:
	ui32 amount; //0 if random
	std::string message;

	CGResource();
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	std::string getHoverText(PlayerColor player) const override;

	void collectRes(PlayerColor player) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & amount & message;
	}
};

class DLL_LINKAGE CGShrine : public CPlayersVisited
{
public:
	SpellID spell; //id of spell or NONE if random
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);;
		h & spell;
	}
};

class DLL_LINKAGE CGMine : public CArmedInstance
{
public:
	Res::ERes producedResource;
	ui32 producedQuantity;

private:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void flagMine(PlayerColor player) const;
	void newTurn() const override;
	void initObj() override;

	std::string getObjectName() const override;
	std::string getHoverText(PlayerColor player) const override;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & producedResource & producedQuantity;
	}
	ui32 defaultResProduction();
};

struct DLL_LINKAGE TeleportChannel
{
	enum EPassability {UNKNOWN, IMPASSABLE, PASSABLE};

	TeleportChannel() : passability(UNKNOWN) {}

	std::vector<ObjectInstanceID> entrances;
	std::vector<ObjectInstanceID> exits;
	EPassability passability;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & entrances & exits & passability;
	}
};

class DLL_LINKAGE CGTeleport : public CGObjectInstance
{
	bool isChannelEntrance(ObjectInstanceID id) const;
	bool isChannelExit(ObjectInstanceID id) const;

	std::vector<ObjectInstanceID> getAllEntrances(bool excludeCurrent = false) const;

protected:
	enum EType {UNKNOWN, ENTRANCE, EXIT, BOTH};
	EType type;

	CGTeleport();
	ObjectInstanceID getRandomExit(const CGHeroInstance * h) const;
	std::vector<ObjectInstanceID> getAllExits(bool excludeCurrent = false) const;

public:
	TeleportChannelID channel;

	bool isEntrance() const;
	bool isExit() const;

	virtual void teleportDialogAnswered(const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const = 0;

	static bool isTeleport(const CGObjectInstance * dst);
	static bool isConnected(const CGTeleport * src, const CGTeleport * dst);
	static bool isConnected(const CGObjectInstance * src, const CGObjectInstance * dst);
	static void addToChannel(std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > &channelsList, const CGTeleport * obj);
	static std::vector<ObjectInstanceID> getPassableExits(CGameState * gs, const CGHeroInstance * h, std::vector<ObjectInstanceID> exits);
	static bool isExitPassable(CGameState * gs, const CGHeroInstance * h, const CGObjectInstance * obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & channel & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGMonolith : public CGTeleport
{
	TeleportChannelID findMeChannel(std::vector<Obj> IDs, int SubID) const;

protected:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void teleportDialogAnswered(const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const override;
	void initObj() override;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTeleport&>(*this);
	}
};

class DLL_LINKAGE CGSubterraneanGate : public CGMonolith
{
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;

public:
	static void postInit();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMonolith&>(*this);
	}
};

class DLL_LINKAGE CGWhirlpool : public CGMonolith
{
	void onHeroVisit(const CGHeroInstance * h) const override;
	void teleportDialogAnswered(const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const override;
	static bool isProtected( const CGHeroInstance * h );

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMonolith&>(*this);
	}
};

class DLL_LINKAGE CGMagicWell : public CGObjectInstance //objects giving bonuses to luck/morale/movement
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGSirens : public CGObjectInstance
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGObservatory : public CGObjectInstance //Redwood observatory
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBoat : public CGObjectInstance
{
public:
	ui8 direction;
	const CGHeroInstance *hero;  //hero on board

	void initObj() override;

	CGBoat()
	{
		hero = nullptr;
		direction = 4;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this) & direction & hero;
	}
};

class CGShipyard : public CGObjectInstance, public IShipyard
{
public:
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed
	CGShipyard();
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<IShipyard&>(*this);
	}
};

class DLL_LINKAGE CGMagi : public CGObjectInstance
{
public:
	static std::map <si32, std::vector<ObjectInstanceID> > eyelist; //[subID][id], supports multiple sets as in H5

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};



class DLL_LINKAGE CCartographer : public CPlayersVisited
{
///behaviour varies depending on surface and  floor
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
	}
};

class DLL_LINKAGE CGDenOfthieves : public CGObjectInstance
{
	void onHeroVisit(const CGHeroInstance * h) const override;
};

class DLL_LINKAGE CGObelisk : public CPlayersVisited
{
public:
	static const int OBJPROP_INC = 20;
	static ui8 obeliskCount; //how many obelisks are on map
	static std::map<TeamID, ui8> visited; //map: team_id => how many obelisks has been visited

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	std::string getHoverText(PlayerColor player) const override;
	static void reset();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGLighthouse : public CGObjectInstance
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	std::string getHoverText(PlayerColor player) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
	void giveBonusTo( PlayerColor player ) const;
};
