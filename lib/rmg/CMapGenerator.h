
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
	class CPlayerSettings
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
	CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed);
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
	std::unique_ptr<CMapEditManager> mapMgr;
};
