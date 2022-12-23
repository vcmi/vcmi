/*
 * CMapGenOptions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRmgTemplate;
class CRandomGenerator;

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
			h & color;
			h & startingTown;
			h & playerType;
		}
	};

	CMapGenOptions();
	CMapGenOptions(const CMapGenOptions&) = delete;

	si32 getWidth() const;
	void setWidth(si32 value);

	si32 getHeight() const;
	void setHeight(si32 value);

	bool getHasTwoLevels() const;
	void setHasTwoLevels(bool value);

	/// The count of all (human or computer) players ranging from 1 to PlayerColor::PLAYER_LIMIT or RANDOM_SIZE for random. If you call
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

	std::vector<const CRmgTemplate *> getPossibleTemplates() const;

	/// Finalizes the options. All random sizes for various properties will be overwritten by numbers from
	/// a random number generator by keeping the options in a valid state. Check options should return true, otherwise
	/// this function fails.
	void finalize(CRandomGenerator & rand);

	/// Returns false if there is no template available which fits to the currently selected options.
	bool checkOptions() const;

	static const si8 RANDOM_SIZE = -1;

private:
	void resetPlayersMap();
	int countHumanPlayers() const;
	int countCompOnlyPlayers() const;
	PlayerColor getNextPlayerColor() const;
	void updateCompOnlyPlayers();
	void updatePlayers();
	const CRmgTemplate * getPossibleTemplate(CRandomGenerator & rand) const;

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
		h & width;
		h & height;
		h & hasTwoLevels;
		h & playerCount;
		h & teamCount;
		h & compOnlyPlayerCount;
		h & compOnlyTeamCount;
		h & waterContent;
		h & monsterStrength;
		h & players;
		//TODO add name of template to class, enables selection of a template by a user
	}
};

VCMI_LIB_NAMESPACE_END
