#pragma once



/*
 * CCampaignHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct StartInfo;
class CGHeroInstance;

namespace CampaignVersion
{
	enum Version
	{
		RoE = 4,
		AB = 5,
		SoD = 6,
		WoG = 6
	};
}

class DLL_LINKAGE CCampaignHeader
{
public:
	si32 version; //4 - RoE, 5 - AB, 6 - SoD and WoG
	ui8 mapVersion; //CampText.txt's format
	std::string name, description;
	ui8 difficultyChoosenByPlayer;
	ui8 music; //CmpMusic.txt, start from 0

	std::string filename;
	ui8 loadFromLod; //if true, this campaign must be loaded fro, .lod file

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & version & mapVersion & name & description & difficultyChoosenByPlayer & music & filename & loadFromLod;
	}
};

class DLL_LINKAGE CScenarioTravel
{
public:
	ui8 whatHeroKeeps; //bitfield [0] - experience, [1] - prim skills, [2] - sec skills, [3] - spells, [4] - artifacts
	ui8 monstersKeptByHero[19];
	ui8 artifsKeptByHero[18];

	ui8 startOptions; //1 - start bonus, 2 - traveling hero, 3 - hero options

	ui8 playerColor; //only for startOptions == 1

	struct DLL_LINKAGE STravelBonus
	{
		ui8 type;	//0 - spell, 1 - monster, 2 - building, 3 - artifact, 4 - spell scroll, 5 - prim skill, 6 - sec skill, 7 - resource,
					//8 - player from previous scenario, 9 - hero [???]
		si32 info1, info2, info3; //purpose depends on type

		bool isBonusForHero() const;

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

class DLL_LINKAGE CCampaignScenario
{
public:
	std::string mapName;
	ui32 packedMapSize; //generally not used
	std::set<ui8> preconditionRegions; //what we need to conquer to conquer this one (stored as bitfield in h3c)
	ui8 regionColor;
	ui8 difficulty;
	ui8 conquered;

	std::string regionText;

	struct DLL_LINKAGE SScenarioPrologEpilog
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

	std::vector<CGHeroInstance*> crossoverHeroes;

	void loadPreconditionRegions(ui32 regions);
	void prepareCrossoverHeroes(std::vector<CGHeroInstance *> heroes);

	bool isNotVoid() const;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & mapName & packedMapSize & preconditionRegions & regionColor & difficulty & conquered & regionText & 
			prolog & epilog & travelOptions & crossoverHeroes;
	}
};

class DLL_LINKAGE CCampaign
{
public:
	CCampaignHeader header;
	std::vector<CCampaignScenario> scenarios;
	std::map<int, std::string> mapPieces; //binary h3ms, scenario number -> map data

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & header & scenarios & mapPieces;
	}

	bool conquerable(int whichScenario) const;

	CCampaign();
};

class DLL_LINKAGE CCampaignState
{
public:
	CCampaign *camp;
	std::string campaignName; 
	std::vector<ui8> mapsConquered, mapsRemaining;
	ui8 currentMap; 

	void initNewCampaign(const StartInfo &si);
	void mapConquered(const std::vector<CGHeroInstance*> & heroes);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & camp & campaignName & mapsRemaining & mapsConquered & currentMap;
	}
};

class DLL_LINKAGE CCampaignHandler
{
	static CCampaignHeader readHeaderFromMemory( const ui8 *buffer, int & outIt );
	static CCampaignScenario readScenarioFromMemory( const ui8 *buffer, int & outIt, int version, int mapVersion );
	static CScenarioTravel readScenarioTravelFromMemory( const ui8 * buffer, int & outIt , int version);
	static std::vector<ui32> locateH3mStarts(const ui8 * buffer, int start, int size);
	static bool startsAt( const ui8 * buffer, int size, int pos ); //a simple heuristic that checks if a h3m starts at given pos
	static std::pair<ui8 *, size_t> getFile(const std::string & name);
public:
	enum GetMode {RoE, AB, SoD, WoG, Custom};
	static std::vector<CCampaignHeader> getCampaignHeaders(GetMode mode);
	static CCampaignHeader getHeader( const std::string & name); //name - name of appropriate file

	static CCampaign * getCampaign(const std::string & name); //name - name of appropriate file
};
