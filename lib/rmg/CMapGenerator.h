
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

typedef std::vector<JsonNode> JsonVector;

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

typedef int TRmgTemplateZoneId;

/// The CRmgTemplateZone describes a zone in a template.
class DLL_LINKAGE CRmgTemplateZone
{
public:
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

	CRmgTemplateZone();

	TRmgTemplateZoneId getId() const; /// Default: 0
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
	bool getTownsAreSameType() const; /// Default: false
	void setTownsAreSameType(bool value);
	const std::set<TFaction> & getTownTypes() const; /// Default: all
	void setTownTypes(const std::set<TFaction> & value);
	std::set<TFaction> getDefaultTownTypes() const;
	bool getMatchTerrainToTown() const; /// Default: true
	void setMatchTerrainToTown(bool value);
	const std::set<ETerrainType> & getTerrainTypes() const; /// Default: all
	void setTerrainTypes(const std::set<ETerrainType> & value);
	std::set<ETerrainType> getDefaultTerrainTypes() const;
	boost::optional<TRmgTemplateZoneId> getTerrainTypeLikeZone() const;
	void setTerrainTypeLikeZone(boost::optional<TRmgTemplateZoneId> value);
	boost::optional<TRmgTemplateZoneId> getTownTypeLikeZone() const;
	void setTownTypeLikeZone(boost::optional<TRmgTemplateZoneId> value);

private:
	TRmgTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int size;
	boost::optional<int> owner;
	CTownInfo playerTowns, neutralTowns;
	bool townsAreSameType;
	std::set<TFaction> townTypes;
	bool matchTerrainToTown;
	std::set<ETerrainType> terrainTypes;
	boost::optional<TRmgTemplateZoneId> terrainTypeLikeZone, townTypeLikeZone;
};

/// The CRmgTemplateZoneConnection describes the connection between two zones.
class DLL_LINKAGE CRmgTemplateZoneConnection
{
public:
	CRmgTemplateZoneConnection();

	TRmgTemplateZoneId getZoneA() const; /// Default: 0
	void setZoneA(TRmgTemplateZoneId value);
	TRmgTemplateZoneId getZoneB() const; /// Default: 0
	void setZoneB(TRmgTemplateZoneId value);
	int getGuardStrength() const; /// Default: 0
	void setGuardStrength(int value);

private:
	TRmgTemplateZoneId zoneA, zoneB;
	int guardStrength;
};

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
	const std::map<TRmgTemplateZoneId, CRmgTemplateZone> & getZones() const;
	void setZones(const std::map<TRmgTemplateZoneId, CRmgTemplateZone> & value);
	const std::list<CRmgTemplateZoneConnection> & getConnections() const;
	void setConnections(const std::list<CRmgTemplateZoneConnection> & value);

	void validate() const; /// Tests template on validity and throws exception on failure

