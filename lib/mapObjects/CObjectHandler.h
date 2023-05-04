/*
 * CObjectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectTemplate.h"

#include "../int3.h"
#include "../NetPacksBase.h"
#include "../ResourceSet.h"

#include "../bonuses/Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class IGameCallback;
class CGObjectInstance;
struct MetaString;
struct BattleResult;
class JsonSerializeFormat;
class CRandomGenerator;
class CMap;
class JsonNode;

// This one teleport-specific, but has to be available everywhere in callbacks and netpacks
// For now it's will be there till teleports code refactored and moved into own file
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

class DLL_LINKAGE IObjectInterface
{
public:
	static IGameCallback *cb;

	virtual ~IObjectInterface() = default;

	virtual int32_t getObjGroupIndex() const = 0;
	virtual int32_t getObjTypeIndex() const = 0;

	virtual PlayerColor getOwner() const = 0;
	virtual int3 visitablePos() const = 0;
	virtual int3 getPosition() const = 0;

	virtual void onHeroVisit(const CGHeroInstance * h) const;
	virtual void onHeroLeave(const CGHeroInstance * h) const;
	virtual void newTurn(CRandomGenerator & rand) const;
	virtual void initObj(CRandomGenerator & rand); //synchr
	virtual void setProperty(ui8 what, ui32 val);//synchr

	//Called when queries created DURING HERO VISIT are resolved
	//First parameter is always hero that visited object and triggered the query
	virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
	virtual void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const;
	virtual void garrisonDialogClosed(const CGHeroInstance *hero) const;
	virtual void heroLevelUpDone(const CGHeroInstance *hero) const;

	//unified helper to show info dialog for object owner
	virtual void showInfoDialog(const ui32 txtID, const ui16 soundID = 0, EInfoWindowMode mode = EInfoWindowMode::AUTO) const;

	//unified helper to show a specific window
	static void openWindow(const EOpenWindowMode type, const int id1, const int id2 = -1);

	//unified interface, AI helpers
	virtual bool wasVisited (PlayerColor player) const;
	virtual bool wasVisited (const CGHeroInstance * h) const;

	static void preInit(); //called before objs receive their initObj
	static void postInit();//called after objs receive their initObj

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logGlobal->error("IObjectInterface serialized, unexpected, should not happen!");
	}
};

class DLL_LINKAGE IBoatGenerator
{
public:
	const CGObjectInstance *o;

	IBoatGenerator(const CGObjectInstance *O);
	virtual ~IBoatGenerator() = default;

	virtual BoatId getBoatType() const; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	virtual void getOutOffsets(std::vector<int3> &offsets) const =0; //offsets to obj pos when we boat can be placed
	int3 bestLocation() const; //returns location when the boat should be placed

	enum EGeneratorState {GOOD, BOAT_ALREADY_BUILT, TILE_BLOCKED, NO_WATER};
	EGeneratorState shipyardStatus() const; //0 - can buid, 1 - there is already a boat at dest tile, 2 - dest tile is blocked, 3 - no water
	void getProblemText(MetaString &out, const CGHeroInstance *visitor = nullptr) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & o;
	}
};

class DLL_LINKAGE IShipyard : public IBoatGenerator
{
public:
	IShipyard(const CGObjectInstance *O);

	virtual void getBoatCost(TResources & cost) const;

	static const IShipyard *castFrom(const CGObjectInstance *obj);
	static IShipyard *castFrom(CGObjectInstance *obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IBoatGenerator&>(*this);
	}
};

class DLL_LINKAGE CGObjectInstance : public IObjectInterface
{
public:
	/// Position of bottom-right corner of object on map
	int3 pos;
	/// Type of object, e.g. town, hero, creature.
	Obj ID;
	/// Subtype of object, depends on type
	si32 subID;
	/// Current owner of an object (when below PLAYER_LIMIT)
	PlayerColor tempOwner;
	/// Index of object in map's list of objects
	ObjectInstanceID id;
	/// Defines appearance of object on map (animation, blocked tiles, blit order, etc)
	std::shared_ptr<const ObjectTemplate> appearance;
	/// If true hero can visit this object only from neighbouring tiles and can't stand on this object
	bool blockVisit;

	std::string instanceName;
	std::string typeName;
	std::string subTypeName;

	CGObjectInstance(); //TODO: remove constructor
	~CGObjectInstance() override;

	int32_t getObjGroupIndex() const override;
	int32_t getObjTypeIndex() const override;

	/// "center" tile from which the sight distance is calculated
	int3 getSightCenter() const;

	PlayerColor getOwner() const override 
	{
		return this->tempOwner;
	}
	void setOwner(const PlayerColor & ow);

	/** APPEARANCE ACCESSORS **/

	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	bool visitableAt(int x, int y) const; //returns true if object is visitable at location (x, y) (h3m pos)
	int3 visitablePos() const override;
	int3 getPosition() const override;
	int3 getTopVisiblePos() const;
	bool blockingAt(int x, int y) const; //returns true if object is blocking location (x, y) (h3m pos)
	bool coveringAt(int x, int y) const; //returns true if object covers with picture location (x, y) (h3m pos)
	std::set<int3> getBlockedPos() const; //returns set of positions blocked by this object
	std::set<int3> getBlockedOffsets() const; //returns set of relative positions blocked by this object
	bool isVisitable() const; //returns true if object is visitable

	virtual BattleField getBattlefield() const;

	virtual bool isTile2Terrain() const { return false; }

	std::optional<std::string> getAmbientSound() const;
	std::optional<std::string> getVisitSound() const;
	std::optional<std::string> getRemovalSound() const;

	/** VIRTUAL METHODS **/

	/// Returns true if player can pass through visitable tiles of this object
	virtual bool passableFor(PlayerColor color) const;
	/// Range of revealed map around this object, counting from getSightCenter()
	virtual int getSightRadius() const;
	/// returns (x,y,0) offset to a visitable tile of object
	virtual int3 getVisitableOffset() const;
	/// Called mostly during map randomization to turn random object into a regular one (e.g. "Random Monster" into "Pikeman")
	virtual void setType(si32 ID, si32 subID);

	/// returns text visible in status bar with specific hero/player active.

	/// Returns generic name of object, without any player-specific info
	virtual std::string getObjectName() const;

	/// Returns hover name for situation when there are no selected heroes. Default = object name
	virtual std::string getHoverText(PlayerColor player) const;
	/// Returns hero-specific hover name, including visited/not visited info. Default = player-specific name
	virtual std::string getHoverText(const CGHeroInstance * hero) const;

	/** OVERRIDES OF IObjectInterface **/

	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	/// method for synchronous update. Note: For new properties classes should override setPropertyDer instead
	void setProperty(ui8 what, ui32 val) final;

	virtual void afterAddToMap(CMap * map);
	virtual void afterRemoveFromMap(CMap * map);

	///Entry point of binary (de-)serialization
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & instanceName;
		h & typeName;
		h & subTypeName;
		h & pos;
		h & ID;
		h & subID;
		h & id;
		h & tempOwner;
		h & blockVisit;
		h & appearance;
		//definfo is handled by map serializer
	}

	///Entry point of Json (de-)serialization
	void serializeJson(JsonSerializeFormat & handler);
	virtual void updateFrom(const JsonNode & data);

protected:
	/// virtual method that allows synchronously update object state on server and all clients
	virtual void setPropertyDer(ui8 what, ui32 val);

	/// Gives dummy bonus from this object to hero. Can be used to track visited state
	void giveDummyBonus(const ObjectInstanceID & heroID, BonusDuration duration = BonusDuration::ONE_DAY) const;

	///Serialize object-type specific options
	virtual void serializeJsonOptions(JsonSerializeFormat & handler);

	void serializeJsonOwner(JsonSerializeFormat & handler);
};

/// function object which can be used to find an object with an specific sub ID
class CGObjectInstanceBySubIdFinder
{
public:
	CGObjectInstanceBySubIdFinder(CGObjectInstance * obj);
	bool operator()(CGObjectInstance * obj) const;

private:
	CGObjectInstance * obj;
};

class DLL_LINKAGE CObjectHandler
{
public:
	std::vector<ui32> resVals; //default values of resources in gold

	CObjectHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & resVals;
	}
};

VCMI_LIB_NAMESPACE_END
