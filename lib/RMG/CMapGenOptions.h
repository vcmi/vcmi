
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

/**
 * The map gen options class holds values about general map
 * generation settings e.g. the size of the map, the count of players,...
 */
class DLL_LINKAGE CMapGenOptions
{
public:
	/**
	 * C-tor.
	 */
	CMapGenOptions();

	/**
	 * Gets the width of the map.
	 *
	 * @return width of the map in tiles, default is 72
	 */
	int getWidth() const;

	/**
	 * Sets the width of the map.
	 *
	 * @param value the width of the map in tiles, any values higher than 0 are allowed
	 */
	void setWidth(int value);

	/**
	 * Gets the height of the map.
	 *
	 * @return height of the map in tiles, default is 72
	 */
	int getHeight() const;

	/**
	 * Sets the height of the map.
	 *
	 * @param value the height of the map in tiles, any values higher than 0 are allowed
	 */
	void setHeight(int value);

	/**
	 * Gets the flag whether the map should be generated with two levels.
	 *
	 * @return true for two level map, default is true
	 */
	bool getHasTwoLevels() const;

	/**
	 * Sets the flag whether the map should be generated with two levels.
	 *
	 * @param value true for two level map
	 */
	void setHasTwoLevels(bool value);

	/**
	 * Gets the count of the players.
	 *
	 * @return the count of the players ranging from 1 to 8, -1 for random, default is -1
	 */
	int getPlayersCnt() const;

	/**
	 * Sets the count of the players.
	 *
	 * @param value the count of the players ranging from 1 to 8, -1 for random
	 */
	void setPlayersCnt(int value);

	/**
	 * Gets the count of the teams.
	 *
	 * @return the count of the teams ranging from 0 to <players count - 1>, -1 for random, default is -1
	 */
	int getTeamsCnt() const;

	/**
	 * Sets the count of the teams
	 *
	 * @param value the count of the teams ranging from 0 to <players count - 1>, -1 for random
	 */
	void setTeamsCnt(int value);

	/**
	 * Gets the count of the computer only players.
	 *
	 * @return the count of the computer only players ranging from 0 to <8 - players count>, -1 for random, default is -1
	 */
	int getCompOnlyPlayersCnt() const;

	/**
	 * Sets the count of the computer only players.
	 *
	 * @param value the count of the computer only players ranging from 0 to <8 - players count>, -1 for random
	 */
	void setCompOnlyPlayersCnt(int value);

	/**
	 * Gets the count of the computer only teams.
	 *
	 * @return the count of the computer only teams ranging from 0 to <comp only players - 1>, -1 for random, default is -1
	 */
	int getCompOnlyTeamsCnt() const;

	/**
	 * Sets the count of the computer only teams.
	 *
	 * @param value the count of the computer only teams ranging from 0 to <comp only players - 1>, -1 for random
	 */
	void setCompOnlyTeamsCnt(int value);

	/**
	 * Gets the water content.
	 *
	 * @return the water content, default is normal
	 */
	EWaterContent::EWaterContent getWaterContent() const;

	/**
	 * Sets the water content.
	 *
	 * @param value the water content
	 */
	void setWaterContent(EWaterContent::EWaterContent value);

	/**
	 * Gets the strength of the monsters.
	 *
	 * @return the strenght of the monsters, default is normal
	 */
	EMonsterStrength::EMonsterStrength getMonsterStrength() const;

	/**
	 * Sets the strength of the monsters.
	 *
	 * @param value the strenght of the monsters
	 */
	void setMonsterStrength(EMonsterStrength::EMonsterStrength value);

private:
	/** the width of the map in tiles */
	int width;

	/** the height of the map in tiles */
	int height;

	/** true if the map has two levels/underground */
	bool hasTwoLevels;

	/** the count of the players(human + computer); -1 if random */
	int playersCnt;

	/** the count of the teams; -1 if random */
	int teamsCnt;

	/** the count of computer only players; -1 if random */
	int compOnlyPlayersCnt;

	/** the count of computer only teams; -1 if random */
	int compOnlyTeamsCnt;

	/** the water content, -1 if random */
	EWaterContent::EWaterContent waterContent;

	/** the strength of the monsters, -1 if random */
	EMonsterStrength::EMonsterStrength monsterStrength;
};