private:
	std::string name;
	CSize minSize, maxSize;
	CPlayerCountRange players, cpuPlayers;
	std::map<TRmgTemplateZoneId, CRmgTemplateZone> zones;
	std::list<CRmgTemplateZoneConnection> connections;
};

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
	si8 getPlayerCount() const;
	void setPlayerCount(si8 value);

	/// The count of the teams ranging from 0 to <players count - 1> or RANDOM_SIZE for random.
	si8 getTeamCount() const;
	void setTeamCount(si8 value);

	/// The count of the computer only players ranging from 0 to <PlayerColor::PLAYER_LIMIT - players count> or RANDOM_SIZE for random.
	/// If you call this method, all player settings are reset to default settings.
	si8 getCompOnlyPlayerCount() const;
	void setCompOnlyPlayerCount(si8 value);

	/// The count of the computer only teams ranging from 0 to <comp only players - 1> or RANDOM_SIZE for random.
	si8 getCompOnlyTeamCount() const;
	void setCompOnlyTeamCount(si8 value);

	EWaterContent::EWaterContent getWaterContent() const;
	void setWaterContent(EWaterContent::EWaterContent value);

	EMonsterStrength::EMonsterStrength getMonsterStrength() const;
	void setMonsterStrength(EMonsterStrength::EMonsterStrength value);

	/// The first player colors belong to standard players and the last player colors belong to comp only players.
	/// All standard players are by default of type EPlayerType::AI.
	const std::map<PlayerColor, CPlayerSettings> & getPlayersSettings() const;
	void setStartingTownForPlayer(PlayerColor color, si32 town);
	/// Sets a player type for a standard player. A standard player is the opposite of a computer only player. The
	/// values which can be chosen for the player type are EPlayerType::AI or EPlayerType::HUMAN.
	void setPlayerTypeForStandardPlayer(PlayerColor color, EPlayerType::EPlayerType playerType);

	/// The random map template to generate the map with or empty/not set if the template should be chosen randomly.
	/// Default: Not set/random.
	const CRmgTemplate * getMapTemplate() const;
	void setMapTemplate(const CRmgTemplate * value);

	const std::map<std::string, CRmgTemplate> & getAvailableTemplates() const;

	/// Finalizes the options. All random sizes for various properties will be overwritten by numbers from
	/// a random number generator by keeping the options in a valid state. Check options should return true, otherwise
	/// this function fails.
	void finalize();
	void finalize(CRandomGenerator & gen);

	/// Returns false if there is no template available which fits to the currently selected options.
	bool checkOptions() const;

	static const si8 RANDOM_SIZE = -1;

private:
	void resetPlayersMap();
	int countHumanPlayers() const;
	PlayerColor getNextPlayerColor() const;
	void updateCompOnlyPlayers();
	void updatePlayers();
	const CRmgTemplate * getPossibleTemplate(CRandomGenerator & gen) const;

	si32 width, height;
	bool hasTwoLevels;
	si8 playerCount, teamCount, compOnlyPlayerCount, compOnlyTeamCount;
	EWaterContent::EWaterContent waterContent;
	EMonsterStrength::EMonsterStrength monsterStrength;
	std::map<PlayerColor, CPlayerSettings> players;
	const CRmgTemplate * mapTemplate;

public:
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & width & height & hasTwoLevels & playerCount & teamCount & compOnlyPlayerCount;
		h & compOnlyTeamCount & waterContent & monsterStrength & players;
        //TODO add name of template to class, enables selection of a template by a user
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

/// The CRmgTemplateLoader is a abstract base class for loading templates.
class DLL_LINKAGE CRmgTemplateLoader
{
public:
	virtual ~CRmgTemplateLoader() { };
	virtual void loadTemplates() = 0;
	const std::map<std::string, CRmgTemplate> & getTemplates() const;

protected:
	std::map<std::string, CRmgTemplate> templates;
};

/// The CJsonRmgTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CJsonRmgTemplateLoader : public CRmgTemplateLoader
{
public:
	void loadTemplates() override;

private:
	CRmgTemplate::CSize parseMapTemplateSize(const std::string & text) const;
	CRmgTemplateZone::CTownInfo parseTemplateZoneTowns(const JsonNode & node) const;
	ETemplateZoneType::ETemplateZoneType parseZoneType(const std::string & type) const;
	std::set<TFaction> parseTownTypes(const JsonVector & townTypesVector, const std::set<TFaction> & defaultTownTypes) const;
	std::set<ETerrainType> parseTerrainTypes(const JsonVector & terTypeStrings, const std::set<ETerrainType> & defaultTerrainTypes) const;
	CRmgTemplate::CPlayerCountRange parsePlayers(const std::string & players) const;
};

/// The CRmgTemplateStorage is a singleton object where templates are stored and which can be accessed from anywhere.
class DLL_LINKAGE CRmgTemplateStorage
{
public:
	static CRmgTemplateStorage & get();

	const std::map<std::string, CRmgTemplate> & getTemplates() const;

private:
	CRmgTemplateStorage();
	~CRmgTemplateStorage();

	static boost::mutex smx;
	std::map<std::string, CRmgTemplate> templates; /// Key: Template name
};
