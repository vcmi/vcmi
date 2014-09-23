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

	bool wasVisited(PlayerColor player) const;
	bool wasVisited(TeamID team) const;
	void setPropertyDer(ui8 what, ui32 val) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & players;
	}
};

class DLL_LINKAGE CGCreature : public CArmedInstance //creatures on map
{
	enum Action {
		FIGHT = -2, FLEE = -1, JOIN_FOR_FREE = 0 //values > 0 mean gold price
	};

public:
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


	struct DLL_LINKAGE formationInfo // info about merging stacks after battle back into one
	{
		si32 basicType;
		ui32 randomFormation; //random seed used to determine number of stacks and is there's upgraded stack
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & basicType & randomFormation;
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
	
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void flagMine(PlayerColor player) const;
	void newTurn() const override;
	void initObj() override;

	std::string getObjectName() const override;
	std::string getHoverText(PlayerColor player) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & producedResource & producedQuantity;
	}
	ui32 defaultResProduction();
};

class DLL_LINKAGE CGTeleport : public CGObjectInstance //teleports and subterranean gates
{
public:
	static std::map<Obj, std::map<int, std::vector<ObjectInstanceID> > > objs; //teleports: map[ID][subID] => vector of ids
	static std::vector<std::pair<ObjectInstanceID, ObjectInstanceID> > gates; //subterranean gates: pairs of ids
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	static void postInit();
	static ObjectInstanceID getMatchingGate(ObjectInstanceID id); //receives id of one subterranean gate and returns id of the paired one, -1 if none

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
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
	void getOutOffsets(std::vector<int3> &offsets) const; //offsets to obj pos when we boat can be placed
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
	static ui8 obeliskCount; //how many obelisks are on map
	static std::map<TeamID, ui8> visited; //map: team_id => how many obelisks has been visited

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	std::string getHoverText(PlayerColor player) const override;

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
