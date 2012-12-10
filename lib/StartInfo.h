#pragma once

#include "GameConstants.h"

/*
 * StartInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "RMG/CMapGenOptions.h"

class CCampaignState;

/// Struct which describes the name, the color, the starting bonus of a player
struct PlayerSettings
{
	enum { PLAYER_AI = 0 }; // for use in playerID

	enum Ebonus {
		NONE     = -2,
		RANDOM   = -1,
		ARTIFACT =  0,
		GOLD     =  1,
		RESOURCE =  2
	};

	//uses enum type Ebonus
	si8 bonus;
	si16 castle;
	si32 hero,
	     heroPortrait; //-1 if default, else ID

	std::string heroName;
	TPlayerColor color; //from 0 - 
	ui8 handicap;//0-no, 1-mild, 2-severe
	ui8 team;

	std::string name;
	ui8 playerID; //0 - AI, non-0 serves as player id
	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & castle;
		h & hero;
		h & heroPortrait;
		h & heroName;
		h & bonus;
		h & color;
		h & handicap;
		h & name;
		h & playerID;
		h & team;
	}

	PlayerSettings()
	{
		bonus = RANDOM;
		castle = NONE;
		heroPortrait = RANDOM;
	}
};

/// Struct which describes the difficulty, the turn time,.. of a heroes match.
struct StartInfo
{
	enum EMode {NEW_GAME, LOAD_GAME, CAMPAIGN, DUEL, INVALID = 255};

	ui8 mode; //uses EMode enum
	ui8 difficulty; //0=easy; 4=impossible

	typedef bmap<TPlayerColor, PlayerSettings> TPlayerInfos;
	TPlayerInfos playerInfos; //color indexed

	ui32 seedToBeUsed; //0 if not sure (client requests server to decide, will be send in reply pack)
	ui32 seedPostInit; //so we know that game is correctly synced at the start; 0 if not known yet
	ui32 mapfileChecksum; //0 if not relevant
	ui8 turnTime; //in minutes, 0=unlimited
	std::string mapname; // empty for random map, otherwise name of the map or savegame
	bool createRandomMap; // true if a random map should be created
	shared_ptr<CMapGenOptions> mapGenOptions; // needs to be not nullptr if createRandomMap=true

	shared_ptr<CCampaignState> campState;

	PlayerSettings & getIthPlayersSettings(TPlayerColor no)
	{
		if(playerInfos.find(no) != playerInfos.end())
			return playerInfos[no];
		tlog1 << "Cannot find info about player " << no <<". Throwing...\n";
		throw std::runtime_error("Cannot find info about player");
	}

	PlayerSettings *getPlayersSettings(const ui8 nameID)
	{
		for(auto it=playerInfos.begin(); it != playerInfos.end(); ++it)
			if(it->second.playerID == nameID)
				return &it->second;

		return NULL;
	}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & mode;
		h & difficulty;
		h & playerInfos;
		h & seedToBeUsed & seedPostInit;
		h & mapfileChecksum;
		h & turnTime;
		h & mapname;
		h & createRandomMap;
		h & mapGenOptions;
		h & campState;
	}

	StartInfo() : mode(INVALID), difficulty(0), seedToBeUsed(0), seedPostInit(0),
		mapfileChecksum(0), turnTime(0), createRandomMap(false)
	{

	}
};
