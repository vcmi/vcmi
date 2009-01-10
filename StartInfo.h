#ifndef __STARTINFO_H__
#define __STARTINFO_H__

#include "global.h"
#include <vector>

enum Ebonus {brandom=-1,bartifact, bgold, bresource};

struct StartInfo
{
	struct PlayerSettings
	{
		si32 castle, hero,  //ID, if -1 then random, if -2 then none
			heroPortrait; //-1 if default, else ID
		std::string heroName;
		si8 bonus; //usees enum type Ebonus
		ui8 color; //from 0 - 
		ui8 serial;
		ui8 handicap;//0-no, 1-mild, 2-severe
		std::string name;
		ui8 human;
		template <typename Handler> 	void serialize(Handler &h, const int version)
		{
			h & castle;
			h & hero;
			h & heroPortrait;
			h & heroName;
			h & bonus;
			h & color;
			h & serial;
			h & handicap;
			h & name;
			h & human;
		}
	};
	ui8 mode; //0 - new game; 1 - load game
	si32 difficulty; //0=easy; 4=impossible
	std::vector<PlayerSettings> playerInfos;
	ui8 turnTime; //in minutes, 0=unlimited
	std::string mapname;
	PlayerSettings & getIthPlayersSettings(int no)
	{
		for(unsigned int i=0; i<playerInfos.size(); ++i)
			if(playerInfos[i].color == no)
				return playerInfos[i];
		tlog1 << "Cannot find info about player " << no <<". Throwing...\n";
		throw std::string("Cannot find info about player");

	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mode;
		h & difficulty;
		h & playerInfos;
		h & turnTime;
		h & mapname;
	}
};


#endif // __STARTINFO_H__
