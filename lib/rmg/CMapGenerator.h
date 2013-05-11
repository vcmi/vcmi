
/*
 * CMapGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../CRandomGenerator.h"

class CMap;
class CTerrainViewPatternConfig;
class CMapEditManager;
class JsonNode;

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
	RANDOM = -1,
	WEAK,
	NORMAL,
	STRONG
};
}

namespace EPlayerType
{
enum EPlayerType
{
	HUMAN,
	AI,
	COMP_ONLY
};
}

/// The map gen options class holds values about general map generation settings
/// e.g. the size of the map, the count of players,...
class DLL_LINKAGE CMapGenOptions
{
public:
	/// The player settings class maps the player color, starting town and human player flag.
	class DLL_LINKAGE CPlayerSettings
	{
	public:
		CPlayerSettings();

		/// The color of the player ranging from 0 to PlayerColor::PLAYER_LIMIT - 1.
		/// The default value is 0.
		PlayerColor getColor() const;
		void setColor(PlayerColor value);

		/// The starting town of the player ranging from 0 to town max count or RANDOM_TOWN.
		/// The default value is RANDOM_TOWN.
		si32 getStartingTown() const;
		void setStartingTown(si32 value);

		/// The default value is EPlayerType::AI.
		EPlayerType::EPlayerType getPlayerType() const;
		void setPlayerType(EPlayerType::EPlayerType value);

		/// Constant for a random town selection.
		static const si32 RANDOM_TOWN = -1;

	private:
		PlayerColor color;
		si32 startingTown;
		EPlayerType::EPlayerType playerType;

	public:
		template <typename Handler>
		void serialize(Handler & h, const int version)
		{
			h & color & startingTown & playerType;
		}
	};

	CMapGenOptions();

	si32 getWidth() const;
	void setWidth(si32 value);

	si32 getHeight() const;
	void setHeight(si32 value);

	bool getHasTwoLevels() const;
	void setHasTwoLevels(bool value);

	/// The count of the players ranging from 1 to PlayerColor::PLAYER_LIMIT or RANDOM_SIZE for random. If you call
	/// this method, all player settings are reset to default settings.
	si8 getPlayersCnt() const;
	void setPlayersCnt(si8 value);

	/// The count of the teams ranging from 0 to <players count - 1> or RANDOM_SIZE for random.
	si8 getTeamsCnt() const;
	void setTeamsCnt(si8 value);

	/// The count of the computer only players ranging from 0 to <PlayerColor::PLAYER_LIMIT - players count> or RANDOM_SIZE for random.
	/// If you call this method, all player settings are reset to default settings.
	si8 getCompOnlyPlayersCnt() const;
	void setCompOnlyPlayersCnt(si8 value);

	/// The count of the computer only teams ranging from 0 to <comp only players - 1> or RANDOM_SIZE for random.
	si8 getCompOnlyTeamsCnt() const;
	void setCompOnlyTeamsCnt(si8 value);

	EWaterContent::EWaterContent getWaterContent() const;
	void setWaterContent(EWaterContent::EWaterContent value);

	EMonsterStrength::EMonsterStrength getMonsterStrength() const;
	void setMonsterStrength(EMonsterStrength::EMonsterStrength value);

	/// The first player colors belong to standard players and the last player colors belong to comp only players.
	/// All standard players are by default of type EPlayerType::AI.
	const std::map<PlayerColor, CPlayerSettings> & getPlayersSettings() const;
	void setStartingTownForPlayer(PlayerColor color, si32 town);
	/// Sets a player type for a standard player. A standard player is the opposite of a computer only player. The
	/// values which can be chosen for the player type are EPlayerType::AI or EPlayerType::HUMAN. Calling this method
	/// has no effect for the map itself, but it adds some informational text for the map description.
	void setPlayerTypeForStandardPlayer(PlayerColor color, EPlayerType::EPlayerType playerType);

	/// Finalizes the options. All random sizes for various properties will be overwritten by numbers from
	/// a random number generator by keeping the options in a valid state.
	void finalize();
	void finalize(CRandomGenerator & gen);

	static const si8 RANDOM_SIZE = -1;

private:
	void resetPlayersMap();
	int countHumanPlayers() const;
	PlayerColor getNextPlayerColor() const;

	si32 width, height;
	bool hasTwoLevels;
	si8 playersCnt, teamsCnt, compOnlyPlayersCnt, compOnlyTeamsCnt;
	EWaterContent::EWaterContent waterContent;
	EMonsterStrength::EMonsterStrength monsterStrength;
	std::map<PlayerColor, CPlayerSettings> players;

public:
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		//FIXME: Enum is not a fixed with data type. Add enum class to both enums
		// later. For now it is ok.
		h & width & height & hasTwoLevels & playersCnt & teamsCnt & compOnlyPlayersCnt;
		h & compOnlyTeamsCnt & waterContent & monsterStrength & players;
	}
};

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator
{
public:
	explicit CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed = std::time(nullptr));
	~CMapGenerator(); // required due to unique_ptr

	std::unique_ptr<CMap> generate();

private:
	/// Generation methods
	std::string getMapDescription() const;
	void addPlayerInfo();
	void addHeaderInfo();
	void genTerrain();
	void genTowns();

	CMapGenOptions mapGenOptions;
	std::unique_ptr<CMap> map;
	CRandomGenerator gen;
	int randomSeed;
	CMapEditManager * editManager;
};

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

namespace ETemplateZoneType
{
enum ETemplateZoneType
{
	HUMAN_START,
	COMPUTER_START,
	TREASURE,
	JUNCTION
};
}

/// The CTemplateZoneTowns holds info about towns in a template zone.
class DLL_LINKAGE CTemplateZoneTowns
{
public:
	CTemplateZoneTowns();

	int getMinTowns() const; /// Default: 0
	void setMinTowns(int value);
	int getMinCastles() const; /// Default: 0
	void setMinCastles(int value);
	int getTownDensity() const; /// Default: 0
	void setTownDensity(int value);
	int getCastleDensity() const; /// Default: 0
	void setCastleDensity(int value);

private:
	int minTowns, minCastles, townDensity, castleDensity;
};

typedef int TTemplateZoneId;

/// The CTemplateZone describes a zone in a template.
class DLL_LINKAGE CTemplateZone
{
public:
	CTemplateZone();

	TTemplateZoneId getId() const; /// Default: 0 = not set;
	void setId(TTemplateZoneId value);
	ETemplateZoneType::ETemplateZoneType getType() const; /// Default: ETemplateZoneType::HUMAN_START
	void setType(ETemplateZoneType::ETemplateZoneType value);
	int getBaseSize() const; /// Default: 0 = not set;
	void setBaseSize(int value);
	int getOwner() const; /// Default: 0 = not set;
	void setOwner(int value);
	const CTemplateZoneTowns & getPlayerTowns() const;
	void setPlayerTowns(const CTemplateZoneTowns & value);
	const CTemplateZoneTowns & getNeutralTowns() const;
	void setNeutralTowns(const CTemplateZoneTowns & value);
	bool getNeutralTownsAreSameType() const; /// Default: false
	void setNeutralTownsAreSameType(bool value);
	const std::set<TFaction> & getAllowedTownTypes() const;
	void setAllowedTownTypes(const std::set<TFaction> & value);
	bool getMatchTerrainToTown() const; /// Default: false
	void setMatchTerrainToTown(bool value);
	const std::set<ETerrainType> & getTerrainTypes() const;
	void setTerrainTypes(const std::set<ETerrainType> & value);

private:
	TTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int baseSize;
	int owner;
	CTemplateZoneTowns playerTowns, neutralTowns;
	bool neutralTownsAreSameType;
	std::set<TFaction> allowedTownTypes;
	bool matchTerrainToTown;
	std::set<ETerrainType> terrainTypes;
};

/// The CTemplateZoneConnection describes the connection between two zones.
class DLL_LINKAGE CTemplateZoneConnection
{
public:
	CTemplateZoneConnection();

	TTemplateZoneId getZoneA() const; /// Default: 0 = not set;
	void setZoneA(TTemplateZoneId value);
	TTemplateZoneId getZoneB() const; /// Default: 0 = not set;
	void setZoneB(TTemplateZoneId value);
	int getGuardStrength() const; /// Default: 0
	void setGuardStrength(int value);

private:
	TTemplateZoneId zoneA, zoneB;
	int guardStrength;
};

/// The CRandomMapTemplateSize describes the dimensions of the template.
class CRandomMapTemplateSize
{
public:
	CRandomMapTemplateSize();
	CRandomMapTemplateSize(int width, int height, bool under);

	int getWidth() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
	void setWidth(int value);
	int getHeight() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
	void setHeight(int value);
	bool getUnder() const; /// Default: true
	void setUnder(bool value);

private:
	int width, height;
	bool under;
};

/// The CRandomMapTemplate describes a random map template.
class DLL_LINKAGE CRandomMapTemplate
{
public:
	CRandomMapTemplate();

	const std::string & getName() const;
	void setName(const std::string & value);
	const CRandomMapTemplateSize & getMinSize() const; /// Default: CMapHeader::MAP_SIZE_SMALL x CMapHeader::MAP_SIZE_SMALL x wo under
	void setMinSize(const CRandomMapTemplateSize & value);
	const CRandomMapTemplateSize & getMaxSize() const; /// Default: CMapHeader::MAP_SIZE_XLARGE x CMapHeader::MAP_SIZE_XLARGE x under
	void setMaxSize(const CRandomMapTemplateSize & value);
	int getMinHumanCnt() const; /// Default: 1
	void setMinHumanCnt(int value);
	int getMaxHumanCnt() const; /// Default: PlayerColor::PLAYER_LIMIT_I
	void setMaxHumanCnt(int value);
	int getMinTotalCnt() const; /// Default: 2
	void setMinTotalCnt(int value);
	int getMaxTotalCnt() const; /// Default: PlayerColor::PLAYER_LIMIT_I
	void setMaxTotalCnt(int value);
	const std::map<TTemplateZoneId, CTemplateZone> & getZones() const;
	void setZones(const std::map<TTemplateZoneId, CTemplateZone> & value);
	const std::list<CTemplateZoneConnection> & getConnections() const;
	void setConnections(const std::list<CTemplateZoneConnection> & value);

private:
	std::string name;
	CRandomMapTemplateSize minSize, maxSize;
	int minHumanCnt, maxHumanCnt, minTotalCnt, maxTotalCnt;
	std::map<TTemplateZoneId, CTemplateZone> zones;
	std::list<CTemplateZoneConnection> connections;
};

/// The CRmTemplateLoader is a abstract base class for loading templates.
class DLL_LINKAGE CRmTemplateLoader
{
public:
	virtual ~CRmTemplateLoader() { };
	virtual void loadTemplates() = 0;
	const std::map<std::string, CRandomMapTemplate> & getTemplates() const;

protected:
	std::map<std::string, CRandomMapTemplate> templates;
};

/// The CJsonRmTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CJsonRmTemplateLoader : public CRmTemplateLoader
{
public:
	void loadTemplates() override;

private:
	CRandomMapTemplateSize parseMapTemplateSize(const std::string & text) const;
	CTemplateZoneTowns parseTemplateZoneTowns(const JsonNode & node) const;
	ETemplateZoneType::ETemplateZoneType getZoneType(const std::string & type) const;
	std::set<TFaction> getFactions(const std::vector<JsonNode> factionStrings) const;
	std::set<ETerrainType> parseTerrainTypes(const std::vector<JsonNode> terTypeStrings) const;
};

/// The CRandomMapTemplateStorage is a singleton object where templates are stored and which can be accessed from anywhere.
class DLL_LINKAGE CRandomMapTemplateStorage
{
public:
	static CRandomMapTemplateStorage & get();

	const std::map<std::string, CRandomMapTemplate> & getTemplates() const;

private:
	CRandomMapTemplateStorage();
	~CRandomMapTemplateStorage();

	static boost::mutex smx;
	std::map<std::string, CRandomMapTemplate> templates; /// Key: Template name
};
