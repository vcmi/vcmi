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
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;

namespace ETemplateZoneType
{
	enum ETemplateZoneType
	{
		PLAYER_START,
		CPU_START,
		TREASURE,
		JUNCTION,
		WATER
	};
}

namespace EWaterContent
{
	enum EWaterContent
	{
		RANDOM = -1,
		NONE,
		NORMAL,
		ISLANDS
	};
}

namespace EMonsterStrength
{
	enum EMonsterStrength
	{
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

class DLL_LINKAGE ZoneConnection
{
public:
	ZoneConnection();

	TRmgTemplateZoneId getZoneA() const;
	TRmgTemplateZoneId getZoneB() const;
	int getGuardStrength() const;

	void serializeJson(JsonSerializeFormat & handler);
	
	friend bool operator==(const ZoneConnection &, const ZoneConnection &);
private:
	TRmgTemplateZoneId zoneA;
	TRmgTemplateZoneId zoneB;
	int guardStrength;
};

class DLL_LINKAGE ZoneOptions
{
public:
	static const TRmgTemplateZoneId NO_ZONE;

	class DLL_LINKAGE CTownInfo
	{
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
	};

	ZoneOptions();

	TRmgTemplateZoneId getId() const;
	void setId(TRmgTemplateZoneId value);

	ETemplateZoneType::ETemplateZoneType getType() const;
	void setType(ETemplateZoneType::ETemplateZoneType value);
	
	int getSize() const;
	void setSize(int value);
	std::optional<int> getOwner() const;

	const std::set<TerrainId> & getTerrainTypes() const;
	void setTerrainTypes(const std::set<TerrainId> & value);

	const CTownInfo & getPlayerTowns() const;
	const CTownInfo & getNeutralTowns() const;
	std::set<FactionID> getDefaultTownTypes() const;
	const std::set<FactionID> & getTownTypes() const;
	const std::set<FactionID> & getMonsterTypes() const;

	void setTownTypes(const std::set<FactionID> & value);
	void setMonsterTypes(const std::set<FactionID> & value);

	void setMinesInfo(const std::map<TResource, ui16> & value);
	std::map<TResource, ui16> getMinesInfo() const;

	void setTreasureInfo(const std::vector<CTreasureInfo> & value);
	void addTreasureInfo(const CTreasureInfo & value);
	const std::vector<CTreasureInfo> & getTreasureInfo() const;
	ui32 getMaxTreasureValue() const;
	void recalculateMaxTreasureValue();

	TRmgTemplateZoneId getMinesLikeZone() const;
	TRmgTemplateZoneId getTerrainTypeLikeZone() const;
	TRmgTemplateZoneId getTreasureLikeZone() const;

	void addConnection(TRmgTemplateZoneId otherZone);
	std::vector<TRmgTemplateZoneId> getConnections() const;

	void serializeJson(JsonSerializeFormat & handler);
	
	EMonsterStrength::EMonsterStrength zoneMonsterStrength;
	
	bool areTownsSameType() const;
	bool isMatchTerrainToTown() const;

protected:
	TRmgTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int size;
	ui32 maxTreasureValue;
	std::optional<int> owner;
	CTownInfo playerTowns;
	CTownInfo neutralTowns;
	bool matchTerrainToTown;
	std::set<TerrainId> terrainTypes;
	bool townsAreSameType;

	std::set<FactionID> townTypes;
	std::set<FactionID> monsterTypes;

	std::map<TResource, ui16> mines; //obligatory mines to spawn in this zone

	std::vector<CTreasureInfo> treasureInfo;

	std::vector<TRmgTemplateZoneId> connections; //list of adjacent zones

	TRmgTemplateZoneId minesLikeZone;
	TRmgTemplateZoneId terrainTypeLikeZone;
	TRmgTemplateZoneId treasureLikeZone;
};

}

/// The CRmgTemplate describes a random map template.
class DLL_LINKAGE CRmgTemplate
{
public:
	using Zones = std::map<TRmgTemplateZoneId, std::shared_ptr<rmg::ZoneOptions>>;

	class DLL_LINKAGE CPlayerCountRange
	{
	public:
		void addRange(int lower, int upper);
		void addNumber(int value);
		bool isInRange(int count) const;
		std::set<int> getNumbers() const;

		std::string toString() const;
		void fromString(const std::string & value);

	private:
		std::vector<std::pair<int, int> > range;
	};

	CRmgTemplate();

	bool matchesSize(const int3 & value) const;
	bool isWaterContentAllowed(EWaterContent::EWaterContent waterContent) const;
	const std::set<EWaterContent::EWaterContent> & getWaterContentAllowed() const;

	void setId(const std::string & value);
	void setName(const std::string & value);
	const std::string & getId() const;
	const std::string & getName() const;

	const CPlayerCountRange & getPlayers() const;
	const CPlayerCountRange & getCpuPlayers() const;
	std::pair<int3, int3> getMapSizes() const;
	const Zones & getZones() const;
	const std::vector<rmg::ZoneConnection> & getConnections() const;

	void validate() const; /// Tests template on validity and throws exception on failure

	void serializeJson(JsonSerializeFormat & handler);

private:
	std::string id;
	std::string name;
	int3 minSize, maxSize;
	CPlayerCountRange players, cpuPlayers;
	Zones zones;
	std::vector<rmg::ZoneConnection> connections;
	std::set<EWaterContent::EWaterContent> allowedWaterContent;

	void afterLoad();
	std::set<TerrainId> inheritTerrainType(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::map<TResource, ui16> inheritMineTypes(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	std::vector<CTreasureInfo> inheritTreasureInfo(std::shared_ptr<rmg::ZoneOptions> zone, uint32_t iteration = 0);
	void serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName);
	void serializePlayers(JsonSerializeFormat & handler, CPlayerCountRange & value, const std::string & fieldName);
};

VCMI_LIB_NAMESPACE_END
