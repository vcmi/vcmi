#define VCMI_DLL
#include "../stdafx.h"
#include "map.h"
#include "CObjectHandler.h"
#include "CDefObjInfoHandler.h"
#include "VCMI_Lib.h"
#include <zlib.h>
#include <boost/crc.hpp>
#include "CLodHandler.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include <boost/bind.hpp>
#include <assert.h>
#include "CSpellHandler.h"
#include <boost/foreach.hpp>

/*
 * map.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static std::set<si32> convertBuildings(const std::set<si32> h3m, int castleID)
{
	std::map<int,int> mapa;
	std::set<si32> ret;
	std::ifstream b5(DATA_DIR "/config/buildings5.txt");
	while(!b5.eof())
	{
		int h3, VCMI, town;
		b5 >> town >> h3 >> VCMI;
		if ( town == castleID || town == -1 )
			mapa[h3]=VCMI;
	}
	for(std::set<si32>::const_iterator i=h3m.begin();i!=h3m.end();i++)
	{
		if(mapa[*i]>=0)
			ret.insert(mapa[*i]);
		else if(mapa[*i]  >=  (-CREATURES_PER_TOWN)) // horde buildings
		{
			int level = (mapa[*i]);
			ret.insert(level-30);//(-30)..(-36) - horde buildings (for game loading only), don't see other way to handle hordes in random towns
		}
		else
		{
			tlog3<<"Conversion warning: unknown building "<<*i<<" in castle "<<castleID<<std::endl;
		}
	}

	ret.insert(10); //village hall is always present
	ret.insert(-1); //houses near v.hall / eyecandies
	ret.insert(-2); //terrain eyecandy, if -1 is used

	if(ret.find(11)!=ret.end())
		ret.insert(27);
	if(ret.find(12)!=ret.end())
		ret.insert(28);
	if(ret.find(13)!=ret.end())
		ret.insert(29);

	return ret;
}
static unsigned int intPow(unsigned int a, unsigned int b)
{
	unsigned int ret=1;
	for(unsigned int i=0; i<b; ++i)
		ret*=a;
	return ret;
}
static unsigned char reverse(unsigned char arg)
{
	unsigned char ret = 0;
	for (int i=0; i<8;i++)
	{
		if(((arg)&(1<<i))>>i)
		{
			ret |= (128>>i);
		}
	}
	return ret;
}

void readCreatureSet(CCreatureSet *out, const unsigned char * bufor, int &i, int number, bool version) //version==true for >RoE maps
{
	const int bytesPerCre = version ? 4 : 3,
		idBytes = version ? 2 : 1,
		maxID = version ? 0xffff : 0xff;

	for(int ir=0;ir < number; ir++)
	{
		int creID = readNormalNr(bufor,i+ir*bytesPerCre, idBytes);
		int count = readNormalNr(bufor,i+ir*bytesPerCre+idBytes, 2);
		if(creID == maxID) //empty slot
			continue;

		CStackInstance *hlp = new CStackInstance();
		hlp->count = count;

		if(creID > maxID - 0xf)
		{
			creID = maxID + 1 - creID + VLC->creh->creatures.size();//this will happen when random object has random army
			hlp->idRand = creID;
		}
		else
			hlp->setType(creID);

		out->putStack(ir, hlp);
	}
	i+=number*bytesPerCre;
	
	out->validTypes(true);
}

CMapHeader::CMapHeader(const unsigned char *map)
{
	int i=0;
	initFromMemory(map,i);
}

CMapHeader::CMapHeader()
{
	areAnyPLayers = difficulty = levelLimit = howManyTeams = 0;
	height = width = twoLevel = -1;
}

void CMapHeader::initFromMemory( const unsigned char *bufor, int &i )
{
	version = (Eformat)(readNormalNr(bufor,i)); i+=4; //map version
	if(version != RoE && version != AB && version != SoD && version != WoG)
	{
		throw std::string("Invalid map format!");
	}
	areAnyPLayers = readChar(bufor,i); //invalid on some maps
	height = width = (readNormalNr(bufor,i)); i+=4; // dimensions of map
	twoLevel = readChar(bufor,i); //if there is underground
	int pom;
	name = readString(bufor,i);
	description= readString(bufor,i);
	difficulty = readChar(bufor,i); // reading map difficulty
	if(version != RoE)
		levelLimit = readChar(bufor,i); // hero level limit
	else
		levelLimit = 0;
	loadPlayerInfo(pom, bufor, i);
	loadViCLossConditions(bufor, i);

	howManyTeams=bufor[i++]; //read number of teams
	if(howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
			players[rr].team = bufor[i++];
		}
	}
	else//no alliances
	{
		for(int i=0;i<PLAYER_LIMIT;i++)
			if(players[i].canComputerPlay || players[i].canHumanPlay)
				players[i].team = howManyTeams++;
	}


	pom = i;
	allowedHeroes.resize(HEROES_QUANTITY,false);
	for(; i<pom+ (version == RoE ? 16 : 20) ; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
			if((i-pom)*8+yy < HEROES_QUANTITY)
				if(c == (c|((unsigned char)intPow(2, yy))))
					allowedHeroes[(i-pom)*8+yy] = true;
	}
	if(version>RoE) //probably reserved for further heroes
	{
		int placeholdersQty = readNormalNr(bufor, i); i+=4;
		for(int p = 0; p < placeholdersQty; p++)
			placeholdedHeroes.push_back(bufor[i++]);
	}
}
void CMapHeader::loadPlayerInfo( int &pom, const unsigned char * bufor, int &i )
{
	players.resize(8);
	for (pom=0;pom<8;pom++)
	{
		players[pom].canHumanPlay = bufor[i++];
		players[pom].canComputerPlay = bufor[i++];
		if ((!(players[pom].canHumanPlay || players[pom].canComputerPlay))) //if nobody can play with this player
		{
			switch(version)
			{
			case SoD: case WoG: 
				i+=13;
				break;
			case AB:
				i+=12;
				break;
			case RoE:
				i+=6;
				break;
			}
			continue;
		}

		players[pom].AITactic = bufor[i++];

		if(version == SoD || version == WoG)
			players[pom].p7= bufor[i++];
		else
			players[pom].p7= -1;

		//factions this player can choose
		players[pom].allowedFactions = 0;
		players[pom].allowedFactions += bufor[i++];
		if(version != RoE)
			players[pom].allowedFactions += (bufor[i++])*256;

		players[pom].isFactionRandom = bufor[i++];
		players[pom].hasMainTown = bufor[i++];
		if (players[pom].hasMainTown)
		{
			if(version != RoE)
			{
				players[pom].generateHeroAtMainTown = bufor[i++];
				players[pom].generateHero = bufor[i++];
			}
			else
			{
				players[pom].generateHeroAtMainTown = true;
				players[pom].generateHero = false;
			}

			players[pom].posOfMainTown.x = bufor[i++];
			players[pom].posOfMainTown.y = bufor[i++];
			players[pom].posOfMainTown.z = bufor[i++];


		}
		players[pom].p8= bufor[i++];
		players[pom].p9= bufor[i++];		
		if(players[pom].p9!=0xff)
		{
			players[pom].mainHeroPortrait = bufor[i++];
			players[pom].mainHeroName = readString(bufor,i);
		}

		if(version != RoE)
		{
			players[pom].powerPlacehodlers = bufor[i++];//unknown byte
			int heroCount = bufor[i++];
			i+=3;
			for (int pp=0;pp<heroCount;pp++)
			{
				SheroName vv;
				vv.heroID=bufor[i++];
				int hnl = bufor[i++];
				i+=3;
				for (int zz=0;zz<hnl;zz++)
				{
					vv.heroName+=bufor[i++];
				}
				players[pom].heroesNames.push_back(vv);
			}
		}
	}
}

void CMapHeader::loadViCLossConditions( const unsigned char * bufor, int &i)
{
	victoryCondition.obj = NULL;
	victoryCondition.condition = (EvictoryConditions)bufor[i++];
	if (victoryCondition.condition != winStandard) //specific victory conditions
	{
		int nr;
		switch (victoryCondition.condition) //read victory conditions
		{
		case artifact:
			{
				victoryCondition.ID = bufor[i+2];
				nr=(version==RoE ? 1 : 2);
				break;
			}
		case gatherTroop:
			{
// 				int temp1 = bufor[i+2];
// 				int temp2 = bufor[i+3];
				victoryCondition.ID = bufor[i+2];
				victoryCondition.count = readNormalNr(bufor, i+(version==RoE ? 3 : 4));
				nr=(version==RoE ? 5 : 6);
				break;
			}
		case gatherResource:
			{
				victoryCondition.ID = bufor[i+2];
				victoryCondition.count = readNormalNr(bufor, i+3);
				nr = 5;
				break;
			}
		case buildCity:
			{
				victoryCondition.pos.x = bufor[i+2];
				victoryCondition.pos.y = bufor[i+3];
				victoryCondition.pos.z = bufor[i+4];
				victoryCondition.count = bufor[i+5];
				victoryCondition.ID = bufor[i+6];
				nr = 5;
				break;
			}
		case buildGrail:
			{
				if (bufor[i+4]>2)
					victoryCondition.pos = int3(-1,-1,-1);
				else
				{
					victoryCondition.pos.x = bufor[i+2];
					victoryCondition.pos.y = bufor[i+3];
					victoryCondition.pos.z = bufor[i+4];
				}
				nr = 3;
				break;
			}
		case beatHero:
		case captureCity:
		case beatMonster:
			{
				victoryCondition.pos.x = bufor[i+2];
				victoryCondition.pos.y = bufor[i+3];
				victoryCondition.pos.z = bufor[i+4];
				nr = 3;
				break;
			}
		case takeDwellings:
		case takeMines:
			{
				nr = 0;
				break;
			}
		case transportItem:
			{
				victoryCondition.ID =  bufor[i+2];
				victoryCondition.pos.x = bufor[i+3];
				victoryCondition.pos.y = bufor[i+4];
				victoryCondition.pos.z = bufor[i+5];
				nr = 4;
				break;
			}
		}
		victoryCondition.allowNormalVictory = bufor[i++];
		victoryCondition.appliesToAI = bufor[i++];
		i+=nr;
	}
	lossCondition.typeOfLossCon = (ElossCon)bufor[i++];
	switch (lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
	case lossHero:
		{
			lossCondition.pos.x=bufor[i++];
			lossCondition.pos.y=bufor[i++];
			lossCondition.pos.z=bufor[i++];
			break;
		}
	case timeExpires:
		{
			lossCondition.timeLimit = readNormalNr(bufor,i++,2);
			i++;
			break;
		}
	}
}

CMapHeader::~CMapHeader()
{

}

void Mapa::initFromBytes(const unsigned char * bufor)
{
	int i=0;
	initFromMemory(bufor,i);
	timeHandler th;
	th.getDif();
	readHeader(bufor, i);
	tlog0<<"\tReading header: "<<th.getDif()<<std::endl;

	if (victoryCondition.condition == artifact || victoryCondition.condition == transportItem)
	{ //messy, but needed
		allowedArtifact[victoryCondition.ID] = false;
	}

	readRumors(bufor, i);
	tlog0<<"\tReading rumors: "<<th.getDif()<<std::endl;

	readPredefinedHeroes(bufor, i);
	tlog0<<"\tReading predefined heroes: "<<th.getDif()<<std::endl;

	readTerrain(bufor, i);
	tlog0<<"\tReading terrain: "<<th.getDif()<<std::endl;

	readDefInfo(bufor, i);
	tlog0<<"\tReading defs info: "<<th.getDif()<<std::endl;

	readObjects(bufor, i);
	tlog0<<"\tReading objects: "<<th.getDif()<<std::endl;
	
	readEvents(bufor, i);
	tlog0<<"\tReading events: "<<th.getDif()<<std::endl;

	//map readed, bufor no longer needed	
	for(unsigned int f=0; f<objects.size(); ++f) //calculationg blocked / visitable positions
	{
		if(!objects[f]->defInfo)
			continue;
		addBlockVisTiles(objects[f]);
	}
	tlog0<<"\tCalculating blocked/visitable tiles: "<<th.getDif()<<std::endl;
	tlog0 << "\tMap initialization done!" << std::endl;
}	
void Mapa::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(total || ((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects -= obj;
					curt.blocked = curt.blockingObjects.size();
				}
			}
		}
	}
}
void Mapa::addBlockVisTiles(CGObjectInstance * obj)
{
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(!((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
			}
		}
	}
}
Mapa::Mapa(std::string filename)
	:grailPos(-1, -1, -1), grailRadious(0)
{
	int mapsize = 0;

	tlog0<<"Opening map file: "<<filename<<"\t "<<std::flush;
	
	//load file and decompress
	unsigned char * initTable = CLodHandler::getUnpackedFile(filename, &mapsize);

	tlog0<<"done."<<std::endl;

	// Compute checksum
	boost::crc_32_type  result;
	result.process_bytes(initTable, mapsize);
	checksum = result.checksum();
	tlog0 << "\tOur map checksum: "<<result.checksum() << std::endl;

	initFromBytes(initTable);

	delete [] initTable;
}

Mapa::Mapa()
{
	terrain = NULL;

}
Mapa::~Mapa()
{
	//for(int i=0; i < defy.size(); i++)
	//	if(defy[i]->serial < 0) //def not present in the main vector in defobjinfo
	//		delete defy[i];

	if(terrain)
	{
		for (int ii=0;ii<width;ii++)
		{
			for(int jj=0;jj<height;jj++)
				delete [] terrain[ii][jj];
			delete [] terrain[ii];
		}
		delete [] terrain;
	}
	for(std::list<ConstTransitivePtr<CMapEvent> >::iterator i = events.begin(); i != events.end(); i++)
		i->dellNull();
}

CGHeroInstance * Mapa::getHero(int ID, int mode)
{
	if (mode != 0)
#ifndef __GNUC__
		throw new std::exception("gs->getHero: This mode is not supported!");
#else
		throw new std::exception();
#endif

	for(unsigned int i=0; i<heroes.size();i++)
		if(heroes[i]->subID == ID)
			return heroes[i];
	return NULL;
}

int Mapa::loadSeerHut( const unsigned char * bufor, int i, CGObjectInstance *& nobj )
{
	CGSeerHut *hut = new CGSeerHut();
	nobj = hut;

	if(version>RoE)
	{
		loadQuest(hut,bufor,i);
	}
	else //RoE
	{
		int artID = bufor[i]; ++i;
		if(artID!=255) //not none quest
		{
			hut->m5arts.push_back(artID);
			hut->missionType = 5;
		}
		else
		{
			hut->missionType = 0; //no mission
		}
		hut->isCustom = false;
	}

	if(hut->missionType)
	{
		unsigned char rewardType = bufor[i]; ++i;
		hut->rewardType = rewardType;

		switch(rewardType)
		{
		case 1:
			{
				hut->rVal = readNormalNr(bufor,i); i+=4;
				break;
			}
		case 2:
			{
				hut->rVal = readNormalNr(bufor,i); i+=4;
				break;
			}
		case 3:
			{
				hut->rVal = bufor[i]; ++i;
				break;
			}
		case 4:
			{
				hut->rVal = bufor[i]; ++i;
				break;
			}
		case 5:
			{
				hut->rID = bufor[i]; ++i;
				hut->rVal = readNormalNr(bufor,i, 3); i+=3;
				i+=1;
				break;
			}
		case 6:
			{
				hut->rID = bufor[i]; ++i;
				hut->rVal = bufor[i]; ++i;
				break;
			}
		case 7:
			{
				hut->rID = bufor[i]; ++i;
				hut->rVal = bufor[i]; ++i;
				break;
			}
		case 8:
			{
				hut->rID = readNormalNr(bufor,i, (version == RoE ? 1 : 2)); i+=(version == RoE ? 1 : 2);
				break;
			}
		case 9:
			{
				hut->rID = bufor[i]; ++i;
				break;
			}
		case 10:
			{
				if(version>RoE)
				{
					hut->rID = readNormalNr(bufor,i, 2); i+=2;
					hut->rVal = readNormalNr(bufor,i, 2); i+=2;
				}
				else
				{
					hut->rID = bufor[i]; ++i;
					hut->rVal = readNormalNr(bufor,i, 2); i+=2;
				}
				break;
			}
		}// end of internal switch
		i+=2;
	}
	else //missionType==255
	{
		i+=3;
	}
	return i;
}

void Mapa::loadTown( CGObjectInstance * &nobj, const unsigned char * bufor, int &i, int subid)
{
	CGTownInstance * nt = new CGTownInstance();
	//(*(static_cast<CGObjectInstance*>(nt))) = *nobj;
	//delete nobj;
	nobj = nt;
	nt->identifier = 0;
	if(version>RoE)
	{	
		nt->identifier = readNormalNr(bufor,i); i+=4;
	}
	nt->tempOwner = bufor[i]; ++i;
	if(readChar(bufor,i)) //has name
		nt->name = readString(bufor,i);
	if(readChar(bufor,i))//true if garrison isn't empty
		readCreatureSet(nt, bufor, i, 7, version > RoE);
	nt->formation = bufor[i]; ++i;
	if(readChar(bufor,i)) //custom buildings info
	{
		//built buildings
		for(int byte=0;byte<6;byte++)
		{
			for(int bit=0;bit<8;bit++)
				if(bufor[i] & (1<<bit))
					nt->builtBuildings.insert(byte*8+bit);
			i++;
		}
		//forbidden buildings
		for(int byte=6;byte<12;byte++)
		{
			for(int bit=0;bit<8;bit++)
				if(bufor[i] & (1<<bit))
					nt->forbiddenBuildings.insert((byte-6)*8+bit);
			i++;
		}
		nt->builtBuildings = convertBuildings(nt->builtBuildings,subid);
		nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings,subid);
	}
	else //standard buildings
	{
		if(readChar(bufor,i)) //has fort
			nt->builtBuildings.insert(7);
		nt->builtBuildings.insert(-50); //means that set of standard building should be included
	}

	int ist = i;
	if(version>RoE)
	{
		for(; i<ist+9; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < SPELLS_QUANTITY)
				{
					if(c == (c|((unsigned char)intPow(2, yy))))
						nt->obligatorySpells.push_back((i-ist)*8+yy);
				}
			}
		}
	}

	ist = i;
	for(; i<ist+9; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < SPELLS_QUANTITY)
			{
				if(c != (c|((unsigned char)intPow(2, yy))))
					nt->possibleSpells.push_back((i-ist)*8+yy);
			}
		}
	}

	/////// reading castle events //////////////////////////////////

	int numberOfEvent = readNormalNr(bufor,i); i+=4;

	for(int gh = 0; gh<numberOfEvent; ++gh)
	{
		CCastleEvent *nce = new CCastleEvent();
		nce->town = nt;
		nce->name = readString(bufor,i);
		nce->message = readString(bufor,i);
		nce->resources.resize(RESOURCE_QUANTITY);
		for(int x=0; x < 7; x++)
		{
			nce->resources[x] = readNormalNr(bufor,i); 
			i+=4;
		}

		nce->players = bufor[i]; ++i;
		if(version > AB)
		{
			nce->humanAffected = bufor[i]; ++i;
		}
		else
			nce->humanAffected = true;

		nce->computerAffected = bufor[i]; ++i;
		nce->firstOccurence = readNormalNr(bufor,i, 2); i+=2;
		nce->nextOccurence = bufor[i]; ++i;

		i+=17;

		//new buildings
		for(int byte=0;byte<6;byte++)
		{
			for(int bit=0;bit<8;bit++)
				if(bufor[i] & (1<<bit))
					nce->buildings.insert(byte*8+bit);
			i++;
		}
		nce->buildings = convertBuildings(nce->buildings,subid);
		
		nce->creatures.resize(7);
		for(int vv=0; vv<7; ++vv)
		{
			nce->creatures[vv] = readNormalNr(bufor,i, 2);i+=2;
		}
		i+=4;
		nt->events.push_back(nce);
	}//castle events have been read 

	if(version > AB)
	{
		nt->alignment = bufor[i]; ++i;
	}
	else
		nt->alignment = 0xff;
	i+=3;

	nt->builded = 0;
	nt->destroyed = 0;
	nt->garrisonHero = NULL;
}

CGObjectInstance * Mapa::loadHero(const unsigned char * bufor, int &i, int idToBeGiven)
{
	CGHeroInstance * nhi = new CGHeroInstance();

	int identifier = 0;
	if(version > RoE)
	{
		identifier = readNormalNr(bufor,i, 4); i+=4;
		questIdentifierToId[identifier] = idToBeGiven;
	}

	ui8 owner = bufor[i++];
	nhi->subID = readNormalNr(bufor,i, 1); ++i;	
	
	for(unsigned int j=0; j<predefinedHeroes.size(); j++)
	{
		if(predefinedHeroes[j]->subID == nhi->subID)
		{
			tlog0 << "Hero " << nhi->subID << " will be taken from the predefined heroes list.\n";
			delete nhi;
			nhi = predefinedHeroes[j];
			break;
		}
	}
	nhi->setOwner(owner);

	nhi->portrait = nhi->subID;

	for(unsigned int j=0; j<disposedHeroes.size(); j++)
	{
		if(disposedHeroes[j].ID == nhi->subID)
		{
			nhi->name = disposedHeroes[j].name;
			nhi->portrait = disposedHeroes[j].portrait;
			break;
		}
	}
	if(readChar(bufor,i))//true if hero has nonstandard name
		nhi->name = readString(bufor,i);
	if(version>AB)
	{
		if(readChar(bufor,i))//true if hero's experience is greater than 0
		{	nhi->exp = readNormalNr(bufor,i); i+=4;	}
		else
			nhi->exp = 0xffffffff;
	}
	else
	{	
		nhi->exp = readNormalNr(bufor,i); i+=4;	
		if(!nhi->exp) //0 means "not set" in <=AB maps
			nhi->exp = 0xffffffff;
	}

	bool portrait=bufor[i]; ++i;
	if (portrait)
		nhi->portrait = bufor[i++];
	if(readChar(bufor,i))//true if hero has specified abilities
	{
		int howMany = readNormalNr(bufor,i); i+=4;
		nhi->secSkills.resize(howMany);
		for(int yy=0; yy<howMany; ++yy)
		{
			nhi->secSkills[yy].first = readNormalNr(bufor,i, 1); ++i;
			nhi->secSkills[yy].second = readNormalNr(bufor,i, 1); ++i;
		}
	}
	if(readChar(bufor,i))//true if hero has nonstandard garrison
		readCreatureSet(nhi, bufor, i, 7, version > RoE);

	nhi->formation =bufor[i]; ++i; //formation
	loadArtifactsOfHero(bufor, i, nhi);
	nhi->patrol.patrolRadious = readNormalNr(bufor,i, 1); ++i;
	if(nhi->patrol.patrolRadious == 0xff)
		nhi->patrol.patrolling = false;
	else 
		nhi->patrol.patrolling = true;

	if(version>RoE)
	{
		if(readChar(bufor,i))//true if hero has nonstandard (mapmaker defined) biography
			nhi->biography = readString(bufor,i);
		nhi->sex = bufor[i]; ++i;
		
		if (nhi->sex != 0xFF)//remove trash
			nhi->sex &=1;
	}
	else
		nhi->sex = 0xFF;
	//spells
	if(version>AB)
	{
		bool areSpells = bufor[i]; ++i;

		if(areSpells) //TODO: sprawdziæ //seems to be ok - tow
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			int ist = i;
			for(; i<ist+9; ++i)
			{
				unsigned char c = bufor[i];
				for(int yy=0; yy<8; ++yy)
				{
					if((i-ist)*8+yy < SPELLS_QUANTITY)
					{
						if(c == (c|((unsigned char)intPow(2, yy))))
							nhi->spells.insert((i-ist)*8+yy);
					}
				}
			}
		}
	}
	else if(version==AB) //we can read one spell
	{
		unsigned char buff = bufor[i]; ++i;
		if(buff != 254)
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			if(buff < 254) //255 means no spells
				nhi->spells.insert(buff);
		}
	}
	//spells loaded
	if(version>AB)
	{
		if(readChar(bufor,i))//customPrimSkills
		{
			for(int xx=0;xx<4;xx++)
				nhi->pushPrimSkill(xx, bufor[i++]);
		}
	}
	i+=16;

	return nhi;
}

void Mapa::readRumors( const unsigned char * bufor, int &i)
{
	int rumNr = readNormalNr(bufor,i,4);i+=4;
	for (int it=0;it<rumNr;it++)
	{
		Rumor ourRumor;
		int nameL = readNormalNr(bufor,i,4);i+=4; //read length of name of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.name+=bufor[i++];
		nameL = readNormalNr(bufor,i,4);i+=4; //read length of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.text+=bufor[i++];
		rumors.push_back(ourRumor); //add to our list
	}
}

void Mapa::readHeader( const unsigned char * bufor, int &i)
{
	//reading allowed heroes (20 bytes)
	int ist = i;				//starting i for loop
	unsigned char disp = 0;

	if(version>=SoD)
	{
		disp = bufor[i++];
		disposedHeroes.resize(disp);
		for(int g=0; g<disp; ++g)
		{
			disposedHeroes[g].ID = bufor[i++];
			disposedHeroes[g].portrait = bufor[i++];
			int lenbuf = readNormalNr(bufor,i); i+=4;
			for (int zz=0; zz<lenbuf; zz++)
				disposedHeroes[g].name+=bufor[i++];
			disposedHeroes[g].players = bufor[i++];
		}
	}

	i+=31; //omitting NULLS

	allowedArtifact.resize(ARTIFACTS_QUANTITY);
	for (unsigned int x=0; x<allowedArtifact.size(); x++)
		allowedArtifact[x] = true;

	//reading allowed artifacts:  17 or 18 bytes
	if (version!=RoE)
	{
		ist=i; //starting i for loop
		for (; i<ist+(version==AB ? 17 : 18); ++i)
		{
			unsigned char c = bufor[i];
			for (int yy=0; yy<8; ++yy)
			{
				if ((i-ist)*8+yy < ARTIFACTS_QUANTITY)
				{
					if (c == (c|((unsigned char)intPow(2, yy))))
						allowedArtifact[(i-ist)*8+yy] = false;
				}
			}
		}//allowed artifacts have been read
	}


	allowedSpell.resize(SPELLS_QUANTITY);
	for(unsigned int x=0;x<allowedSpell.size();x++)
		allowedSpell[x] = true;

	allowedAbilities.resize(SKILL_QUANTITY);
	for(unsigned int x=0;x<allowedAbilities.size();x++)
		allowedAbilities[x] = true;

	if(version>=SoD)
	{
		//reading allowed spells (9 bytes)
		ist=i; //starting i for loop
		for(; i<ist+9; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
				if((i-ist)*8+yy < SPELLS_QUANTITY)
					if(c == (c|((unsigned char)intPow(2, yy))))
						allowedSpell[(i-ist)*8+yy] = false;
		}


		//allowed hero's abilities (4 bytes)
		ist=i; //starting i for loop
		for(; i<ist+4; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < SKILL_QUANTITY)
				{
					if(c == (c|((unsigned char)intPow(2, yy))))
						allowedAbilities[(i-ist)*8+yy] = false;
				}
			}
		}
	}
}

void Mapa::readPredefinedHeroes( const unsigned char * bufor, int &i)
{
	switch(version)
	{
	case WoG: case SoD:
		{
			for(int z=0;z<HEROES_QUANTITY;z++) //disposed heroes
			{
				int custom =  bufor[i++];
				if(!custom)
					continue;
				CGHeroInstance * cgh = new CGHeroInstance;
				cgh->ID = HEROI_TYPE;
				cgh->subID = z;
				if(readChar(bufor,i))//true if hore's experience is greater than 0
				{	cgh->exp = readNormalNr(bufor,i); i+=4;	}
				else
					cgh->exp = 0;
				if(readChar(bufor,i))//true if hero has specified abilities
				{
					int howMany = readNormalNr(bufor,i); i+=4;
					cgh->secSkills.resize(howMany);
					for(int yy=0; yy<howMany; ++yy)
					{
						cgh->secSkills[yy].first = readNormalNr(bufor,i, 1); ++i;
						cgh->secSkills[yy].second = readNormalNr(bufor,i, 1); ++i;
					}
				}

				loadArtifactsOfHero(bufor, i, cgh);

				if(readChar(bufor,i))//customBio
					cgh->biography = readString(bufor,i);
				int sex = bufor[i++]; // 0xFF is default, 00 male, 01 female    //FIXME:unused?
				if(readChar(bufor,i))//are spells
				{
					int ist = i;
					for(; i<ist+9; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < SPELLS_QUANTITY)
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									cgh->spells.insert((i-ist)*8+yy);
							}
						}
					}
				}
				if(readChar(bufor,i))//customPrimSkills
				{
					for(int xx=0;xx<4;xx++)
						cgh->pushPrimSkill(xx, bufor[i++]);
				}
				predefinedHeroes.push_back(cgh);
			}
			break;
		}
	case RoE:
		i+=0;
		break;
	}
}

void Mapa::readTerrain( const unsigned char * bufor, int &i)
{
	terrain = new TerrainTile**[width]; // allocate memory 
	for (int ii=0;ii<width;ii++)
	{
		terrain[ii] = new TerrainTile*[height]; // allocate memory 
		for(int jj=0;jj<height;jj++)
			terrain[ii][jj] = new TerrainTile[twoLevel+1];
	}

	for (int c=0; c<width; c++) // reading terrain
	{
		for (int z=0; z<height; z++)
		{
			terrain[z][c][0].tertype = static_cast<TerrainTile::EterrainType>(bufor[i++]);
			terrain[z][c][0].terview = bufor[i++];
			terrain[z][c][0].nuine = static_cast<TerrainTile::Eriver>(bufor[i++]);
			terrain[z][c][0].rivDir = bufor[i++];
			terrain[z][c][0].malle = static_cast<TerrainTile::Eroad>(bufor[i++]);
			terrain[z][c][0].roadDir = bufor[i++];
			terrain[z][c][0].siodmyTajemniczyBajt = bufor[i++];
			terrain[z][c][0].blocked = 0;
			terrain[z][c][0].visitable = 0;
		}
	}
	if (twoLevel) // read underground terrain
	{
		for (int c=0; c<width; c++) // reading terrain
		{
			for (int z=0; z<height; z++)
			{
				terrain[z][c][1].tertype = static_cast<TerrainTile::EterrainType>(bufor[i++]);
				terrain[z][c][1].terview = bufor[i++];
				terrain[z][c][1].nuine = static_cast<TerrainTile::Eriver>(bufor[i++]);
				terrain[z][c][1].rivDir = bufor[i++];
				terrain[z][c][1].malle = static_cast<TerrainTile::Eroad>(bufor[i++]);
				terrain[z][c][1].roadDir = bufor[i++];
				terrain[z][c][1].siodmyTajemniczyBajt = bufor[i++];
				terrain[z][c][1].blocked = 0;
				terrain[z][c][1].visitable = 0;
			}
		}
	}
}

void Mapa::readDefInfo( const unsigned char * bufor, int &i)
{
	int defAmount = readNormalNr(bufor,i); i+=4;
	defy.reserve(defAmount+8);
	for (int idd = 0 ; idd<defAmount; idd++) // reading defs
	{
		CGDefInfo * vinya = new CGDefInfo(); // info about new def 

		//reading name
		int nameLength = readNormalNr(bufor,i,4);i+=4;
		vinya->name.reserve(nameLength);
		for (int cd=0;cd<nameLength;cd++)
		{
			vinya->name += bufor[i++];
		}
		std::transform(vinya->name.begin(),vinya->name.end(),vinya->name.begin(),(int(*)(int))toupper);


		unsigned char bytes[12];
		for (int v=0; v<12; v++) // read info
		{
			bytes[v] = bufor[i++];
		}
		vinya->terrainAllowed = readNormalNr(bufor,i,2);i+=2;
		vinya->terrainMenu = readNormalNr(bufor,i,2);i+=2;
		vinya->id = readNormalNr(bufor,i,4);i+=4;
		vinya->subid = readNormalNr(bufor,i,4);i+=4;
		vinya->type = bufor[i++];
		vinya->printPriority = bufor[i++];
		for (int zi=0; zi<6; zi++)
		{
			vinya->blockMap[zi] = reverse(bytes[zi]);
		}
		for (int zi=0; zi<6; zi++)
		{
			vinya->visitMap[zi] = reverse(bytes[6+zi]);
		}
		i+=16;
		if(vinya->id!=HEROI_TYPE && vinya->id!=70)
		{
			CGDefInfo *h = VLC->dobjinfo->gobjs[vinya->id][vinya->subid];
			if(!h) 
			{
				//remove fake entry
				VLC->dobjinfo->gobjs[vinya->id].erase(vinya->subid);
				if(VLC->dobjinfo->gobjs[vinya->id].size())
					VLC->dobjinfo->gobjs.erase(vinya->id);
				tlog2<<"\t\tWarning: no defobjinfo entry for object ID="<<vinya->id<<" subID=" << vinya->subid<<std::endl;
			}
			else
			{
				vinya->visitDir = VLC->dobjinfo->gobjs[vinya->id][vinya->subid]->visitDir;
			}
		}
		else
		{
			vinya->visitDir = 0xff;
		}

		if(vinya->id == EVENTI_TYPE)
			std::memset(vinya->blockMap,255,6);

		//calculating coverageMap
		vinya->fetchInfoFromMSK();

		defy.push_back(vinya); // add this def to the vector
	}

	//add holes - they always can appear 
	for (int i = 0; i < 8 ; i++)
	{
		defy.push_back(VLC->dobjinfo->gobjs[124][i]);
	}
}

class _HERO_SORTER
{
public:
	bool operator()(const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
	{
		return a->subID < b->subID;
	}
};

void Mapa::readObjects( const unsigned char * bufor, int &i)
{
	int howManyObjs = readNormalNr(bufor,i, 4); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		CGObjectInstance * nobj = 0;

		int3 pos;
		pos.x = bufor[i++];
		pos.y = bufor[i++];
		pos.z = bufor[i++];

		int defnum = readNormalNr(bufor,i, 4); i+=4;
		int idToBeGiven = objects.size();

		CGDefInfo * defInfo = defy[defnum];
		i+=5;

		switch(defInfo->id)
		{
		case EVENTI_TYPE: //for event objects
			{
				CGEvent *evnt = new CGEvent();
				nobj = evnt;

				bool guardMess = bufor[i]; ++i;
				if(guardMess)
				{
					int messLong = readNormalNr(bufor,i, 4); i+=4;
					if(messLong>0)
					{
						for(int yy=0; yy<messLong; ++yy)
						{
							evnt->message +=bufor[i+yy];
						}
						i+=messLong;
					}
					if(bufor[i++])
					{
						readCreatureSet(evnt, bufor, i, 7, version > RoE); 
					}
					i+=4;
				}
				evnt->gainedExp = readNormalNr(bufor,i, 4); i+=4;
				evnt->manaDiff = readNormalNr(bufor,i, 4); i+=4;
				evnt->moraleDiff = readNormalNr(bufor,i, 1, true); ++i;
				evnt->luckDiff = readNormalNr(bufor,i, 1, true); ++i;

				evnt->resources.resize(RESOURCE_QUANTITY);
				for(int x=0; x<7; x++)
				{
					evnt->resources[x] = readNormalNr(bufor,i); 
					i+=4;
				}

				evnt->primskills.resize(PRIMARY_SKILLS);
				for(int x=0; x<4; x++)
				{
					evnt->primskills[x] = readNormalNr(bufor,i, 1); 
					i++;
				}

				int gabn; //number of gained abilities
				gabn = readNormalNr(bufor,i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					evnt->abilities.push_back(readNormalNr(bufor,i, 1)); ++i;
					evnt->abilityLevels.push_back(readNormalNr(bufor,i, 1)); ++i;
				}

				int gart = readNormalNr(bufor,i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					evnt->artifacts.push_back(readNormalNr(bufor,i, (version == RoE ? 1 : 2))); i+=(version == RoE ? 1 : 2);
				}

				int gspel = readNormalNr(bufor,i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					evnt->spells.push_back(readNormalNr(bufor,i, 1)); ++i;
				}

				int gcre = readNormalNr(bufor,i, 1); ++i; //number of gained creatures
				readCreatureSet(&evnt->creatures, bufor,i,gcre,(version>RoE));

				i+=8;
				evnt->availableFor = readNormalNr(bufor,i, 1); ++i;
				evnt->computerActivate = readNormalNr(bufor,i, 1); ++i;
				evnt->removeAfterVisit = readNormalNr(bufor,i, 1); ++i;
				evnt->humanActivate = true;

				i+=4;
				break;
			}
		case 34: case 70: case 62: //34 - hero; 70 - random hero; 62 - prison
			{
				nobj = loadHero(bufor, i, idToBeGiven);
				break;
			}
		case 4: //Arena
		case 51: //Mercenary Camp
		case 23: //Marletto Tower
		case 61: // Star Axis
		case 32: // Garden of Revelation
		case 100: //Learning Stone
		case 102: //Tree of Knowledge
		case 41: //Library of Enlightenment
		case 47: //School of Magic
		case 107: //School of War
			{
				nobj = new CGVisitableOPH();
				break;
			}
		case 55: //mystical garden
		case 112://windmill
		case 109://water wheel
			{
				nobj = new CGVisitableOPW();
				break;
			}
		case 43: //teleport
		case 44: //teleport
		case 45: //teleport
		case 103://subterranean gate
		case 111://Whirlpool
			{
				nobj = new CGTeleport();
				break;
			}
		case 12: //campfire
		case 29: //Flotsam
		case 82: //Sea Chest
		case 86: //Shipwreck Survivor
		case 101://treasure chest
			{
				nobj = new CGPickable();
				break;
			}
		case 54:  //Monster 
		case 71: case 72: case 73: case 74: case 75:	// Random Monster 1 - 4
		case 162: case 163: case 164:					// Random Monster 5 - 7
			{
				CGCreature *cre = new CGCreature();
				nobj = cre;

				if(version>RoE)
				{
					cre->identifier = readNormalNr(bufor,i); i+=4;
					questIdentifierToId[cre->identifier] = idToBeGiven;
					//monsters[cre->identifier] = cre;
				}

				CStackInstance *hlp = new CStackInstance();
				hlp->count =  readNormalNr(bufor,i, 2); i+=2;
				//type will be set during initialization
				cre->putStack(0, hlp);

				cre->character = bufor[i]; ++i;
				bool isMesTre = bufor[i]; ++i; //true if there is message or treasury
				if(isMesTre)
				{
					cre->message = readString(bufor,i);
					cre->resources.resize(RESOURCE_QUANTITY);
					for(int j=0; j<7; j++)
					{
						cre->resources[j] = readNormalNr(bufor,i); i+=4;
					}

					int artID = readNormalNr(bufor,i, (version == RoE ? 1 : 2)); i+=(version == RoE ? 1 : 2);
					if(version==RoE)
					{
						if(artID!=0xff)
							cre->gainedArtifact = artID;
						else
							cre->gainedArtifact = -1;
					}
					else
					{
						if(artID!=0xffff)
							cre->gainedArtifact = artID;
						else
							cre->gainedArtifact = -1;
					}
				}
				cre->neverFlees = bufor[i]; ++i;
				cre->notGrowingTeam = bufor[i]; ++i;
				i+=2;;
				break;
			}
		case 59: case 91: //ocean bottle and sign
			{
				CGSignBottle *sb = new CGSignBottle();
				nobj = sb;
				sb->message = readString(bufor,i);
				i+=4;
				break;
			}
		case 83: //seer's hut
			{
				i = loadSeerHut(bufor, i, nobj);
				break;
			}
		case 113: //witch hut
			{
				CGWitchHut *wh = new CGWitchHut();
				nobj = wh;
				if(version>RoE) //in reo we cannot specify it - all are allowed (I hope)
				{
					int ist=i; //starting i for loop
					for(; i<ist+4; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < SKILL_QUANTITY)
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									wh->allowedAbilities.push_back((i-ist)*8+yy);
							}
						}
					}
				}
				else //(RoE map)
				{
					for(int gg=0; gg<SKILL_QUANTITY; ++gg)
					{
						wh->allowedAbilities.push_back(gg);
					}
				}
				break;
			}
		case 81: //scholar
			{
				CGScholar *sch = new CGScholar();
				nobj = sch;
				sch->bonusType = bufor[i++];
				sch->bonusID = bufor[i++];
				i+=6;
				break;
			}
		case 33: case 219: //garrison
			{
				CGGarrison *gar = new CGGarrison();
				nobj = gar;
				nobj->setOwner(bufor[i++]);
				i+=3;
				readCreatureSet(gar, bufor, i, 7, version > RoE);
				if(version > RoE)
				{
					gar->removableUnits = bufor[i]; ++i;
				}
				else
					gar->removableUnits = true;
				i+=8;
				break;
			}
		case 5: //artifact	
		case 65: case 66: case 67: case 68: case 69: //random artifact
		case 93: //spell scroll
			{
				int artID = -1;
				int spellID = -1;
				CGArtifact *art = new CGArtifact();
				nobj = art;

				bool areSettings = bufor[i++];
				if(areSettings)
				{
					art->message = readString(bufor,i);
					bool areGuards = bufor[i++];
					if(areGuards)
					{
						readCreatureSet(art, bufor, i, 7, version > RoE);
					}
					i+=4;
				}

				if(defInfo->id==93)
				{
					spellID = readNormalNr(bufor,i); i+=4;
					artID = 1;
				}
				else if(defInfo->id == 5) //specific artifact
				{
					artID = defInfo->subid;
				}

				art->storedArtifact = createArt(artID, spellID);
				break;
			}
		case 76: case 79: //random resource; resource
			{
				CGResource *res = new CGResource();
				nobj = res;

				bool isMessGuard = bufor[i]; ++i;
				if(isMessGuard)
				{
					res->message = readString(bufor,i);
					if(bufor[i++])
					{
						readCreatureSet(res, bufor, i, 7, version > RoE);
					}
					i+=4;
				}
				res->amount = readNormalNr(bufor,i); i+=4;
				if (defInfo->subid == 6) // Gold is multiplied by 100.
					res->amount *= 100;
				i+=4;

				break;
			}
		case 77: case 98: //random town; town
			{
				loadTown(nobj, bufor, i, defInfo->subid);
				break;
			}
		case 53: 
		case 220://mine (?)
			{
				nobj = new CGMine();
				nobj->setOwner(bufor[i++]);
				i+=3;
				break;
			}
		case 17: case 18: case 19: case 20: //dwellings
			{
				nobj = new CGDwelling();
				nobj->setOwner(bufor[i++]);
				i+=3;
				break;
			}
		case 78: //Refugee Camp
		case 106: //War Machine Factory
			{
				nobj = new CGDwelling();
				break;
			}
		case 88: case 89: case 90: //spell shrine
			{
				CGShrine * shr = new CGShrine();
				nobj = shr;
				shr->spell = bufor[i]; i+=4;
				break;
			}
		case 6: //pandora's box
			{
				CGPandoraBox *box = new CGPandoraBox();
				nobj = box;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					box->message = readString(bufor,i);
					if(bufor[i++])
					{
						readCreatureSet(box, bufor, i, 7, version > RoE);
					}
					i+=4;
				}

				box->gainedExp = readNormalNr(bufor,i, 4); i+=4;
				box->manaDiff = readNormalNr(bufor,i, 4); i+=4;
				box->moraleDiff = readNormalNr(bufor,i, 1, true); ++i;
				box->luckDiff = readNormalNr(bufor,i, 1, true); ++i;				
				
				box->resources.resize(RESOURCE_QUANTITY);
				for(int x=0; x<7; x++)
				{
					box->resources[x] = readNormalNr(bufor,i); 
					i+=4;
				}

				box->primskills.resize(PRIMARY_SKILLS);
				for(int x=0; x<4; x++)
				{
					box->primskills[x] = readNormalNr(bufor,i, 1); 
					i++;
				}

				int gabn; //number of gained abilities
				gabn = readNormalNr(bufor,i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					box->abilities.push_back(readNormalNr(bufor,i, 1)); ++i;
					box->abilityLevels.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gart = readNormalNr(bufor,i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					if(version > RoE)
					{
						box->artifacts.push_back(readNormalNr(bufor,i, 2)); i+=2;
					}
					else
					{
						box->artifacts.push_back(readNormalNr(bufor,i, 1)); i+=1;
					}
				}
				int gspel = readNormalNr(bufor,i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					box->spells.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gcre = readNormalNr(bufor,i, 1); ++i; //number of gained creatures
				readCreatureSet(&box->creatures, bufor,i,gcre,(version>RoE));
				i+=8;
				break;
			}
		case 36: //grail
			{
				grailPos = pos;
				grailRadious = readNormalNr(bufor,i); i+=4;
				continue;
			}
		case 217:
			{
				nobj = new CGDwelling();
				CCreGenObjInfo * spec = new CCreGenObjInfo;
				spec->player = readNormalNr(bufor,i); i+=4;
				spec->identifier =  readNormalNr(bufor,i); i+=4;
				if(!spec->identifier)
				{
					spec->asCastle = false;
					spec->castles[0] = bufor[i]; ++i;
					spec->castles[1] = bufor[i]; ++i;
				}
				else
				{
					spec->asCastle = true;
				}
				nobj->setOwner(spec->player);
				static_cast<CGDwelling*>(nobj)->info = spec;
				break;
			}
		case 216:
			{
				nobj = new CGDwelling();
				CCreGen2ObjInfo * spec = new CCreGen2ObjInfo;
				spec->player = readNormalNr(bufor,i); i+=4;
				spec->identifier =  readNormalNr(bufor,i); i+=4;
				if(!spec->identifier)
				{
					spec->asCastle = false;
					spec->castles[0] = bufor[i]; ++i;
					spec->castles[1] = bufor[i]; ++i;
				}
				else
				{
					spec->asCastle = true;
				}
				spec->minLevel = bufor[i]; ++i;
				spec->maxLevel = bufor[i]; ++i;
				nobj->setOwner(spec->player);
				static_cast<CGDwelling*>(nobj)->info = spec;
				break;
			}
		case 218:
			{
				nobj = new CGDwelling();
				CCreGen3ObjInfo * spec = new CCreGen3ObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->minLevel = bufor[i]; ++i;
				spec->maxLevel = bufor[i]; ++i;
				if(spec->maxLevel>7)
					spec->maxLevel = 7;
				if(spec->minLevel<1)
					spec->minLevel = 1;
				nobj->setOwner(spec->player);
				static_cast<CGDwelling*>(nobj)->info = spec;
				break;
			}
		case 215:
			{
				CGQuestGuard *guard = new CGQuestGuard();
				nobj = guard;
				loadQuest(guard, bufor, i);
				break;
			}
		case 28: //faerie ring
		case 14: //Swan pond
		case 38: //idol of fortune
		case 30: //Fountain of Fortune
		case 64: //Rally Flag
		case 56: //oasis
		case 96: //temple
		case 110://Watering Hole
		case 31: //Fountain of Youth
		case 11: //Buoy
		case 52: //Mermaid
		case 94: //Stables
			{
				nobj = new CGBonusingObject();
				break;
			}
		case 49: //Magic Well
			{
				nobj = new CGMagicWell();
				break;
			}
		case 15: //Cover of darkness
		case 58: //Redwood Observatory
		case 60: //Pillar of Fire
			{
				nobj = new CGObservatory();
				break;
			}
		case 22: //Corpse
		case 39: //Lean To
		case 105://Wagon
		case 108://Warrior's Tomb
			{
				nobj = new CGOnceVisitable();
				break;
			}
		case 8: //Boat
			{
				nobj = new CGBoat();
				break;
			}
		case 92: //Sirens
			{
				nobj = new CGSirens();
				break;
			}
		case 87: //Shipyard
			{
				nobj = new CGShipyard();
				nobj->setOwner(readNormalNr(bufor,i)); i+=4;
				break;
			}
		case 214: //hero placeholder
			{
				CGHeroPlaceholder *hp = new CGHeroPlaceholder();;
				nobj = hp;

				int a = bufor[i++]; //unkown byte, seems to be always 0 (if not - scream!)
				tlog2 << "Unhandled Hero Placeholder detected\n";

				int htid = bufor[i++]; //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
					hp->power = bufor[i++];
				else
					hp->power = 0;

				break;
			}
		case 10: //Keymaster
			{
				nobj = new CGKeymasterTent();
				break;
			}
		case 9: //Border Guard
			{
				nobj = new CGBorderGuard();
				break;
			}
		case 212: //Border Gate
			{
				nobj = new CGBorderGate();
				break;
			}
		case 27: case 37: //Eye and Hut of Magi
			{
				nobj = new CGMagi();
				break;
			}
		case 16: case 24: case 25: case 84: case 85: //treasure bank
			{
				nobj = new CBank();
				break;
			}
		case 63: //Pyramid
			{
				nobj = new CGPyramid();
				break;
			}
		case 13: //Cartographer
			{
				nobj = new CCartographer();
				break;
			}
		case 48: //Magic Spring
			{
				nobj = new CGMagicSpring();
				break;
			}
		case 97: //den of thieves
			{
				nobj = new CGDenOfthieves();
				break;
			}
		case 57: //Obelisk
			{
				nobj = new CGObelisk();
				break;
			}
		case 42: //Lighthouse
			{
				nobj = new CGLighthouse();
				nobj->tempOwner = readNormalNr(bufor,i); i+=4;
				break;
			}
		case 2: //Altar of Sacrifice
		case 99: //Trading Post
		case 213: //Freelancer's Guild
		case 221: //Trading Post (snow)
			{
				nobj = new CGMarket();
				break;
			}
		case 104: //University
			{
				nobj = new CGUniversity();
				break;
			}
		case 7: //Black Market
			{
				nobj = new CGBlackMarket();
				break;
			}

		default: //any other object
			{
				nobj = new CGObjectInstance();
				break;
			}

		} //end of main switch

		nobj->pos = pos;
		nobj->ID = defInfo->id;
		nobj->id = idToBeGiven;
		if(nobj->ID != HEROI_TYPE && nobj->ID != 214 && nobj->ID != 62)
			nobj->subID = defInfo->subid;
		nobj->defInfo = defInfo;
		assert(idToBeGiven == objects.size());
		objects.push_back(nobj);
		if(nobj->ID==TOWNI_TYPE)
			towns.push_back(static_cast<CGTownInstance*>(nobj));
		if(nobj->ID==HEROI_TYPE)
			heroes.push_back(static_cast<CGHeroInstance*>(nobj));
	}

	std::sort(heroes.begin(), heroes.end(), _HERO_SORTER());
}

void Mapa::readEvents( const unsigned char * bufor, int &i )
{
	int numberOfEvents = readNormalNr(bufor,i); i+=4;
	for(int yyoo=0; yyoo<numberOfEvents; ++yyoo)
	{
		CMapEvent *ne = new CMapEvent();
		ne->name = std::string();
		ne->message = std::string();
		int nameLen = readNormalNr(bufor,i); i+=4;
		for(int qq=0; qq<nameLen; ++qq)
		{
			ne->name += bufor[i]; ++i;
		}
		int messLen = readNormalNr(bufor,i); i+=4;
		for(int qq=0; qq<messLen; ++qq)
		{
			ne->message +=bufor[i]; ++i;
		}
		ne->resources.resize(RESOURCE_QUANTITY);
		for(int k=0; k < 7; k++)
		{
			ne->resources[k] = readNormalNr(bufor,i); i+=4;
		}
		ne->players = bufor[i]; ++i;
		if(version>AB)
		{
			ne->humanAffected = bufor[i]; ++i;
		}
		else
			ne->humanAffected = true;
		ne->computerAffected = bufor[i]; ++i;
		ne->firstOccurence = readNormalNr(bufor,i, 2); i+=2;
		ne->nextOccurence = bufor[i]; ++i;

		char unknown[17];
		memcpy(unknown, bufor+i, 17);
		i+=17;

		events.push_back(ne);
	}
}

bool Mapa::isInTheMap(const int3 &pos) const
{
	if(pos.x<0 || pos.y<0 || pos.z<0 || pos.x >= width || pos.y >= height || pos.z > twoLevel)
		return false;
	else return true;
}

void Mapa::loadQuest(CQuest * guard, const unsigned char * bufor, int & i)
{
	guard->missionType = bufor[i]; ++i;
	//int len1, len2, len3;
	switch(guard->missionType)
	{
	case 0:
		return;
	case 2:
		{
			guard->m2stats.resize(4);
			for(int x=0; x<4; x++)
			{
				guard->m2stats[x] = bufor[i++];
			}
		}
		break;
	case 1:
	case 3:
	case 4:
		{
			guard->m13489val = readNormalNr(bufor,i); i+=4;
			break;
		}
	case 5:
		{
			int artNumber = bufor[i]; ++i;
			for(int yy=0; yy<artNumber; ++yy)
			{
				int artid = readNormalNr(bufor,i, 2); i+=2;
				guard->m5arts.push_back(artid); 
				allowedArtifact[artid] = false; //these are unavaliable for random generation
			}
			break;
		}
	case 6:
		{
			int typeNumber = bufor[i]; ++i;
			guard->m6creatures.resize(typeNumber);
			for(int hh=0; hh<typeNumber; ++hh)
			{
				guard->m6creatures[hh].type = VLC->creh->creatures[readNormalNr(bufor,i, 2)]; i+=2;
				guard->m6creatures[hh].count = readNormalNr(bufor,i, 2); i+=2;
			}
			break;
		}
	case 7:
		{
			guard->m7resources.resize(7);
			for(int x=0; x<7; x++)
			{
				guard->m7resources[x] = readNormalNr(bufor,i); 
				i+=4;
			}
			break;
		}
	case 8:
	case 9:
		{
			guard->m13489val = bufor[i]; ++i;
			break;
		}
	}


	int limit = readNormalNr(bufor,i); i+=4;
	if(limit == ((int)0xffffffff))
	{
		guard->lastDay = -1;
	}
	else
	{
		guard->lastDay = limit;
	}
	guard->firstVisitText = readString(bufor,i);
	guard->nextVisitText = readString(bufor,i);
	guard->completedText = readString(bufor,i);
	if (guard->firstVisitText.size() && guard->nextVisitText.size() && guard->completedText.size())
		guard->isCustom = true;
	else
		guard->isCustom = false;  //randomize all if any text is missing
}

TerrainTile & Mapa::getTile( const int3 & tile )
{
	return terrain[tile.x][tile.y][tile.z];
}

const TerrainTile & Mapa::getTile( const int3 & tile ) const
{
	return terrain[tile.x][tile.y][tile.z];
}

bool Mapa::isWaterTile(const int3 &pos) const
{
	return isInTheMap(pos) && getTile(pos).tertype == TerrainTile::water;
}

const CGObjectInstance *Mapa::getObjectiveObjectFrom(int3 pos, bool lookForHero)
{
	const std::vector <CGObjectInstance*> &objs = getTile(pos).visitableObjects;
	assert(objs.size());
	if(objs.size() > 1 && lookForHero && objs.front()->ID != HEROI_TYPE)
	{
		assert(objs.back()->ID == HEROI_TYPE);
		return objs.back();
	}
	else
		return objs.front();
}

void Mapa::checkForObjectives()
{
	if(isInTheMap(victoryCondition.pos))
		victoryCondition.obj = getObjectiveObjectFrom(victoryCondition.pos, victoryCondition.condition == beatHero);

	if(isInTheMap(lossCondition.pos))
		lossCondition.obj = getObjectiveObjectFrom(lossCondition.pos, lossCondition.typeOfLossCon == lossHero);
}

void Mapa::addNewArtifactInstance( CArtifactInstance *art )
{
	art->id = artInstances.size();
	artInstances.push_back(art);
}

bool Mapa::loadArtifactToSlot(CGHeroInstance *h, int slot, const unsigned char * bufor, int &i)
{
	const int artmask = version == RoE ? 0xff : 0xffff;
	const int artidlen = version == RoE ? 1 : 2;

	int aid = readNormalNr(bufor,i, artidlen); i+=artidlen;
	bool isArt  =  aid != artmask;
	if(isArt)
	{
		if(vstd::contains(VLC->arth->bigArtifacts, aid) && slot >= Arts::BACKPACK_START)
		{
			tlog3 << "Warning: A big artifact (war machine) in hero's backpack, ignoring...\n";
			return false;
		}
		if(aid == 0 && slot == Arts::MISC5)
		{
			//TODO: check how H3 handles it -> art 0 in slot 18 in AB map
			tlog3 << "Spellbook to MISC5 slot? Putting it spellbook place. AB format peculiarity ? (format " << (int)version << ")\n";
			slot = Arts::SPELLBOOK;
		}
		
		h->putArtifact(slot, createArt(aid));
	}
	return isArt;
}

void Mapa::loadArtifactsOfHero(const unsigned char * bufor, int & i, CGHeroInstance * nhi)
{
	bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
	if(artSet)
	{
		for(int pom=0;pom<16;pom++)
			loadArtifactToSlot(nhi, pom, bufor, i);

		//misc5 art //17
		if(version >= SoD)
		{
			if(!loadArtifactToSlot(nhi, Arts::MACH4, bufor, i))
				nhi->putArtifact(Arts::MACH4, createArt(Arts::ID_CATAPULT)); //catapult by default
		}

		loadArtifactToSlot(nhi, Arts::SPELLBOOK, bufor, i);

		//19 //???what is that? gap in file or what? - it's probably fifth slot..
		if(version > RoE)
			loadArtifactToSlot(nhi, Arts::MISC5, bufor, i);
		else
			i+=1;

		//bag artifacts //20
		int amount = readNormalNr(bufor,i, 2); i+=2; //number of artifacts in hero's bag
		for(int ss = 0; ss < amount; ++ss)
			loadArtifactToSlot(nhi, Arts::BACKPACK_START + nhi->artifactsInBackpack.size(), bufor, i);
	} //artifacts
}

CArtifactInstance * Mapa::createArt(int aid, int spellID /*= -1*/)
{
	CArtifactInstance *a = NULL;
	if(aid >= 0)
	{
		if(spellID < 0)
			a = CArtifactInstance::createNewArtifactInstance(aid);
		else
			a = CArtifactInstance::createScroll(VLC->spellh->spells[spellID]);
	}
	else
		a = new CArtifactInstance();

	addNewArtifactInstance(a);
	if(a->artType && a->artType->constituents) //TODO make it nicer
	{
		CCombinedArtifactInstance *comb = dynamic_cast<CCombinedArtifactInstance*>(a);
		BOOST_FOREACH(CCombinedArtifactInstance::ConstituentInfo &ci, comb->constituentsInfo)
		{
			addNewArtifactInstance(ci.art);
		}
	}
	return a;
}

void Mapa::eraseArtifactInstance(CArtifactInstance *art)
{
	assert(artInstances[art->id] == art);
	artInstances[art->id].dellNull();
}

LossCondition::LossCondition()
{
	obj = NULL;
	timeLimit = -1;
	pos = int3(-1,-1,-1);
}

CVictoryCondition::CVictoryCondition()
{
	pos = int3(-1,-1,-1);
	obj = NULL;
	ID = allowNormalVictory = appliesToAI = count = 0;
}

bool TerrainTile::entrableTerrain(const TerrainTile *from /*= NULL*/) const
{
	return entrableTerrain(from ? from->tertype != water : true, from ? from->tertype == water : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	return tertype != rock 
		&& ((allowSea && tertype == water)  ||  (allowLand && tertype != water));
}

bool TerrainTile::isClear(const TerrainTile *from /*= NULL*/) const
{
	return entrableTerrain(from) && !blocked;
}
