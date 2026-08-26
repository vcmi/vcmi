/*
 * CGObjectInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IObjectInterface.h"

#include "../bonuses/BonusEnum.h"
#include "../constants/EntityIdentifiers.h"
#include "../filesystem/ResourcePath.h"
#include "../int3.h"
#include "../texts/MetaString.h"

#include <vcmi/scripting/ApiTags.h>

struct Component;
class JsonSerializeFormat;
class ObjectTemplate;
class CMap;
class CGameState;
class Quest;
class IQuestSource;
class AObjectTypeHandler;
using TObjectTypeHandler = std::shared_ptr<AObjectTypeHandler>;

class DLL_LINKAGE CGObjectInstance : public IObjectInterface, public scripting::ApiRawPointer<CGObjectInstance>
{
public:
	/// Type of object, e.g. town, hero, creature.
	MapObjectID ID;
	/// Subtype of object, depends on type
	MapObjectSubID subID;
	/// Current owner of an object (when below PLAYER_LIMIT)
	PlayerColor tempOwner;
	/// Index of object in map's list of objects
	ObjectInstanceID id;
	/// Defines appearance of object on map (animation, blocked tiles, blit order, etc)
	std::shared_ptr<const ObjectTemplate> appearance;

	/// Position of bottom-right corner of object on map
	int3 pos;

	std::string instanceName;

	CGObjectInstance(IGameInfoCallback *cb);
	~CGObjectInstance() override;

	MapObjectID getObjGroupIndex() const override;
	MapObjectSubID getObjTypeIndex() const override;

	std::string getTypeName() const;
	std::string getSubtypeName() const;

	/// "center" tile from which the sight distance is calculated
	int3 getSightCenter() const;
	/// If true hero can visit this object only from neighbouring tiles and can't stand on this object
	bool blockVisit;
	bool removable;
	ui32 rmgValue = 0; // Value assigned during random map generation (not serialized)

	PlayerColor getOwner() const override
	{
		return this->tempOwner;
	}
	void setOwner(const PlayerColor & ow);
	void setAnchorPos(int3 pos);

	/** APPEARANCE ACCESSORS **/

	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	int3 visitablePos() const override;
	int3 anchorPos() const override;
	int3 getTopVisiblePos() const;

	bool visitableAt(const int3 & pos) const; //returns true if object is visitable at location
	bool blockingAt (const int3 & pos) const; //returns true if object is blocking location
	bool coveringAt (const int3 & pos) const; //returns true if object covers with picture location

	/// Visibility of this object to `player`, used as the fog-of-war gate.
	virtual bool isVisibleFor(PlayerColor player) const;

	/// Map-script handler to run instead of onHeroVisit, or empty when this object has none.
	virtual std::string getVisitScriptHandler() const { return {}; }

	std::set<int3> getBlockedPos() const; //returns set of positions blocked by this object
	const std::set<int3> & getBlockedOffsets() const; //returns set of relative positions blocked by this object

	/// returns true if object is visitable
	bool isVisitable() const;

	/// If true hero can visit this object only from neighbouring tiles and can't stand on this object
	virtual bool isBlockedVisitable() const;

	// If true, can be possibly removed from the map
	virtual bool isRemovable() const;

	/// If true this object can be visited by hero standing on the coast
	virtual bool isCoastVisitable() const;

	virtual BattleField getBattlefield() const;

	/// terrain the battle at this object takes place on, or NONE to use the map tile's terrain
	virtual TerrainId getBattleTerrain() const;

	virtual bool isTile2Terrain() const { return false; }

	std::optional<AudioPath> getAmbientSound(vstd::RNG & rng) const;
	std::optional<AudioPath> getVisitSound(vstd::RNG & rng) const;
	std::optional<AudioPath> getRemovalSound(vstd::RNG & rng) const;

	virtual TObjectTypeHandler getObjectHandler() const;

	/** VIRTUAL METHODS **/

	/// Coarse, player-level passability. Required (cannot be dropped for the hero
	/// overload): the AI pathfinder builds one accessibility graph per player for a
	/// whole hero-chain (AINodeStorage / PathfinderUtil::evaluateAccessibility),
	/// so no single hero is available there.
	virtual bool passableFor(PlayerColor color) const;
	/// Hero-aware passability, used where a concrete hero exists (e.g. a toll Quest
	/// Gate that needs the hero's inventory). Default forwards to the colour overload.
	virtual bool passableFor(const CGHeroInstance * hero) const;
	/// Quest-source view of this object (active quest, passage blocking, quest-log
	/// grouping) for the AI, pathfinder and quest log; nullptr if it carries no quest.
	virtual const IQuestSource * asQuestSource() const { return nullptr; }
	/// Range of revealed map around this object, counting from getSightCenter()
	virtual int getSightRadius() const;
	/// returns (x,y,0) offset to a visitable tile of object
	virtual int3 getVisitableOffset() const;

	/// Returns generic name of object, without any player-specific info
	virtual MetaString getObjectName() const;

	/// Language-neutral identity of this object type, for logs and other developer-facing text
	std::string getObjectNameTextID() const;

	/// Returns hover name for situation when there are no selected heroes. Default = object name
	virtual MetaString getHoverText(PlayerColor player) const;
	/// Returns hero-specific hover name, including visited/not visited info. Default = player-specific name
	virtual MetaString getHoverText(const CGHeroInstance * hero) const;

	virtual MetaString getPopupText(PlayerColor player) const;
	virtual MetaString getPopupText(const CGHeroInstance * hero) const;

	virtual std::vector<Component> getPopupComponents(PlayerColor player) const;
	virtual std::vector<Component> getPopupComponents(const CGHeroInstance * hero) const;

	const IOwnableObject * asOwnable() const override;

	/** OVERRIDES OF IObjectInterface **/

	void initObj(IGameRandomizer & gameRandomizer) override;
	void pickRandomObject(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	/// method for synchronous update. Note: For new properties classes should override setPropertyDer instead
	void setProperty(ObjProperty what, ObjPropertyID identifier) final;

	virtual void afterAddToMap(CMap * map){};
	virtual void afterRemoveFromMap(CMap * map){};
	virtual void attachToBonusSystem(CGameState & gs){};
	virtual void detachFromBonusSystem(CGameState & gs){};
	virtual void restoreBonusSystem(CGameState & gs){};

	///Entry point of binary (de-)serialization
	template <typename Handler> void serialize(Handler &h)
	{
		h & instanceName;
		h & pos;
		h & ID;
		subID.serializeIdentifier(h, ID);
		h & id;
		h & tempOwner;
		h & blockVisit;
		h & removable;
		h & appearance;
		//definfo is handled by map serializer
	}

	///Entry point of Json (de-)serialization
	void serializeJson(JsonSerializeFormat & handler);
	virtual void updateFrom(const JsonNode & data);

protected:
	/// virtual method that allows synchronously update object state on server and all clients
	virtual void setPropertyDer(ObjProperty what, ObjPropertyID identifier);

	/// Called mostly during map randomization to turn random object into a regular one (e.g. "Random Monster" into "Pikeman")
	void setType(MapObjectID ID, MapObjectSubID subID);

	/// Gives dummy bonus from this object to hero. Can be used to track visited state
	void giveDummyBonus(IGameEventCallback & gameEvents, const ObjectInstanceID & heroID, BonusDuration::Type duration = BonusDuration::ONE_DAY) const;

	///Serialize object-type specific options
	virtual void serializeJsonOptions(JsonSerializeFormat & handler);

	void serializeJsonOwner(JsonSerializeFormat & handler);
};
