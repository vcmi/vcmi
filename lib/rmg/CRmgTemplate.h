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

#include "../GameConstants.h"
#include "../ResourceSet.h"
#include "CMapGenOptions.h"

class JsonSerializeFormat;

namespace ETemplateZoneType
{
	enum ETemplateZoneType
	{
		PLAYER_START,
		CPU_START,
		TREASURE,
		JUNCTION
	};
}

class DLL_LINKAGE CTreasureInfo
{
public:
	ui32 min;
	ui32 max;
	ui16 density;
};

namespace rmg
{

class DLL_LINKAGE ZoneConnection
{
public:
	ZoneConnection();

	TRmgTemplateZoneId getZoneA() const;
	void setZoneA(TRmgTemplateZoneId value);
	TRmgTemplateZoneId getZoneB() const;
	void setZoneB(TRmgTemplateZoneId value);
	int getGuardStrength() const; /// Default: 0
	void setGuardStrength(int value);

private:
	TRmgTemplateZoneId zoneA;
	TRmgTemplateZoneId zoneB;
	int guardStrength;
};

class DLL_LINKAGE ZoneOptions
{
public:
	static const std::set<ETerrainType> DEFAULT_TERRAIN_TYPES;

	class DLL_LINKAGE CTownInfo
	{
	public:
		CTownInfo();

		int getTownCount() const; /// Default: 0
		void setTownCount(int value);
		int getCastleCount() const; /// Default: 0
		void setCastleCount(int value);
		int getTownDensity() const; /// Default: 0
		void setTownDensity(int value);
		int getCastleDensity() const; /// Default: 0
		void setCastleDensity(int value);

	private:
		int townCount, castleCount, townDensity, castleDensity;
	};

	ZoneOptions();

	ZoneOptions & operator=(const ZoneOptions & other);

	TRmgTemplateZoneId getId() const;
	void setId(TRmgTemplateZoneId value);

	ETemplateZoneType::ETemplateZoneType getType() const; /// Default: ETemplateZoneType::PLAYER_START
	void setType(ETemplateZoneType::ETemplateZoneType value);

	int getSize() const; /// Default: 1
	void setSize(int value);

	boost::optional<int> getOwner() const;
	void setOwner(boost::optional<int> value);

	const CTownInfo & getPlayerTowns() const;
	void setPlayerTowns(const CTownInfo & value);
	const CTownInfo & getNeutralTowns() const;
	void setNeutralTowns(const CTownInfo & value);

	bool getMatchTerrainToTown() const; /// Default: true
	void setMatchTerrainToTown(bool value);

	const std::set<ETerrainType> & getTerrainTypes() const; /// Default: all
	void setTerrainTypes(const std::set<ETerrainType> & value);

	bool getTownsAreSameType() const; /// Default: false
	void setTownsAreSameType(bool value);

	std::set<TFaction> getDefaultTownTypes() const;

	const std::set<TFaction> & getTownTypes() const; /// Default: all
	void setTownTypes(const std::set<TFaction> & value);
	void setMonsterTypes(const std::set<TFaction> & value);

	void setMonsterStrength(EMonsterStrength::EMonsterStrength val);

	void setMinesAmount (TResource res, ui16 amount);
	std::map<TResource, ui16> getMinesInfo() const;

	void addTreasureInfo(const CTreasureInfo & info);
	const std::vector<CTreasureInfo> & getTreasureInfo() const;

	void addConnection(TRmgTemplateZoneId otherZone);

protected:
	TRmgTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int size;
	boost::optional<int> owner;
	CTownInfo playerTowns;
	CTownInfo neutralTowns;
	bool matchTerrainToTown;
	std::set<ETerrainType> terrainTypes;
	bool townsAreSameType;

	std::set<TFaction> townTypes;
	std::set<TFaction> monsterTypes;

	EMonsterStrength::EMonsterStrength zoneMonsterStrength;

	std::map<TResource, ui16> mines; //obligatory mines to spawn in this zone

	std::vector<CTreasureInfo> treasureInfo;

	std::vector<TRmgTemplateZoneId> connections; //list of adjacent zones
};

}

/// The CRmgTemplate describes a random map template.
class DLL_LINKAGE CRmgTemplate
{
public:
	class CSize
	{
	public:
		CSize();
		CSize(int width, int height, bool under);

		int getWidth() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
		void setWidth(int value);
		int getHeight() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
		void setHeight(int value);
		bool getUnder() const; /// Default: true
		void setUnder(bool value);
		bool operator<=(const CSize & value) const;
		bool operator>=(const CSize & value) const;

	private:
		int width, height;
		bool under;
	};

	class CPlayerCountRange
	{
	public:
		void addRange(int lower, int upper);
		void addNumber(int value);
		bool isInRange(int count) const;
		std::set<int> getNumbers() const;

	private:
		std::list<std::pair<int, int> > range;
	};

	CRmgTemplate();
	~CRmgTemplate();

	const std::string & getName() const;
	void setName(const std::string & value);
	const CSize & getMinSize() const;
	void setMinSize(const CSize & value);
	const CSize & getMaxSize() const;
	void setMaxSize(const CSize & value);
	const CPlayerCountRange & getPlayers() const;
	void setPlayers(const CPlayerCountRange & value);
	const CPlayerCountRange & getCpuPlayers() const;
	void setCpuPlayers(const CPlayerCountRange & value);
	const std::map<TRmgTemplateZoneId, rmg::ZoneOptions *> & getZones() const;
	void setZones(const std::map<TRmgTemplateZoneId, rmg::ZoneOptions *> & value);
	const std::list<rmg::ZoneConnection> & getConnections() const;
	void setConnections(const std::list<rmg::ZoneConnection> & value);

	void validate() const; /// Tests template on validity and throws exception on failure

private:
	std::string name;
	CSize minSize, maxSize;
	CPlayerCountRange players, cpuPlayers;
	std::map<TRmgTemplateZoneId, rmg::ZoneOptions *> zones;
	std::list<rmg::ZoneConnection> connections;
};
