
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
	 * Gets the width of the map. The default value is 72.
	 *
	 * @return width of the map in tiles
	 */
	si32 getWidth() const;

	/**
	 * Sets the width of the map.
	 *
	 * @param value the width of the map in tiles, any values higher than 0 are allowed
	 */
	void setWidth(si32 value);

	/**
	 * Gets the height of the map. The default value is 72.
	 *
	 * @return height of the map in tiles
	 */
	si32 getHeight() const;

	/**
	 * Sets the height of the map.
	 *
	 * @param value the height of the map in tiles, any values higher than 0 are allowed
	 */
	void setHeight(si32 value);

	/**
	 * Gets the flag whether the map should be generated with two levels. The
	 * default value is true.
	 *
	 * @return true for two level map
	 */
	bool getHasTwoLevels() const;

	/**
	 * Sets the flag whether the map should be generated with two levels.
	 *
	 * @param value true for two level map
	 */
	void setHasTwoLevels(bool value);

	/**
	 * Gets the count of the players. The default value is RANDOM_SIZE representing a random
	 * player count.
	 *
	 * @return the count of the players ranging from 1 to PlayerColor::PLAYER_LIMIT or RANDOM_SIZE for random
	 */
	si8 getPlayersCnt() const;

	/**
	 * Sets the count of the players.
	 *
	 * @param value the count of the players ranging from 1 to PlayerColor::PLAYER_LIMIT, RANDOM_SIZE for random
	 */
	void setPlayersCnt(si8 value);

	/**
	 * Gets the count of the teams. The default value is RANDOM_SIZE representing a random
	 * team count.
	 *
	 * @return the count of the teams ranging from 0 to <players count - 1> or RANDOM_SIZE for random
	 */
	si8 getTeamsCnt() const;

	/**
	 * Sets the count of the teams
	 *
	 * @param value the count of the teams ranging from 0 to <players count - 1>, RANDOM_SIZE for random
	 */
	void setTeamsCnt(si8 value);

	/**
	 * Gets the count of the computer only players. The default value is 0.
	 *
	 * @return the count of the computer only players ranging from 0 to <PlayerColor::PLAYER_LIMIT - players count> or RANDOM_SIZE for random
	 */
	si8 getCompOnlyPlayersCnt() const;

	/**
	 * Sets the count of the computer only players.
	 *
	 * @param value the count of the computer only players ranging from 0 to <PlayerColor::PLAYER_LIMIT - players count>, RANDOM_SIZE for random
	 */
	void setCompOnlyPlayersCnt(si8 value);

	/**
	 * Gets the count of the computer only teams. The default value is RANDOM_SIZE representing
	 * a random computer only team count.
	 *
	 * @return the count of the computer only teams ranging from 0 to <comp only players - 1> or RANDOM_SIZE for random
	 */
	si8 getCompOnlyTeamsCnt() const;

	/**
	 * Sets the count of the computer only teams.
	 *
	 * @param value the count of the computer only teams ranging from 0 to <comp only players - 1>, RANDOM_SIZE for random
	 */
	void setCompOnlyTeamsCnt(si8 value);

	/**
	 * Gets the water content. The default value is random.
	 *
	 * @return the water content
	 */
	EWaterContent::EWaterContent getWaterContent() const;

	/**
	 * Sets the water content.
	 *
	 * @param value the water content
	 */
	void setWaterContent(EWaterContent::EWaterContent value);

	/**
	 * Gets the strength of the monsters. The default value is random.
	 *
	 * @return the strenght of the monsters
	 */
	EMonsterStrength::EMonsterStrength getMonsterStrength() const;

	/**
	 * Sets the strength of the monsters.
	 *
	 * @param value the strenght of the monsters
	 */
	void setMonsterStrength(EMonsterStrength::EMonsterStrength value);

	/** The constant for specifying a random number of sth. */
	static const si8 RANDOM_SIZE = -1;

private:
	/** The width of the map in tiles. */
	si32 width;

	/** The height of the map in tiles. */
	si32 height;

	/** True if the map has two levels that means an underground. */
	bool hasTwoLevels;

	/** The count of the players(human + computer). */
	si8 playersCnt;

	/** The count of the teams. */
	si8 teamsCnt;

	/** The count of computer only players. */
	si8 compOnlyPlayersCnt;

	/** The count of computer only teams. */
	si8 compOnlyTeamsCnt;

	/** The amount of water content. */
	EWaterContent::EWaterContent waterContent;

	/** The strength of the monsters. */
	EMonsterStrength::EMonsterStrength monsterStrength;

public:
	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		//FIXME: Enum is not a fixed with data type. Add enum class to both enums
		// later. For now it is ok.
		h & width & height & hasTwoLevels & playersCnt & teamsCnt & compOnlyPlayersCnt;
		h & compOnlyTeamsCnt & waterContent & monsterStrength;
	}
};
