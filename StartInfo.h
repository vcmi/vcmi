#ifndef STARTINFO_H
#define STARTINFO_H

#include "global.h"
#include <vector>

enum Ebonus {brandom=-1,bartifact, bgold, bresource};

struct StartInfo
{
	struct PlayerSettings
	{
		int castle, hero,  //ID, if -1 then random, if -2 then none
			heroPortrait; //-1 if default, else ID
		std::string heroName;
		Ebonus bonus; 
		Ecolor color; //from 0 - 
		int handicap;//0-no, 1-mild, 2-severe
		std::string name;
	};
	std::vector<PlayerSettings> playerInfos;
	int turnTime; //in minutes, 0=unlimited
	PlayerSettings & getIthPlayersSettings(int no)
	{
		for(int i=0; i<playerInfos.size(); ++i)
			if(playerInfos[i].color == no)
				return playerInfos[i];
	}
};

#endif