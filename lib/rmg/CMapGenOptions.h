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
#include "../serializer/Serializeable.h"
#include "CRmgTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

enum class EPlayerType
{
	HUMAN,
	AI,
	COMP_ONLY
};

/// The map gen options class holds values about general map generation settings
/// e.g. the size of the map, the count of players,...
class DLL_LINKAGE CMapGenOptions : public Serializeable
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
		void setColor(const PlayerColor & value);

		/// The starting town of the player ranging from 0 to town max count or RANDOM_TOWN.
		/// The default value is RANDOM_TOWN.
		FactionID getStartingTown() const;
		void setStartingTown(FactionID value);

		/// The starting hero of the player ranging from 0 to hero max count or RANDOM_HERO.
		/// The default value is RANDOM_HERO
		HeroTypeID getStartingHero() const;
		void setStartingHero(HeroTypeID value);

		/// The default value is EPlayerType::AI.
		EPlayerType getPlayerType() const;
		void setPlayerType(EPlayerType value);
		
		/// Team id for this player. TeamID::NO_TEAM by default - team will be randomly assigned
		TeamID getTeam() const;
		void setTeam(const TeamID & value);

	private:
		PlayerColor color;
		FactionID startingTown;
		HeroTypeID startingHero;
		EPlayerType playerType;
		TeamID team;

	public:
		template <typename Handler>
		void serialize(Handler & h)
		{
			h & color;
			h & startingTown;
			h & playerType;
			h & team;
			h & startingHero;
		}
	};

	CMapGenOptions();
	CMapGenOptions(const CMapGenOptions&) = delete;

	si32 getWidth() const;
	void setWidth(si32 value);

	si32 getHeight() const;
	void setHeight(si32 value);

	int getLevels() const;
	void setLevels(int value);

	/// The count of all (human or computer) players ranging from 1 to PlayerColor::PLAYER_LIMIT or RANDOM_SIZE for random. If you call
	/// this method, all player settings are reset to default settings.
	si8 getHumanOrCpuPlayerCount() const;
	void setHumanOrCpuPlayerCount(si8 value);

	si8 getMinPlayersCount(bool withTemplateLimit = true) const;
	si8 getMaxPlayersCount(bool withTemplateLimit = true) const;
	si8 getPlayerLimit() const;

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
	
	bool isRoadEnabled(const RoadId & roadType) const;
	bool isRoadEnabled() const;
	void setRoadEnabled(const RoadId & roadType, bool enable);

	/// The first player colors belong to standard players and the last player colors belong to comp only players.
	/// All standard players are by default of type EPlayerType::AI.
	const std::map<PlayerColor, CPlayerSettings> & getPlayersSettings() const;
	const std::map<PlayerColor, CPlayerSettings> & getSavedPlayersMap() const;
	void setStartingTownForPlayer(const PlayerColor & color, FactionID town);
	void setStartingHeroForPlayer(const PlayerColor & color, HeroTypeID hero);
	/// Sets a player type for a standard player. A standard player is the opposite of a computer only player. The
	/// values which can be chosen for the player type are EPlayerType::AI or EPlayerType::HUMAN.
	void setPlayerTypeForStandardPlayer(const PlayerColor & color, EPlayerType playerType);

	void setPlayerTeam(const PlayerColor & color, const TeamID & team = TeamID::NO_TEAM);

	/// The random map template to generate the map with or empty/not set if the template should be chosen randomly.
	/// Default: Not set/random.
	const CRmgTemplate * getMapTemplate() const;
	void setMapTemplate(const CRmgTemplate * value);
	void setMapTemplate(const std::string & name);

	std::vector<const CRmgTemplate *> getPossibleTemplates() const;

	/// Finalizes the options. All random sizes for various properties will be overwritten by numbers from
	/// a random number generator by keeping the options in a valid state. Check options should return true, otherwise
	/// this function fails.
	void finalize(vstd::RNG & rand);

	/// Returns false if there is no template available which fits to the currently selected options.
	bool checkOptions() const;
	/// Returns true if player colors or teams were set in game GUI
	bool arePlayersCustomized() const;

	static const si8 RANDOM_SIZE = -1;

private:
	void initPlayersMap();
	void resetPlayersMap();
	void savePlayersMap();
	int countHumanPlayers() const;
	int countCompOnlyPlayers() const;
	PlayerColor getNextPlayerColor() const;
	void updateCompOnlyPlayers();
	void updatePlayers();
	const CRmgTemplate * getPossibleTemplate(vstd::RNG & rand) const;

	si32 width;
	si32 height;
	si32 levels;
	si8 humanOrCpuPlayerCount;
	si8 teamCount;
	si8 compOnlyPlayerCount;
	si8 compOnlyTeamCount;
	EWaterContent::EWaterContent waterContent;
	EMonsterStrength::EMonsterStrength monsterStrength;
	std::map<PlayerColor, CPlayerSettings> players;
	std::map<PlayerColor, CPlayerSettings> savedPlayerSettings;
	std::set<RoadId> enabledRoads;
	bool customizedPlayers;
	
	const CRmgTemplate * mapTemplate;

public:
	template <typename Handler>
	void serialize(Handler & h)
	{
		h & width;
		h & height;
		if (h.version >= Handler::Version::MORE_MAP_LAYERS)
			h & levels;
		else
		{
			if (h.saving)
			{
				bool hasTwoLevels = levels == 2;
				h & hasTwoLevels;
			}
			else
			{
				bool hasTwoLevels;
				h & hasTwoLevels;
				levels = hasTwoLevels ? 2 : 1;
			}
		}
		h & humanOrCpuPlayerCount;
		h & teamCount;
		h & compOnlyPlayerCount;
		h & compOnlyTeamCount;
		h & waterContent;
		h & monsterStrength;
		h & players;
		std::string templateName;
		if(mapTemplate && h.saving)
		{
			templateName = mapTemplate->getId();
		}
		h & templateName;
		if(!h.saving)
		{
			setMapTemplate(templateName);
		}

		h & enabledRoads;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

VCMI_LIB_NAMESPACE_END
