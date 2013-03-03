
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
#include "CMapGenOptions.h"
#include "../CRandomGenerator.h"

class CMap;
class CTerrainViewPatternConfig;
class CMapEditManager;

/**
 * The map generator creates a map randomly.
 */
class CMapGenerator
{
public:
	/**
	 * The player settings class maps the player color, starting town and human player flag.
	 */
	class CPlayerSettings
	{
	public:
		enum EPlayerType
		{
			HUMAN,
			AI,
			COMP_ONLY
		};

		/**
		 * Constructor.
		 */
		CPlayerSettings();

		/**
		 * Gets the color of the player. The default value is 0.
		 *
		 * @return the color of the player ranging from 0 to PlayerColor::PLAYER_LIMIT - 1
		 */
		PlayerColor getColor() const;

		/**
		 * Sets the color of the player.
		 *
		 * @param value the color of the player ranging from 0 to PlayerColor::PLAYER_LIMIT - 1
		 */
		void setColor(PlayerColor value);

		/**
		 * Gets the starting town of the player. The default value is RANDOM_TOWN.
		 *
		 * @return the starting town of the player ranging from 0 to town max count or RANDOM_TOWN
		 */
		int getStartingTown() const;

		/**
		 * Sets the starting town of the player.
		 *
		 * @param value the starting town of the player ranging from 0 to town max count or RANDOM_TOWN
		 */
		void setStartingTown(int value);

		/**
		 * Gets the type of the player. The default value is EPlayerType::AI.
		 *
		 * @return the type of the player
		 */
		EPlayerType getPlayerType() const;

		/**
		 * Sets the type of the player.
		 *
		 * @param playerType the type of the player
		 */
		void setPlayerType(EPlayerType value);

		/** Constant for a random town selection. */
		static const int RANDOM_TOWN = -1;

	private:
		/** The color of the player. */
		PlayerColor color;

		/** The starting town of the player. */
		int startingTown;

		/** The type of the player e.g. human, comp only,... */
		EPlayerType playerType;
	};

	/**
	 * Constructor.
	 *
	 * @param mapGenOptions these options describe how to generate the map.
	 * @param players the random gen player settings
	 * @param randomSeed a random seed is required to get random numbers.
	 */
	CMapGenerator(const CMapGenOptions & mapGenOptions, const std::map<PlayerColor, CPlayerSettings> & players, int randomSeed);

	/**
	 * Destructor.
	 */
	~CMapGenerator();

	/**
	 * Generates a map.
	 *
	 * @return the generated map object stored in a unique ptr
	 */
	std::unique_ptr<CMap> generate();

private:
	/**
	 * Validates map gen options and players options. On errors exceptions will be thrown.
	 */
	void validateOptions() const;

	/**
	 * Finalizes map generation options. Random sizes for various properties are
	 * converted to fixed values.
	 */
	void finalizeMapGenOptions();

	/**
	 * Gets the map description of the generated map.
	 *
	 * @return the map description of the generated map
	 */
	std::string getMapDescription() const;

	/**
	 * Adds player information.(teams, colors, etc...)
	 */
	void addPlayerInfo();

	/**
	 * Counts the amount of human players.
	 *
	 * @return the amount of human players ranging from 0 to PlayerColor::PLAYER_LIMIT
	 */
	int countHumanPlayers() const;

	/**
	 * Generate terrain.
	 */
	void genTerrain();

	/**
	 * Generate towns.
	 */
	void genTowns();

	/**
	 * Adds header info(size, description, etc...)
	 */
	void addHeaderInfo();

	/**
	 * Gets the next free player color.
	 *
	 * @return the next free player color
	 */
	PlayerColor getNextPlayerColor() const;

	/** The map options which describes the size of the map and contain player info. */
	CMapGenOptions mapGenOptions;

	/** The generated map. */
	std::unique_ptr<CMap> map;

	/** The random number generator. */
	CRandomGenerator gen;

	/** The random seed, it is used for the map description. */
	int randomSeed;

	/** The terrain view pattern config. */
	std::unique_ptr<CTerrainViewPatternConfig> terViewPatternConfig;

	/** The map edit manager. */
	std::unique_ptr<CMapEditManager> mapMgr;

	/** The random gen player settings. */
	std::map<PlayerColor, CPlayerSettings> players;
};
