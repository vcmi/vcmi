#ifndef __CCAMPAIGNHANDLER_H__
#define __CCAMPAIGNHANDLER_H__

#include "../global.h"
#include <string>
#include <vector>

/*
 * CCampaignHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class DLL_EXPORT CCampaignHeader
{
public:
	si32 version; //5 - AB, 6 - SoD, WoG - ?!?
	ui8 mapVersion; //CampText.txt's format
	std::string name, description;
	ui8 difficultyChoosenByPlayer;
	ui8 music; //CmpMusic.txt, start from 0

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & version & mapVersion & name & description & difficultyChoosenByPlayer & music;
	}
};

class DLL_EXPORT CScenarioTravel
{
public:
	ui8 whatHeroKeeps; //bitfield [0] - experience, [1] - prim skills, [2] - sec skills, [3] - spells, [4] - artifacts
	ui8 monstersKeptByHero[19];
	ui8 artifsKeptByHero[18];

	ui8 startOptions; //1 - start bonus, 2 - traveling hero, 3 - hero options

	ui8 playerColor; //only for startOptions == 1

	struct DLL_EXPORT STravelBonus
	{
		ui8 type;	//0 - spell, 1 - monster, 2 - building, 3 - artifact, 4 - spell scroll, 5 - prim skill, 6 - sec skill, 7 - resource,
					//8 - player from previous scenario, 9 - hero [???]
		si32 info1, info2, info3; //purpose depends on type

		template <typename Handler> void serialize(Handler &h, const int formatVersion)
		{
			h & type & info1 & info2 & info3;
		}
	};

	std::vector<STravelBonus> bonusesToChoose;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & whatHeroKeeps & monstersKeptByHero & artifsKeptByHero & startOptions & playerColor & bonusesToChoose;
	}

};

class DLL_EXPORT CCampaignScenario
{
public:
	std::string mapName;
	ui32 packedMapSize;
	ui8 preconditionRegion;
	ui8 regionColor;
	ui8 difficulty;

	std::string regionText;

	struct DLL_EXPORT SScenarioPrologEpilog
	{
		ui8 hasPrologEpilog;
		ui8 prologVideo; // from CmpMovie.txt
		ui8 prologMusic; // from CmpMusic.txt
		std::string prologText;

		template <typename Handler> void serialize(Handler &h, const int formatVersion)
		{
			h & hasPrologEpilog & prologVideo & prologMusic & prologText;
		}
	};

	SScenarioPrologEpilog prolog, epilog;

	CScenarioTravel travelOptions;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & mapName & packedMapSize & preconditionRegion & regionColor & difficulty & regionText & prolog & epilog & travelOptions;
	}
};

class DLL_EXPORT CCampaign
{
public:
	CCampaignHeader header;
	std::vector<CCampaignScenario> scenarios;
	std::vector<std::string> mapPieces; //binary h3ms

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & header & scenarios & mapPieces;
	}
};

class DLL_EXPORT CCampaignHandler
{
	static CCampaignHeader readHeaderFromMemory( const unsigned char *buffer, int & outIt );
	static CCampaignScenario readScenarioFromMemory( const unsigned char *buffer, int & outIt );
	static CScenarioTravel readScenarioTravelFromMemory( const unsigned char * buffer, int & outIt );
	static std::vector<ui32> locateH3mStarts(const unsigned char * buffer, int start, int size);
	static bool startsAt( const unsigned char * buffer, int size, int pos ); //a simple heuristic that checks if a h3m starts at given pos
public:
	static std::vector<CCampaignHeader> getCampaignHeaders();
	static CCampaignHeader getHeader( const std::string & name );

	static CCampaign * getCampaign(const std::string & name);
};


#endif __CCAMPAIGNHANDLER_H__