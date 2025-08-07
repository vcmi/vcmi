/*
 * CRmgTemplate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../int3.h"
#include "../GameConstants.h"
#include "../Point.h"
#include "../ResourceSet.h"
#include "ObjectInfo.h"
#include "ObjectConfig.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;
struct CompoundMapObjectID;
class TemplateEditor;

enum class ETemplateZoneType
{
	PLAYER_START,
	CPU_START,
	TREASURE,
	JUNCTION,
	WATER,
	SEALED
};

namespace EWaterContent // Not enum class, because it's used in method RandomMapTab::setMapGenOptions
{ // defined in client\lobby\RandomMapTab.cpp and probably in other similar places
	enum EWaterContent // as an argument of CToggleGroup::setSelected(int id) from \client\widgets\Buttons.cpp 
	{
		RANDOM = -1,
		NONE,
		NORMAL,
		ISLANDS
	};
}

namespace EMonsterStrength // used as int in monster generation procedure and in map description for the generated random map
{
	enum EMonsterStrength
	{
		ZONE_NONE = -3,
		RANDOM = -2,
		ZONE_WEAK = -1,
		ZONE_NORMAL = 0,
		ZONE_STRONG = 1,
		GLOBAL_WEAK = 2,
		GLOBAL_NORMAL = 3,
		GLOBAL_STRONG = 4
	};
}

class DLL_LINKAGE CTreasureInfo
{
public:
	ui32 min;
	ui32 max;
	ui16 density;
	CTreasureInfo();
	CTreasureInfo(ui32 min, ui32 max, ui16 density);

	bool operator ==(const CTreasureInfo & other) const;

	void serializeJson(JsonSerializeFormat & handler);
};

namespace rmg
{

enum class EConnectionType
{
	GUARDED = 0, //default
	FICTIVE,
	REPULSIVE,
	WIDE,
	FORCE_PORTAL
};

enum class ERoadOption
{
	ROAD_RANDOM = 0,
	ROAD_TRUE,
	ROAD_FALSE
};

class DLL_LINKAGE ZoneConnection
{
#ifdef ENABLE_TEMPLATE_EDITOR
	friend class ::TemplateEditor;
#endif

public:

	ZoneConnection();

	int getId() const;
	void setId(int id);
	TRmgTemplateZoneId getZoneA() const;
	TRmgTemplateZoneId getZoneB() const;
	TRmgTemplateZoneId getOtherZoneId(TRmgTemplateZoneId id) const;
	int getGuardStrength() const;
	rmg::EConnectionType getConnectionType() const;
	rmg::ERoadOption getRoadOption() const;
	void setRoadOption(rmg::ERoadOption roadOption);

	void serializeJson(JsonSerializeFormat & handler);
	
	friend bool operator==(const ZoneConnection &, const ZoneConnection &);
	friend bool operator<(const ZoneConnection &, const ZoneConnection &);
private:
	int id;
	TRmgTemplateZoneId zoneA;
	TRmgTemplateZoneId zoneB;
	int guardStrength;
	rmg::EConnectionType connectionType;
	rmg::ERoadOption hasRoad;
};

class DLL_LINKAGE ZoneOptions
{
#ifdef ENABLE_TEMPLATE_EDITOR
	friend class ::TemplateEditor;
#endif

public:
	static const TRmgTemplateZoneId NO_ZONE;

	class DLL_LINKAGE CTownInfo
	{
#ifdef ENABLE_TEMPLATE_EDITOR
		friend class ::TemplateEditor;
#endif
	public:
		CTownInfo();

		int getTownCount() const;
		int getCastleCount() const;
		int getTownDensity() const;
		int getCastleDensity() const;

		void serializeJson(JsonSerializeFormat & handler);

	private:
		int townCount;
		int castleCount;
		int townDensity;
		int castleDensity;

		// TODO: Copy from another zone once its randomized

		TRmgTemplateZoneId townTypesLikeZone = NO_ZONE;
		TRmgTemplateZoneId townTypesNotLikeZone = NO_ZONE;
		TRmgTemplateZoneId townTypesRelatedToZoneTerrain = NO_ZONE;
	};

	class DLL_LINKAGE CTownHints
	{
	public:
		CTownHints();
		// TODO: Make private
		TRmgTemplateZoneId likeZone = NO_ZONE;
		std::vector<TRmgTemplateZoneId> notLikeZone;
		TRmgTemplateZoneId relatedToZoneTerrain = NO_ZONE;

		void serializeJson(JsonSerializeFormat & handler);
	};

	ZoneOptions();

	TRmgTemplateZoneId getId() const;
	void setId(TRmgTemplateZoneId value);

	ETemplateZoneType getType() const;
	void setType(ETemplateZoneType value);
	
	int getSize() const;
	void setSize(int value);
	std::optional<int> getOwner() const;

	std::set<TerrainId> getTerrainTypes() const;
	void setTerrainTypes(const std::set<TerrainId> & value);
	std::set<TerrainId> getDefaultTerrainTypes() const;

	const CTownInfo & getPlayerTowns() const;
	void setPlayerTowns(const CTownInfo & value);
	const CTownInfo & getNeutralTowns() const;
	void setNeutralTowns(const CTownInfo & value);
	bool isMatchTerrainToTown() const;
	void setMatchTerrainToTown(bool value);
	const std::vector<CTownHints> & getTownHints() const;
	void setTownHints(const std::vector<CTownHints> & value);
	std::set<FactionID> getTownTypes() const;
	void setTownTypes(const std::set<FactionID> & value);
	std::set<FactionID> getBannedTownTypes() const;
	void setBannedTownTypes(const std::set<FactionID> & value);

	std::set<FactionID> getDefaultTownTypes() const;
	std::set<FactionID> getMonsterTypes() const;

	void setMonsterTypes(const std::set<FactionID> & value);

	void setMinesInfo(const std::map<TResource, ui16> & value);
	std::map<TResource, ui16> getMinesInfo() const;

	void setTreasureInfo(const std::vector<CTreasureInfo> & value);
	void addTreasureInfo(const CTreasureInfo & value);
	std::vector<CTreasureInfo> getTreasureInfo() const;
	ui32 getMaxTreasureValue() const;
	void recalculateMaxTreasureValue();

	TRmgTemplateZoneId getMinesLikeZone() const;
	TRmgTemplateZoneId getTerrainTypeLikeZone() const;
	TRmgTemplateZoneId getTreasureLikeZone() const;

	void addConnection(const ZoneConnection & connection);
	std::vector<ZoneConnection> getConnections() const;
	std::vector<ZoneConnection>& getConnectionsRef();
	std::vector<TRmgTemplateZoneId> getConnectedZoneIds() const;

	// Set road option for a specific connection by ID
	void setRoadOption(int connectionId, rmg::ERoadOption roadOption);

	void serializeJson(JsonSerializeFormat & handler);
	
	EMonsterStrength::EMonsterStrength monsterStrength;
	
	bool areTownsSameType() const;

	// Get a group of configured objects
	const std::vector<CompoundMapObjectID> & getBannedObjects() const;
	const std::vector<ObjectConfig::EObjectCategory> & getBannedObjectCategories() const;
	const std::vector<ObjectInfo> & getConfiguredObjects() const;

	// Copy whole custom object config from another zone
	ObjectConfig getCustomObjects() const;
	void setCustomObjects(const ObjectConfig & value);
	TRmgTemplateZoneId getCustomObjectsLikeZone() const;
	TRmgTemplateZoneId getTownsLikeZone() const;

	Point getVisiblePosition() const;
	void setVisiblePosition(Point value);

	float getVisibleSize() const;
	void setVisibleSize(float value);

protected:
	TRmgTemplateZoneId id;
	ETemplateZoneType type;
	int size;
	ui32 maxTreasureValue;
	std::optional<int> owner;

	Point visiblePosition;
	float visibleSize;

	ObjectConfig objectConfig;
	CTownInfo playerTowns;
	CTownInfo neutralTowns;
	bool matchTerrainToTown;
	std::set<TerrainId> terrainTypes;
	std::set<TerrainId> bannedTerrains;
	bool townsAreSameType;
	std::vector<CTownHints> townHints; // For every town present on map

	std::set<FactionID> townTypes;
	std::set<FactionID> bannedTownTypes;
	std::set<FactionID> monsterTypes;
	std::set<FactionID> bannedMonsters;

	std::map<TResource, ui16> mines; //obligatory mines to spawn in this zone

	std::vector<CTreasureInfo> treasureInfo;

	std::vector<TRmgTemplateZoneId> connectedZoneIds; //list of adjacent zone ids
	std::vector<ZoneConnection> connectionDetails; //list of connections linked to that zone

	TRmgTemplateZoneId townsLikeZone;
	TRmgTemplateZoneId minesLikeZone;
	TRmgTemplateZoneId terrainTypeLikeZone;
	TRmgTemplateZoneId treasureLikeZone;
	TRmgTemplateZoneId customObjectsLikeZone;
};

}

/// The CRmgTemplate describes a random map template.
class DLL_LINKAGE CRmgTemplate : boost::noncopyable
{
#ifdef ENABLE_TEMPLATE_EDITOR
	friend class ::TemplateEditor;
#endif

public:
	using Zones = std::map<TRmgTemplateZoneId, std::shared_ptr<rmg::ZoneOptions>>;

	class DLL_LINKAGE CPlayerCountRange
	{
#ifdef ENABLE_TEMPLATE_EDITOR
		friend class ::TemplateEditor;
#endif
	public:
		void addRange(int lower, int upper);
		void addNumber(int value);
		bool isInRange(int count) const;
		std::set<int> getNumbers() const;

		std::string toString() const;
		void fromString(const std::string & value);

		int maxValue() const;
		int minValue() const;

	private:
		std::vector<std::pair<int, int> > range;
	};

	CRmgTemplate();
	~CRmgTemplate();

	bool matchesSize(const int3 & value) const;
	bool isWaterContentAllowed(EWaterContent::EWaterContent waterContent) const;
	const std::set<EWaterContent::EWaterContent> & getWaterContentAllowed() const;

	void setId(const std::string & value);
	void setName(const std::string & value);
	const std::string & getId() const;
	const std::string & getName() const;
	const std::string & getDescription() const;

	const CPlayerCountRange & getPlayers() const;
	const CPlayerCountRange & getHumanPlayers() const;
	std::pair<int3, int3> getMapSizes() const;
	const Zones & getZones() const;
	const JsonNode & getMapSettings() const;
	const std::vector<rmg::ZoneConnection> & getConnectedZoneIds() const;

	const std::set<SpellID> & getBannedSpells() const { return bannedSpells; }
	const std::set<ArtifactID> & getBannedArtifacts() const { return bannedArtifacts; }
	const std::set<SecondarySkill> & getBannedSkills() const { return bannedSkills; }
	const std::set<HeroTypeID> & getBannedHeroes() const { return bannedHeroes; }

	void validate() const; /// Tests template on validity and throws exception on failure

	void serializeJson(JsonSerializeFormat & handler);
	void afterLoad();

private:
	std::string id;
	std::string name;
	std::string description;
	int3 minSize;
	int3 maxSize;
	CPlayerCountRange players;
	CPlayerCountRange humanPlayers;
	Zones zones;
	std::vector<rmg::ZoneConnection> connections;
	std::set<EWaterContent::EWaterContent> allowedWaterContent;
	std::unique_ptr<JsonNode> mapSettings;

	std::set<SpellID> bannedSpells;
	std::set<ArtifactID> bannedArtifacts;
	std::set<SecondarySkill> bannedSkills;
	std::set<HeroTypeID> bannedHeroes;

	std::set<TerrainId> inheritTerrainType(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::map<TResource, ui16> inheritMineTypes(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::vector<CTreasureInfo> inheritTreasureInfo(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);

	void inheritTownProperties(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);

	void serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName);
	void serializePlayers(JsonSerializeFormat & handler, CPlayerCountRange & value, const std::string & fieldName);

	template<typename T>
	T inheritZoneProperty(std::shared_ptr<rmg::ZoneOptions> zone, 
						  T (rmg::ZoneOptions::*getter)() const,
						  void (rmg::ZoneOptions::*setter)(const T&),
						  TRmgTemplateZoneId (rmg::ZoneOptions::*inheritFrom)() const,
						  const std::string& propertyString,
						  uint32_t iteration = 0);

};

VCMI_LIB_NAMESPACE_END
