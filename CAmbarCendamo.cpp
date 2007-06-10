#include "stdafx.h"
#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "CGameInfo.h"
#include <set>

unsigned int intPow(unsigned int a, unsigned int b)
{
	unsigned int ret=1;
	for(int i=0; i<b; ++i)
		ret*=a;
	return ret;
}

CAmbarCendamo::CAmbarCendamo (const char * tie)
{
	is = new std::ifstream();
	is -> open(tie,std::ios::binary);
	is->seekg(0,std::ios::end); // na koniec
	andame = is->tellg();  // read length
	is->seekg(0,std::ios::beg); // wracamy na poczatek
	bufor = new unsigned char[andame]; // allocate memory 
	is->read((char*)bufor, andame); // read map file to buffer
	delete is;
}
CAmbarCendamo::~CAmbarCendamo () 
{// free memory
	for (int ii=0;ii<map.width;ii++)
		delete map.terrain[ii] ; 
	delete map.terrain;
	delete bufor;
}
void CAmbarCendamo::teceDef()
{
	for (int i=0; i<map.defy.size(); i++)
	{
		std::ofstream * of = new std::ofstream(map.defy[i].name.c_str());
		for (int j=0;j<46;j++)
		{
			(*of) << map.defy[i].bytes[j]<<std::endl;
		}
		delete of;
	}
}
void CAmbarCendamo::deh3m()
{
	THC timeHandler th;
	map.version = (Eformat)bufor[0]; //wersja mapy
	map.areAnyPLayers = bufor[4];
	map.height = map.width = bufor[5]; // wymiary mapy
	map.twoLevel = bufor[9]; //czy sa lochy
	map.terrain = new TerrainTile*[map.width]; // allocate memory 
	for (int ii=0;ii<map.width;ii++)
		map.terrain[ii] = new TerrainTile[map.height]; // allocate memory 
	if (map.twoLevel)
	{
		map.undergroungTerrain = new TerrainTile*[map.width]; // allocate memory 
		for (int ii=0;ii<map.width;ii++)
			map.undergroungTerrain[ii] = new TerrainTile[map.height]; // allocate memory 
	}
	int length = bufor[10]; //name length
	int i=14, pom; 
	while (i-14<length)	//read name
		map.name+=bufor[i++];
	length = bufor[i] + bufor[i+1]*256; //description length
	i+=4;
	for (pom=0;pom<length;pom++)
		map.description+=bufor[i++];
	map.difficulty = bufor[i++]; // reading map difficulty
	map.levelLimit = bufor[i++]; // hero level limit
	for (pom=0;pom<8;pom++)
	{
		map.players[pom].canHumanPlay = bufor[i++];
		map.players[pom].canComputerPlay = bufor[i++];
		if ((!(map.players[pom].canHumanPlay || map.players[pom].canComputerPlay)) || (!map.areAnyPLayers))
		{
			i+=13;
			continue;
		}

		map.players[pom].AITactic = bufor[i++];
		if (bufor[i++])
		{
			map.players[pom].allowedFactions = 0;
			map.players[pom].allowedFactions += bufor[i++];
			map.players[pom].allowedFactions += (bufor[i++])*256;
		}
		else 
		{
			map.players[pom].allowedFactions = 511;
			i+=2;
		}
		map.players[pom].isFactionRandom = bufor[i++];
		map.players[pom].hasMainTown = bufor[i++];
		if (map.players[pom].hasMainTown)
		{
			map.players[pom].generateHeroAtMainTown = bufor[i++];
			i++; //unknown byte
			map.players[pom].posOfMainTown.x = bufor[i++];
			map.players[pom].posOfMainTown.y = bufor[i++];
			map.players[pom].posOfMainTown.z = bufor[i++];
		}
		i++; //unknown byte
		int unknown = bufor[i++];
		if (unknown == 255)
		{
			map.players[pom].mainHeroPortrait = 255;
			i+=5;
			continue;
		}
		map.players[pom].mainHeroPortrait = bufor[i++];
		int nameLength = bufor[i++];
		i+=3; 
		for (int pp=0;pp<nameLength;pp++)
			map.players[pom].mainHeroName+=bufor[i++];
		i++; ////unknown byte
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
			map.players[pom].heroesNames.push_back(vv);
		}
	}
	map.victoryCondition = (EvictoryConditions)bufor[i++];
	if (map.victoryCondition != winStandard) //specific victory conditions
	{
		int nr;
		switch (map.victoryCondition) //read victory conditions
		{
		case artifact:
			{
				map.vicConDetails = new VicCon0();
				((VicCon0*)map.vicConDetails)->ArtifactID = bufor[i+2];
				nr=2;
				break;
			}
		case gatherTroop:
			{
				map.vicConDetails = new VicCon1();
				int temp1 = bufor[i+2];
				int temp2 = bufor[i+3];
				((VicCon1*)map.vicConDetails)->monsterID = bufor[i+2];
				((VicCon1*)map.vicConDetails)->neededQuantity=readNormalNr(i+4);
				nr=6;
				break;
			}
		case gatherResource:
			{
				map.vicConDetails = new VicCon2();
				((VicCon2*)map.vicConDetails)->resourceID = bufor[i+2];
				((VicCon2*)map.vicConDetails)->neededQuantity=readNormalNr(i+3);
				nr=5;
				break;
			}
		case buildCity:
			{
				map.vicConDetails = new VicCon3();
				((VicCon3*)map.vicConDetails)->posOfCity.x = bufor[i+2];
				((VicCon3*)map.vicConDetails)->posOfCity.y = bufor[i+3];
				((VicCon3*)map.vicConDetails)->posOfCity.z = bufor[i+4];
				((VicCon3*)map.vicConDetails)->councilNeededLevel = bufor[i+5];
				((VicCon3*)map.vicConDetails)->fortNeededLevel = bufor[i+6];
				nr=5;
				break;
			}
		case buildGrail:
			{
				map.vicConDetails = new VicCon4();
				if (bufor[i+4]>2)
					((VicCon4*)map.vicConDetails)->anyLocation = true;
				else
				{
					((VicCon4*)map.vicConDetails)->whereBuildGrail.x = bufor[i+2];
					((VicCon4*)map.vicConDetails)->whereBuildGrail.y = bufor[i+3];
					((VicCon4*)map.vicConDetails)->whereBuildGrail.z = bufor[i+4];
				}
				nr=3;
				break;
			}
		case beatHero:
			{
				map.vicConDetails = new VicCon5();
				((VicCon5*)map.vicConDetails)->locationOfHero.x = bufor[i+2];
				((VicCon5*)map.vicConDetails)->locationOfHero.y = bufor[i+3];
				((VicCon5*)map.vicConDetails)->locationOfHero.z = bufor[i+4];				
				nr=3;
				break;
			}
		case captureCity:
			{
				map.vicConDetails = new VicCon6();
				((VicCon6*)map.vicConDetails)->locationOfTown.x = bufor[i+2];
				((VicCon6*)map.vicConDetails)->locationOfTown.y = bufor[i+3];
				((VicCon6*)map.vicConDetails)->locationOfTown.z = bufor[i+4];				
				nr=3;
				break;
			}
		case beatMonster:
			{
				map.vicConDetails = new VicCon7();
				((VicCon7*)map.vicConDetails)->locationOfMonster.x = bufor[i+2];
				((VicCon7*)map.vicConDetails)->locationOfMonster.y = bufor[i+3];
				((VicCon7*)map.vicConDetails)->locationOfMonster.z = bufor[i+4];				
				nr=3;
				break;
			}
		case takeDwellings:
			{		
				map.vicConDetails = new CspecificVictoryConidtions();
				nr=3;
				break;
			}
		case takeMines:
			{	
				map.vicConDetails = new CspecificVictoryConidtions();	
				nr=3;
				break;
			}
		case transportItem:
			{
				map.vicConDetails = new VicCona();
				((VicCona*)map.vicConDetails)->artifactID =  bufor[i+2];
				((VicCona*)map.vicConDetails)->destinationPlace.x = bufor[i+3];
				((VicCona*)map.vicConDetails)->destinationPlace.y = bufor[i+4];
				((VicCona*)map.vicConDetails)->destinationPlace.z = bufor[i+5];				
				nr=3;
				break;
			}
		}
		map.vicConDetails->allowNormalVictory = bufor[i++];
		map.vicConDetails->appliesToAI = bufor[i++];
		i+=nr;
	}
	map.lossCondition.typeOfLossCon = (ElossCon)bufor[i++];
	switch (map.lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
		  {
			  map.lossCondition.castlePos.x=bufor[i++];
			  map.lossCondition.castlePos.y=bufor[i++];
			  map.lossCondition.castlePos.z=bufor[i++];
		  }
	case lossHero:
		  {
			  map.lossCondition.heroPos.x=bufor[i++];
			  map.lossCondition.heroPos.y=bufor[i++];
			  map.lossCondition.heroPos.z=bufor[i++];
		  }
	case timeExpires:
		{
			map.lossCondition.timeLimit = readNormalNr(i++,2);
			i++;
		}
	}
	map.howManyTeams=bufor[i++]; //read number of teams
	if(map.howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
			map.players[rr].team=bufor[i++];
		}
	}
	//reading allowed heroes (20 bytes)
	int ist=i; //starting i for loop
	for(i; i<ist+20; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->heroh->heroes.size())
			{
				if(c == (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy].isAllowed = true;
				else
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy].isAllowed = false;
			}
		}
	}
	//allowed heroes have been read
	i+=36;
	//reading allowed artifacts //18 bytes
	ist=i; //starting i for loop
	for(i; i<ist+18; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->arth->artifacts.size())
			{
				if(c != (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->arth->artifacts[(i-ist)*8+yy].isAllowed = true;
				else
					CGameInfo::mainObj->arth->artifacts[(i-ist)*8+yy].isAllowed = false;
			}
		}
	}
	//allowed artifacts have been read
	//reading allowed spells (9 bytes)
	ist=i; //starting i for loop
	for(i; i<ist+9; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
			{
				if(c != (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy].isAllowed = true;
				else
					CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy].isAllowed = false;
			}
		}
	}
	//allowed spells have been read
	//allowed hero's abilities (4 bytes)
	ist=i; //starting i for loop
	for(i; i<ist+4; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->abilh->abilities.size())
			{
				if(c != (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy].isAllowed = true;
				else
					CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy].isAllowed = false;
			}
		}
	}
	//allowed hero's abilities have been read

	THC std::cout<<"Wczytywanie naglowka: "<<th.getDif()<<std::endl;
	int rumNr = readNormalNr(i,4);i+=4;
	for (int it=0;it<rumNr;it++)
	{
		Rumor ourRumor;
		int nameL = readNormalNr(i,4);i+=4; //read length of name of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.name+=bufor[i++];
		nameL = readNormalNr(i,4);i+=4; //read length of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.text+=bufor[i++];
		map.rumors.push_back(ourRumor); //add to our list
	}
	THC std::cout<<"Wczytywanie plotek: "<<th.getDif()<<std::endl;
	i+=156;
	for (int c=0; c<map.width; c++) // reading terrain
	{
		for (int z=0; z<map.height; z++)
		{
			map.terrain[z][c].tertype = (EterrainType)(bufor[i++]);
			map.terrain[z][c].terview = bufor[i++];
			map.terrain[z][c].nuine = (Eriver)bufor[i++];
			map.terrain[z][c].rivDir = bufor[i++];
			map.terrain[z][c].malle = (Eroad)bufor[i++];
			map.terrain[z][c].roadDir = bufor[i++];
			map.terrain[z][c].siodmyTajemniczyBajt = bufor[i++];
		}
	}
	if (map.twoLevel) // read underground terrain
	{
		for (int c=0; c<map.width; c++) // reading terrain
		{
			for (int z=0; z<map.height; z++)
			{
				map.undergroungTerrain[z][c].tertype = (EterrainType)(bufor[i++]);
				map.undergroungTerrain[z][c].terview = bufor[i++];
				map.undergroungTerrain[z][c].nuine = (Eriver)bufor[i++];
				map.undergroungTerrain[z][c].rivDir = bufor[i++];
				map.undergroungTerrain[z][c].malle = (Eroad)bufor[i++];
				map.undergroungTerrain[z][c].roadDir = bufor[i++];
				map.undergroungTerrain[z][c].siodmyTajemniczyBajt = bufor[i++];
			}
		}
	}
	THC std::cout<<"Wczytywanie terenu: "<<th.getDif()<<std::endl;
	int defAmount = bufor[i]; // liczba defow
	i+=4;
	for (int idd = 0 ; idd<defAmount; idd++) // reading defs
	{
		int nameLength = readNormalNr(i,4);i+=4;
		DefInfo vinya; // info about new def
		for (int cd=0;cd<nameLength;cd++)
		{
			vinya.name += bufor[i++];
		}
		for (int v=0; v<42; v++) // read info
		{
			vinya.bytes[v] = bufor[i++];
		}
		map.defy.push_back(vinya); // add this def to the vector
		//teceDef();
	}
	THC std::cout<<"Wczytywanie defow: "<<th.getDif()<<std::endl;
	//todo: read events
}
int CAmbarCendamo::readNormalNr (int pos, int bytCon)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[pos+i]*amp;
		amp*=256;
	}

	return ret;
}
void CAmbarCendamo::loadDefs()
{
	std::set<int> loadedTypes;
	for (int i=0; i<map.width; i++)
	{
		for (int j=0; j<map.width; j++)
		{
			if (loadedTypes.find(map.terrain[i][j].tertype)==loadedTypes.end())
			{
				CSemiDefHandler  *sdh = new CSemiDefHandler();
				sdh->openDef((sdh->nameFromType(map.terrain[i][j].tertype)).c_str(),"H3sprite.lod");
				loadedTypes.insert(map.terrain[i][j].tertype);
				defs.push_back(sdh);
			}
			if (loadedTypes.find(map.undergroungTerrain[i][j].tertype)==loadedTypes.end())
			{
				CSemiDefHandler  *sdh = new CSemiDefHandler();
				sdh->openDef((sdh->nameFromType(map.undergroungTerrain[i][j].tertype)).c_str(),"H3sprite.lod");
				loadedTypes.insert(map.undergroungTerrain[i][j].tertype);
				defs.push_back(sdh);
			}
		}
	}
};