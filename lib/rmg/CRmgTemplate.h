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
#include "MonsterJoiningOptions.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"

#ifdef ENABLE_TEMPLATE_EDITOR
class TemplateEditor;
#endif

class JsonSerializeFormat;
class GameLibrary;
struct CompoundMapObjectID;

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

enum class EZoneLevel // used to force zone placement on surface or underground
{
	AUTOMATIC = 0,
	SURFACE = 1,
	UNDERGROUND = 2
};

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
	FORCE_PORTAL,
	ONE_WAY
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
	/// True if this connection has to be realized as an actual passage between two distinct zones.
	bool needsPassage() const;
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

	class CTownInfo
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
	std::set<TerrainId> getTerrainTypes(const GameLibrary & library) const;
	void setTerrainTypes(const std::set<TerrainId> & value);
	std::set<TerrainId> getDefaultTerrainTypes(const GameLibrary & library) const;

	const CTownInfo & getPlayerTowns() const;
	void setPlayerTowns(const CTownInfo & value);
	const CTownInfo & getNeutralTowns() const;
	void setNeutralTowns(const CTownInfo & value);
	bool isMatchTerrainToTown() const;
	void setMatchTerrainToTown(bool value);
	const std::vector<CTownHints> & getTownHints() const;
	void setTownHints(const std::vector<CTownHints> & value);
	std::set<FactionID> getTownTypes() const;
	std::set<FactionID> getTownTypes(const GameLibrary & library) const;
	void setTownTypes(const std::set<FactionID> & value);
	std::set<FactionID> getBannedTownTypes() const;
	void setBannedTownTypes(const std::set<FactionID> & value);

	std::set<FactionID> getDefaultTownTypes(const GameLibrary & library) const;
	std::set<FactionID> getMonsterTypes() const;

	void setMonsterTypes(const std::set<FactionID> & value);

	void setMinesInfo(const std::map<GameResID, ui16> & value);
	std::map<GameResID, ui16> getMinesInfo() const;

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

	void serializeJson(JsonSerializeFormat & handler, const CObjectClassesHandler & objects);
	
	EMonsterStrength::EMonsterStrength monsterStrength;
	
	bool areTownsSameType() const;

	// Copy whole custom object config from another zone
	ObjectConfig getCustomObjects() const;
	void setCustomObjects(const ObjectConfig & value);
	TRmgTemplateZoneId getCustomObjectsLikeZone() const;
	MonsterJoiningOptions getMonsterJoining() const;
	void setMonsterJoining(const MonsterJoiningOptions & value);
	TRmgTemplateZoneId getTownsLikeZone() const;

	Point getVisiblePosition() const;
	void setVisiblePosition(Point value);

	float getVisibleSize() const;
	void setVisibleSize(float value);

	EZoneLevel getForcedLevel() const;
	void setForcedLevel(EZoneLevel value);

protected:
	TRmgTemplateZoneId id;
	ETemplateZoneType type;
	int size;
	ui32 maxTreasureValue;
	std::optional<int> owner;

	Point visiblePosition;
	float visibleSize;

	ObjectConfig objectConfig;
	MonsterJoiningOptions monsterJoining;
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

	std::map<GameResID, ui16> mines; //obligatory mines to spawn in this zone

	std::vector<CTreasureInfo> treasureInfo;

	std::vector<TRmgTemplateZoneId> connectedZoneIds; //list of adjacent zone ids
	std::vector<ZoneConnection> connectionDetails; //list of connections linked to that zone

	TRmgTemplateZoneId townsLikeZone;
	TRmgTemplateZoneId minesLikeZone;
	TRmgTemplateZoneId terrainTypeLikeZone;
	TRmgTemplateZoneId treasureLikeZone;
	TRmgTemplateZoneId customObjectsLikeZone;
	EZoneLevel forcedLevel;
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
	std::string getDescriptionTextID() const;
	std::string getDescriptionTranslated() const;

	const CPlayerCountRange & getPlayers() const;
	const CPlayerCountRange & getHumanPlayers() const;
	std::pair<int3, int3> getMapSizes() const;
	const Zones & getZones() const;
	const JsonNode & getMapSettings() const;
	const std::vector<rmg::ZoneConnection> & getConnectedZoneIds() const;
	float getPenroseTileScale() const { return penroseTileScale; }

	const std::set<SpellID> & getBannedSpells() const { return banned.spells; }
	const std::set<ArtifactID> & getBannedArtifacts() const { return banned.artifacts; }
	const std::set<SecondarySkill> & getBannedSkills() const { return banned.skills; }
	const std::set<HeroTypeID> & getBannedHeroes() const { return banned.heroes; }
	const std::set<SpellID> & getEnabledSpells() const { return enabled.spells; }
	const std::set<ArtifactID> & getEnabledArtifacts() const { return enabled.artifacts; }
	const std::set<SecondarySkill> & getEnabledSkills() const { return enabled.skills; }
	const std::set<HeroTypeID> & getEnabledHeroes() const { return enabled.heroes; }
	const ObjectConfig & getCustomObjects() const { return objectConfig; }
	const rmg::MonsterJoiningOptions & getMonsterJoining() const { return monsterJoining; }

	void validate() const; /// Tests template on validity and throws exception on failure

	void serializeJson(JsonSerializeFormat & handler, const CObjectClassesHandler & objects);
	void afterLoad(const GameLibrary & library);

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
	float penroseTileScale = 1.0f;
	std::unique_ptr<JsonNode> mapSettings;

	/// Entities named by one template list: the bannedX or enabledX fields
	struct EntitySets
	{
		std::set<SpellID> spells;
		std::set<ArtifactID> artifacts;
		std::set<SecondarySkill> skills;
		std::set<HeroTypeID> heroes;
	};

	EntitySets banned;
	EntitySets enabled;
	ObjectConfig objectConfig;
	rmg::MonsterJoiningOptions monsterJoining;

	std::set<TerrainId> inheritTerrainType(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::map<GameResID, ui16> inheritMineTypes(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::vector<CTreasureInfo> inheritTreasureInfo(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);

	void inheritTownProperties(const GameLibrary & library, std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);

	void serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName);
	void serializePlayers(JsonSerializeFormat & handler, CPlayerCountRange & value, const std::string & fieldName);

	template<typename T, typename Getter>
	T inheritZoneProperty(std::shared_ptr<rmg::ZoneOptions> zone, 
						  Getter getter,
						  void (rmg::ZoneOptions::*setter)(const T&),
						  TRmgTemplateZoneId (rmg::ZoneOptions::*inheritFrom)() const,
						  const std::string& propertyString,
						  uint32_t iteration = 0);

};
