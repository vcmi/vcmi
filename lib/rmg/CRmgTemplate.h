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
#include "../Terrain.h"
#include "CMapGenOptions.h"

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

	ZoneOptions & operator=(const ZoneOptions & other);

	TRmgTemplateZoneId getId() const;
	void setId(TRmgTemplateZoneId value);

	ETemplateZoneType::ETemplateZoneType getType() const;
	void setType(ETemplateZoneType::ETemplateZoneType value);
	
	int getSize() const;
	void setSize(int value);
	boost::optional<int> getOwner() const;

	const std::set<Terrain> & getTerrainTypes() const;
	void setTerrainTypes(const std::set<Terrain> & value);

	const CTownInfo & getPlayerTowns() const;
	const CTownInfo & getNeutralTowns() const;
	std::set<TFaction> getDefaultTownTypes() const;
	const std::set<TFaction> & getTownTypes() const;
	const std::set<TFaction> & getMonsterTypes() const;

	void setTownTypes(const std::set<TFaction> & value);
	void setMonsterTypes(const std::set<TFaction> & value);

	void setMinesInfo(const std::map<TResource, ui16> & value);
	std::map<TResource, ui16> getMinesInfo() const;

	void setTreasureInfo(const std::vector<CTreasureInfo> & value);
	void addTreasureInfo(const CTreasureInfo & value);
	const std::vector<CTreasureInfo> & getTreasureInfo() const;

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
	boost::optional<int> owner;
	CTownInfo playerTowns;
	CTownInfo neutralTowns;
	bool matchTerrainToTown;
	std::set<Terrain> terrainTypes;
	bool townsAreSameType;

	std::set<TFaction> townTypes;
	std::set<TFaction> monsterTypes;

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
	~CRmgTemplate();

	bool matchesSize(const int3 & value) const;
	bool isWaterContentAllowed(EWaterContent::EWaterContent waterContent) const;
	const std::set<EWaterContent::EWaterContent> & getWaterContentAllowed() const;

	void setId(const std::string & value);
	const std::string & getName() const;

	const CPlayerCountRange & getPlayers() const;
	const CPlayerCountRange & getCpuPlayers() const;
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
	void serializeSize(JsonSerializeFormat & handler, int3 & value, const std::string & fieldName);
	void serializePlayers(JsonSerializeFormat & handler, CPlayerCountRange & value, const std::string & fieldName);
};
