/*
 * MiscObjects.h, part of VCMI engine
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
#include "../entities/artifact/CArtifactInstance.h"
#include "../texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMap;
class UpgradeInfo;
class MineInstanceConstructor;

// This one teleport-specific, but has to be available everywhere in callbacks and netpacks
// For now it's will be there till teleports code refactored and moved into own file
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

/// Legacy class, use CRewardableObject instead
class DLL_LINKAGE CTeamVisited: public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	std::set<PlayerColor> players; //players that visited this object

	bool wasVisited (const CGHeroInstance * h) const override;
	bool wasVisited(PlayerColor player) const override;
	bool wasVisited(const TeamID & team) const;
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & players;
	}
};

class DLL_LINKAGE CGSignBottle : public CGObjectInstance //signs and ocean bottles
{
public:
	using CGObjectInstance::CGObjectInstance;

	MetaString message;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & message;
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGGarrison : public CArmedInstance, public IOwnableObject
{
public:
	using CArmedInstance::CArmedInstance;

	bool removableUnits;

	void initObj(IGameRandomizer & gameRandomizer) override;
	bool passableFor(PlayerColor color) const override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & removableUnits;
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
	void addAntimagicGarrisonBonus();

};

class DLL_LINKAGE CGArtifact : public CArmedInstance
{
	ArtifactInstanceID storedArtifact;
public:
	using CArmedInstance::CArmedInstance;

	MetaString message;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;

	std::string getObjectName() const override;
	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;

	void pick(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;
	void initObj(IGameRandomizer & gameRandomizer) override;
	void pickRandomObject(IGameRandomizer & gameRandomizer) override;

	BattleField getBattlefield() const override;

	ArtifactID getArtifactType() const;
	const CArtifactInstance * getArtifactInstance() const;
	void setArtifactInstance(const CArtifactInstance *);

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message;
		if (h.saving || h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & storedArtifact;
		}
		else
		{
			std::shared_ptr<CArtifactInstance> pointer;
			h & pointer;
			if (pointer->getId() == ArtifactInstanceID())
				CArtifactInstance::saveCompatibilityFixArtifactID(pointer);
			storedArtifact = pointer->getId();
		}
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGMine : public CArmedInstance, public IOwnableObject
{
public:
	GameResID producedResource;
	ui32 producedQuantity;
	std::set<GameResID> abandonedMineResources;
	bool isAbandoned() const;
	
	std::shared_ptr<MineInstanceConstructor> getResourceHandler() const;
private:
	using CArmedInstance::CArmedInstance;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;

	void flagMine(IGameEventCallback & gameEvents, const PlayerColor & player) const;
	void initObj(IGameRandomizer & gameRandomizer) override;

	std::string getObjectName() const override;
	std::string getHoverText(PlayerColor player) const override;

public:
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & producedResource;
		h & producedQuantity;
		h & abandonedMineResources;
	}
	ui32 defaultResProduction() const;
	ui32 getProducedQuantity() const;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

struct DLL_LINKAGE TeleportChannel : public Serializeable
{
	enum EPassability {UNKNOWN, IMPASSABLE, PASSABLE};

	std::vector<ObjectInstanceID> entrances;
	std::vector<ObjectInstanceID> exits;
	EPassability passability = EPassability::UNKNOWN;

	template <typename Handler> void serialize(Handler &h)
	{
		h & entrances;
		h & exits;
		h & passability;
	}
};

class DLL_LINKAGE CGTeleport : public CGObjectInstance
{
	bool isChannelEntrance(const ObjectInstanceID & id) const;
	bool isChannelExit(const ObjectInstanceID & id) const;

protected:
	enum EType {UNKNOWN, ENTRANCE, EXIT, BOTH};
	EType type = EType::UNKNOWN;

	ObjectInstanceID getRandomExit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const;


public:
	using CGObjectInstance::CGObjectInstance;

	TeleportChannelID channel;

	bool isEntrance() const;
	bool isExit() const;

	std::vector<ObjectInstanceID> getAllEntrances(bool excludeCurrent = false) const;
	std::vector<ObjectInstanceID> getAllExits(bool excludeCurrent = false) const;

	virtual void teleportDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const = 0;

	static bool isTeleport(const CGObjectInstance * dst);
	static bool isConnected(const CGTeleport * src, const CGTeleport * dst);
	static bool isConnected(const CGObjectInstance * src, const CGObjectInstance * dst);
	static void addToChannel(std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > &channelsList, const CGTeleport * obj);
	static std::vector<ObjectInstanceID> getPassableExits(const IGameInfoCallback & gameInfo, const CGHeroInstance * h, std::vector<ObjectInstanceID> exits);
	static bool isExitPassable(const IGameInfoCallback & gameInfo, const CGHeroInstance * h, const CGObjectInstance * obj);

	template <typename Handler> void serialize(Handler &h)
	{
		h & type;
		h & channel;
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGMonolith : public CGTeleport
{
	TeleportChannelID findMeChannel(const std::vector<Obj> & IDs, MapObjectSubID SubID) const;

protected:
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void teleportDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;

public:
	using CGTeleport::CGTeleport;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGTeleport&>(*this);
	}
};

class DLL_LINKAGE CGSubterraneanGate : public CGMonolith
{
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;

public:
	using CGMonolith::CGMonolith;

	static void postInit(IGameInfoCallback * cb);

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGMonolith&>(*this);
	}
};

class DLL_LINKAGE CGWhirlpool : public CGMonolith
{
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void teleportDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const override;
	static bool isProtected( const CGHeroInstance * h );

public:
	using CGMonolith::CGMonolith;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGMonolith&>(*this);
	}
};

class DLL_LINKAGE CGSirens : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBoat : public CGObjectInstance, public CBonusSystemNode
{
	ObjectInstanceID boardedHeroID;

public:
	using CGObjectInstance::CGObjectInstance;

	ui8 direction;
	bool onboardAssaultAllowed; //if true, hero can attack units from transport
	bool onboardVisitAllowed; //if true, hero can visit objects from transport
	EPathfindingLayer layer;
	
	//animation filenames. If empty - animations won't be used
	AnimationPath actualAnimation; //for OH3 boats those have actual animations
	AnimationPath overlayAnimation; //waves animations
	std::array<AnimationPath, PlayerColor::PLAYER_LIMIT_I> flagAnimations;

	CGBoat(IGameInfoCallback * cb);
	bool isCoastVisitable() const override;

	void setBoardedHero(const CGHeroInstance * hero);
	const CGHeroInstance * getBoardedHero() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<CBonusSystemNode&>(*this);
		h & direction;
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & boardedHeroID;
		}
		else
		{
			std::shared_ptr<CGObjectInstance> ptr;
			h & ptr;
			boardedHeroID = ptr ? ptr->id : ObjectInstanceID();
		}

		h & layer;
		h & onboardAssaultAllowed;
		h & onboardVisitAllowed;
		h & actualAnimation;
		h & overlayAnimation;
		h & flagAnimations;
	}
};

class DLL_LINKAGE CGShipyard : public CGObjectInstance, public IShipyard, public IOwnableObject
{
	friend class ShipyardInstanceConstructor;

	BoatId createdBoat;

protected:
	void getOutOffsets(std::vector<int3> & offsets) const override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	const IObjectInterface * getObject() const override;
	BoatId getBoatType() const override;

	const IOwnableObject * asOwnable() const final;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

public:
	using CGObjectInstance::CGObjectInstance;

	template<typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & createdBoat;
	}

protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGMagi : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	void initObj(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGDenOfthieves : public CGObjectInstance
{
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
public:
	using CGObjectInstance::CGObjectInstance;
};

class DLL_LINKAGE CGObelisk : public CTeamVisited
{
public:
	using CTeamVisited::CTeamVisited;

	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void initObj(IGameRandomizer & gameRandomizer) override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getObjectDescription(PlayerColor player) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CTeamVisited&>(*this);
	}
protected:
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
};

class DLL_LINKAGE CGTerrainPatch : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	bool isTile2Terrain() const override
	{
		return true;
	}
};

class DLL_LINKAGE HillFort : public CGObjectInstance, public ICreatureUpgrader
{
	friend class HillFortInstanceConstructor;

	std::vector<int> upgradeCostPercentage;

protected:
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const override;

public:
	using CGObjectInstance::CGObjectInstance;

	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;

	std::string getDescriptionToolTip() const;
	std::string getUnavailableUpgradeMessage() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & upgradeCostPercentage;
	}
};

VCMI_LIB_NAMESPACE_END
