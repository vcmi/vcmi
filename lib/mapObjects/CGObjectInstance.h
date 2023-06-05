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
#include "../int3.h"
#include "../bonuses/BonusEnum.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;
class ObjectTemplate;
class CMap;

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
	void giveDummyBonus(const ObjectInstanceID & heroID, BonusDuration::Type duration = BonusDuration::ONE_DAY) const;

	///Serialize object-type specific options
	virtual void serializeJsonOptions(JsonSerializeFormat & handler);

	void serializeJsonOwner(JsonSerializeFormat & handler);
};

VCMI_LIB_NAMESPACE_END
