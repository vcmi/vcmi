#include "../stdafx.h"
#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "../CGameInfo.h"
#include "CObjectHandler.h"
#include "CCastleHandler.h"
#include "CTownHandler.h"
#include "CDefObjInfoHandler.h"
#include "../SDL_Extensions.h"
#include "boost\filesystem.hpp"
#include "../CGameState.h"
#include "CLodHandler.h"
#include <set>
#include <iomanip>
#include <sstream>
#include "../CLua.h"

unsigned int intPow(unsigned int a, unsigned int b)
{
	unsigned int ret=1;
	for(int i=0; i<b; ++i)
		ret*=a;
	return ret;
}
unsigned char reverse(unsigned char arg)
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
CAmbarCendamo::CAmbarCendamo (unsigned char * map)
{
	bufor=map;
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
	is->close();
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
	//for (int i=0; i<map.defy.size(); i++)
	//{
	//	std::ofstream * of = new std::ofstream(map.defy[i].name.c_str());
	//	for (int j=0;j<46;j++)
	//	{
	//		(*of) << map.defy[i].bytes[j]<<std::endl;
	//	}
	//	of->close();
	//	delete of;
	//}
}
void CAmbarCendamo::deh3m()
{
	THC timeHandler th;
	map.version = (Eformat)bufor[0]; //wersja mapy
	map.areAnyPLayers = bufor[4]; //invalid on some maps
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
	if(map.version != Eformat::RoE)
	{
		map.levelLimit = bufor[i++]; // hero level limit
	}
	else
	{
		map.levelLimit = 0;
	}
	for (pom=0;pom<8;pom++)
	{
		map.players[pom].canHumanPlay = bufor[i++];
		map.players[pom].canComputerPlay = bufor[i++];
		if ((!(map.players[pom].canHumanPlay || map.players[pom].canComputerPlay)))
		{
			switch(map.version)
			{
			case Eformat::SoD: case Eformat::WoG: 
				i+=13;
				break;
			case Eformat::AB:
				i+=12;
				break;
			case Eformat::RoE:
				i+=6;
				break;
			}
			continue;
		}

		map.players[pom].AITactic = bufor[i++];

		if(map.version == Eformat::SoD || map.version == Eformat::WoG)
			i++;

		map.players[pom].allowedFactions = 0;
		map.players[pom].allowedFactions += bufor[i++];
		if(map.version != Eformat::RoE)
			map.players[pom].allowedFactions += (bufor[i++])*256;

		map.players[pom].isFactionRandom = bufor[i++];
		map.players[pom].hasMainTown = bufor[i++];
		if (map.players[pom].hasMainTown)
		{
			if(map.version != Eformat::RoE)
			{
				map.players[pom].generateHeroAtMainTown = bufor[i++];
				map.players[pom].generateHero = bufor[i++];
			}
			map.players[pom].posOfMainTown.x = bufor[i++];
			map.players[pom].posOfMainTown.y = bufor[i++];
			map.players[pom].posOfMainTown.z = bufor[i++];

			
		}
		map.players[pom].p8= bufor[i++];
		map.players[pom].p9= bufor[i++];		
		if(map.players[pom].p9!=0xff)
		{
			map.players[pom].mainHeroPortrait = bufor[i++];
			int nameLength = bufor[i++];
			i+=3; 
			for (int pp=0;pp<nameLength;pp++)
				map.players[pom].mainHeroName+=bufor[i++];
		}

		if(map.version != Eformat::RoE)
		{
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
				nr=(map.version==RoE ? 1 : 2);
				break;
			}
		case gatherTroop:
			{
				map.vicConDetails = new VicCon1();
				int temp1 = bufor[i+2];
				int temp2 = bufor[i+3];
				((VicCon1*)map.vicConDetails)->monsterID = bufor[i+2];
				((VicCon1*)map.vicConDetails)->neededQuantity=readNormalNr(i+(map.version==RoE ? 3 : 4));
				nr=(map.version==RoE ? 5 : 6);
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
				nr=0;
				break;
			}
		case takeMines:
			{	
				map.vicConDetails = new CspecificVictoryConidtions();	
				nr=0;
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
			  break;
		  }
	case lossHero:
		  {
			  map.lossCondition.heroPos.x=bufor[i++];
			  map.lossCondition.heroPos.y=bufor[i++];
			  map.lossCondition.heroPos.z=bufor[i++];
			  break;
		  }
	case timeExpires:
		{
			map.lossCondition.timeLimit = readNormalNr(i++,2);
			i++;
			 break;
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
	int ist;

	ist=i; //starting i for loop
	for(i; i<ist+ (map.version == Eformat::RoE ? 16 : 20) ; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->heroh->heroes.size())
			{
				if(c == (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy]->isAllowed = true;
				else
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy]->isAllowed = false;
			}
		}
	}
	if(map.version>RoE)
		i+=4;
	unsigned char disp = 0;
	if(map.version>=SoD)
	{
		disp = bufor[i++];
		for(int g=0; g<disp; ++g)
		{
			i+=2;
			int lenbuf = readNormalNr(i); i+=4;
			i+=lenbuf+1;
		}
	}
	//allowed heroes have been read
	unsigned char aaa1=bufor[i], aaa2=bufor[i+1], aaa3=bufor[i+2];
	i+=31; //omitting 0s
	//reading allowed artifacts //18 bytes
	if(map.version!=RoE)
	{
		ist=i; //starting i for loop
		for(i; i<ist+(map.version==AB ? 17 : 18); ++i)
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
		}//allowed artifacts have been read
	}

	//reading allowed spells (9 bytes)
	if(map.version>=SoD)
	{
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
						CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]->isAllowed = true;
					else
						CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]->isAllowed = false;
				}
			}
		}
	}
	//allowed hero's abilities have been read

	THC std::cout<<"Reading header: "<<th.getDif()<<std::endl;
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
	THC std::cout<<"Reading rumors: "<<th.getDif()<<std::endl;
	switch(map.version)
	{
	case WoG: case SoD: case AB:
		if(bufor[i]=='\0') //omit 156 bytes of rubbish
		{
			if(map.version>AB)
				i+=156;
			break;
		}
		else if(map.version!=AB || map.rumors.size()==0) //omit a lot of rubbish in a strage way
		{
			int lastFFpos=i;
			while(i-lastFFpos<200) //i far in terrain bytes
			{
				++i;
				if(bufor[i]==0xff)
				{
					lastFFpos=i;
				}
			}

			i=lastFFpos;

			while(bufor[i-1]!=0 || bufor[i]>8 || bufor[i+2]>4 || bufor[i+1]==0) //back to terrain bytes
			{
				i++;
			}
		}
		break;
	case RoE:
		i+=0;
		break;
	}
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
	THC std::cout<<"Reading terrain: "<<th.getDif()<<std::endl;
	int defAmount = bufor[i]; // liczba defow
	defAmount = readNormalNr(i);
	i+=4;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int srmask = 0xff000000;
    int sgmask = 0x00ff0000;
    int sbmask = 0x0000ff00;
    int samask = 0x000000ff;
#else
    int srmask = 0x000000ff;
    int sgmask = 0x0000ff00;
    int sbmask = 0x00ff0000;
    int samask = 0xff000000;
#endif
	SDL_Surface * alphaTransSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 12, 12, 32, srmask, sgmask, sbmask, samask);
	std::vector<std::string> defsToUnpack;
	for (int idd = 0 ; idd<defAmount; idd++) // reading defs
	{
		int nameLength = readNormalNr(i,4);i+=4;
		CGDefInfo * vinya = new CGDefInfo(); // info about new def
		for (int cd=0;cd<nameLength;cd++)
		{
			vinya->name += bufor[i++];
		}
		//for (int v=0; v<42; v++) // read info
		//{
		//	vinya->bytes[v] = bufor[i++];
		//}
		std::transform(vinya->name.begin(),vinya->name.end(),vinya->name.begin(),(int(*)(int))toupper);
		std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
			vinya->name);
		if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
		{
			vinya->isOnDefList = false;
		}
		else
		{
			//vinya->printPriority = pit->priority;
			vinya->isOnDefList = true;
		}
		unsigned char bytes[12];
		for (int v=0; v<12; v++) // read info
		{
			bytes[v] = bufor[i++];
		}
		vinya->terrainAllowed = readNormalNr(i,2);i+=2;
		vinya->terrainMenu = readNormalNr(i,2);i+=2;
		vinya->id = readNormalNr(i,4);i+=4;
		vinya->subid = readNormalNr(i,4);i+=4;
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
		map.defy.push_back(vinya); // add this def to the vector
		defsToUnpack.push_back(vinya->name);
	}
	THC std::cout<<"Reading defs: "<<th.getDif()<<std::endl;
	////loading objects
	int howManyObjs = readNormalNr(i, 4); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		//std::cout << "object nr "<<ww<<"\ti= "<<i<<std::endl;
		CGObjectInstance * nobj = new CGObjectInstance(); //we will read this object
		nobj->id = CGameInfo::mainObj->objh->objInstances.size();
		nobj->pos.x = bufor[i++];
		nobj->pos.y = bufor[i++];
		nobj->pos.z = bufor[i++];

		int tempd = readNormalNr(i, 4); i+=4;
		nobj->defInfo = map.defy[tempd];
		nobj->ID = nobj->defInfo->id;
		nobj->subID = nobj->defInfo->subid;
		//nobj->defInfo = readNormalNr(i, 4); i+=4;
		//nobj->defObjInfoNumber = -1;
		//nobj->isHero = false;
		//nobj->moveDir = 0;
		//nobj->isStanding = true;
		//nobj->state->owner = 254; //a lot of objs will never have an owner

		//if (((nobj.x==0)&&(nobj.y==0)) || nobj.x>map.width || nobj.y>map.height || nobj.z>1 || nobj.defNumber>map.defy.size())
		//	std::cout << "Alarm!!! Obiekt "<<ww<<" jest kopniety (lub wystaje poza mape)\n";

		i+=5;
		unsigned char buff [30];
		for(int ccc=0; ccc<30; ++ccc)
		{
			buff[ccc] = bufor[i+ccc];
		}
		EDefType uu = getDefType(nobj->defInfo);
		int j = nobj->defInfo->id;
		int p = 99;
		switch(uu)
		{
		case EDefType::EVENTOBJ_DEF: //for event - objects
			{
				CEventObjInfo * spec = new CEventObjInfo;
				bool guardMess;
				guardMess = bufor[i]; ++i;
				if(guardMess)
				{
					int messLong = readNormalNr(i, 4); i+=4;
					if(messLong>0)
					{
						spec->isMessage = true;
						for(int yy=0; yy<messLong; ++yy)
						{
							spec->message +=bufor[i+yy];
						}
						i+=messLong;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+=( map.version == RoE ? 21 : 28);
					}
					i+=4;
				}
				else
				{
					spec->isMessage = false;
					spec->areGuarders = false;
					spec->message = std::string("");
				}
				spec->gainedExp = readNormalNr(i, 4); i+=4;
				spec->manaDiff = readNormalNr(i, 4); i+=4;
				spec->moraleDiff = readNormalNr(i, 1, true); ++i;
				spec->luckDiff = readNormalNr(i, 1, true); ++i;
				spec->wood = readNormalNr(i); i+=4;
				spec->mercury = readNormalNr(i); i+=4;
				spec->ore = readNormalNr(i); i+=4;
				spec->sulfur = readNormalNr(i); i+=4;
				spec->crystal = readNormalNr(i); i+=4;
				spec->gems = readNormalNr(i); i+=4;
				spec->gold = readNormalNr(i); i+=4;
				spec->attack = readNormalNr(i, 1); ++i;
				spec->defence = readNormalNr(i, 1); ++i;
				spec->power = readNormalNr(i, 1); ++i;
				spec->knowledge = readNormalNr(i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)]); ++i;
					spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
				}
				int gart = readNormalNr(i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[readNormalNr(i, (map.version == RoE ? 1 : 2))])); i+=(map.version == RoE ? 1 : 2);
				}
				int gspel = readNormalNr(i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[readNormalNr(i, 1)])); ++i;
				}
				int gcre = readNormalNr(i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(i, gcre); i+=4*gcre;
				i+=8;
				spec->availableFor = readNormalNr(i, 1); ++i;
				spec->computerActivate = readNormalNr(i, 1); ++i;
				spec->humanActivate = readNormalNr(i, 1); ++i;
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::HERO_DEF:
			{
				CHeroObjInfo * spec = new CHeroObjInfo;
				if(map.version>RoE)
				{
					spec->bytes[0] = bufor[i]; ++i;
					spec->bytes[1] = bufor[i]; ++i;
					spec->bytes[2] = bufor[i]; ++i;
					spec->bytes[3] = bufor[i]; ++i;
				}
				spec->player = bufor[i]; ++i;
				int typeBuf = readNormalNr(i, 1); ++i;
				if(typeBuf==0xff)
					spec->type = NULL;
				else
					spec->type = CGameInfo::mainObj->heroh->heroes[typeBuf];
				bool isName = bufor[i]; ++i; //true if hero has nonstandard name
				if(isName)
				{
					int length = readNormalNr(i, 4); i+=4;
					for(int gg=0; gg<length; ++gg)
					{
						spec->name+=bufor[i]; ++i;
					}
				}
				else
					spec->name = std::string("");
				if(map.version>AB)
				{
					bool isExp = bufor[i]; ++i; //true if hore's experience is greater than 0
					if(isExp)
					{
						spec->experience = readNormalNr(i); i+=4;
					}
					else
						spec->experience = 0;
				}
				else
				{
					spec->experience = readNormalNr(i); i+=4;
				}
				bool portrait=bufor[i]; ++i;
				if (portrait)
					i++; //TODO read portrait nr, save, open
				bool nonstandardAbilities = bufor[i]; ++i; //true if hero has specified abilities
				if(nonstandardAbilities)
				{
					int howMany = readNormalNr(i); i+=4;
					for(int yy=0; yy<howMany; ++yy)
					{
						spec->abilities.push_back(CGameInfo::mainObj->abilh->abilities[readNormalNr(i, 1)]); ++i;
						spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
					}
				}
				bool standGarrison = bufor[i]; ++i; //true if hero has nonstandard garrison
				spec->standardGarrison = standGarrison;
				if(standGarrison)
				{
					spec->garrison = readCreatureSet(i); i+= (map.version == RoE ? 21 : 28); //4 bytes per slot
				}
				bool form = bufor[i]; ++i; //formation
				spec->garrison.formation = form;
				bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
				int artmask = map.version == RoE ? 0xff : 0xffff;
				int artidlen = map.version == RoE ? 1 : 2;
				if(artSet)
				{
					//head art //1
					int id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artHead = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artHead = NULL;
					//shoulders art //2
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artShoulders = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artShoulders = NULL;
					//neck art //3
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artNeck = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artNeck = NULL;
					//right hand art //4
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artRhand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRhand = NULL;
					//left hand art //5
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artLHand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLHand = NULL;
					//torso art //6
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artTorso = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artTorso = NULL;
					//right hand ring //7
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artRRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRRing = NULL;
					//left hand ring //8
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artLRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLRing = NULL;
					//feet art //9
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artFeet = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artFeet = NULL;
					//misc1 art //10
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMisc1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc1 = NULL;
					//misc2 art //11
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMisc2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc2 = NULL;
					//misc3 art //12
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMisc3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc3 = NULL;
					//misc4 art //13
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMisc4 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc4 = NULL;
					//machine1 art //14
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMach1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach1 = NULL;
					//machine2 art //15
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMach2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach2 = NULL;
					//machine3 art //16
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artMach3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach3 = NULL;
					//misc5 art //17
					if(map.version>=SoD)
					{
						id = readNormalNr(i, artidlen); i+=artidlen;
						if(id!=artmask)
							spec->artMisc5 = &(CGameInfo::mainObj->arth->artifacts[id]);
						else
							spec->artMisc5 = NULL;
					}
					//spellbook
					id = readNormalNr(i, artidlen); i+=artidlen;
					if(id!=artmask)
						spec->artSpellBook = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artSpellBook = NULL;
					//19 //???what is that? gap in file or what?
					if(map.version>RoE)
						i+=2;
					else
						i+=1;
					//bag artifacts //20
					int amount = readNormalNr(i, 2); i+=2; //number of artifacts in hero's bag
					if(amount>0)
					{
						for(int ss=0; ss<amount; ++ss)
						{
							id = readNormalNr(i, artidlen); i+=2;
							if(id!=artmask)
								spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[id]));
							else
								spec->artifacts.push_back(NULL);
						}
					}
				} //artifacts
				spec->guardRange = readNormalNr(i, 1); ++i;
				if(spec->guardRange == 0xff)
					spec->isGuarding = false;
				else
					spec->isGuarding = true;
				if(map.version>RoE)
				{
					bool hasBiography = bufor[i]; ++i; //true if hero has nonstandard (mapmaker defined) biography
					if(hasBiography)
					{
						int length = readNormalNr(i); i+=4;
						int iStart = i;
						i+=length;
						for(int bb=0; bb<length; ++bb)
						{
							spec->biography+=bufor[iStart+bb];
						}
					}
					spec->sex = !(bufor[i]); ++i;
				}
				//spells
				if(map.version>AB)
				{
					bool areSpells = bufor[i]; ++i;

					if(areSpells) //TODO: sprawdziæ //seems to be ok - tow
					{
						int ist = i;
						for(i; i<ist+9; ++i)
						{
							unsigned char c = bufor[i];
							for(int yy=0; yy<8; ++yy)
							{
								if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
								{
									if(c == (c|((unsigned char)intPow(2, yy))))
										spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
								}
							}
						}
					}
				}
				else if(map.version==AB) //we can read one spell
				{
					unsigned char buff = bufor[i]; ++i;
					if(buff!=254)
					{
						spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[buff]));
					}
				}
				//spells loaded
				if(map.version>AB)
				{
					spec->defaultMainStats = bufor[i]; ++i;
					if(spec->defaultMainStats)
					{
						spec->attack = bufor[i]; ++i;
						spec->defence = bufor[i]; ++i;
						spec->power = bufor[i]; ++i;
						spec->knowledge = bufor[i]; ++i;
					}
					else
					{
						spec->attack = -1;
						spec->defence = -1;
						spec->power = -1;
						spec->knowledge = -1;
					}

				}
				i+=16;
				nobj->info = spec;
				//////creating CHeroInstance
				CGHeroInstance * nhi = new CGHeroInstance;
				(*(static_cast<CGObjectInstance*>(nhi))) = *nobj;
				delete nobj;
				nobj=nhi;
				spec->myInstance = nhi;
				//nobj->isHero = true;
				(static_cast<CGHeroInstance*>(nobj))->moveDir = 4;
				nhi->isStanding = true;
				nhi->exp = spec->experience;
				nhi->level = CGI->heroh->level(nhi->exp);
				nhi->primSkills.resize(PRIMARY_SKILLS);
				nhi->primSkills[0] = spec->attack;
				nhi->primSkills[1] = spec->defence;
				nhi->primSkills[2] = spec->power;
				nhi->primSkills[3] = spec->knowledge;
				nhi->mana = spec->knowledge * 10;
				nhi->movement = -1;
				nhi->name = spec->name;
				nhi->setOwner(spec->player);
				nhi->pos = nobj->pos;
				nhi->type = spec->type;
				nhi->army = spec->garrison;
				nhi->portrait = -1; // TODO: przypisywac portret
				for(int qq=0; qq<spec->abilities.size(); ++qq)
				{
					nhi->secSkills.push_back(std::make_pair(spec->abilities[qq]->idNumber, spec->abilityLevels[qq]-1));
				}
				CGI->heroh->heroInstances.push_back(nhi);
				break;
			}
		case CREATURES_DEF:
			{
				CCreatureObjInfo * spec = new CCreatureObjInfo;
				if(map.version>RoE)
				{
					spec->bytes[0] = bufor[i]; ++i;
					spec->bytes[1] = bufor[i]; ++i;
					spec->bytes[2] = bufor[i]; ++i;
					spec->bytes[3] = bufor[i]; ++i;
				}
				spec->number = readNormalNr(i, 2); i+=2;
				spec->character = bufor[i]; ++i;
				bool isMesTre = bufor[i]; ++i; //true if there is message or treasury
				if(isMesTre)
				{
					int messLength = readNormalNr(i); i+=4;
					if(messLength>0)
					{
						for(int tt=0; tt<messLength; ++tt)
						{
							spec->message += bufor[i]; ++i;
						}
					}
					spec->wood = readNormalNr(i); i+=4;
					spec->mercury = readNormalNr(i); i+=4;
					spec->ore = readNormalNr(i); i+=4;
					spec->sulfur = readNormalNr(i); i+=4;
					spec->crytal = readNormalNr(i); i+=4;
					spec->gems = readNormalNr(i); i+=4;
					spec->gold = readNormalNr(i); i+=4;
					int artID = readNormalNr(i, (map.version == RoE ? 1 : 2)); i+=(map.version == RoE ? 1 : 2);
					if(map.version==RoE)
					{
						if(artID!=0xff)
							spec->gainedArtifact = &(CGameInfo::mainObj->arth->artifacts[artID]);
						else
							spec->gainedArtifact = NULL;
					}
					else
					{
						if(artID!=0xffff)
							spec->gainedArtifact = &(CGameInfo::mainObj->arth->artifacts[artID]);
						else
							spec->gainedArtifact = NULL;
					}
				}
				spec->neverFlees = bufor[i]; ++i;
				spec->notGrowingTeam = bufor[i]; ++i;
				i+=2;
				nobj->info = spec;
				break;
			}
		case EDefType::SIGN_DEF:
			{
				CSignObjInfo * spec = new CSignObjInfo;
				int length = readNormalNr(i); i+=4;
				for(int rr=0; rr<length; ++rr)
				{
					spec->message += bufor[i]; ++i;
				}
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::SEERHUT_DEF:
			{
				CSeerHutObjInfo * spec = new CSeerHutObjInfo;
				if(map.version>RoE)
				{
					spec->missionType = bufor[i]; ++i;
					switch(spec->missionType)
					{
					case 0:
						i+=3;
						continue;
					case 1:
						{
							spec->m1level = readNormalNr(i); i+=4;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 2:
						{
							spec->m2attack = bufor[i]; ++i;
							spec->m2defence = bufor[i]; ++i;
							spec->m2power = bufor[i]; ++i;
							spec->m2knowledge = bufor[i]; ++i;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 3:
						{
							spec->m3bytes[0] = bufor[i]; ++i;
							spec->m3bytes[1] = bufor[i]; ++i;
							spec->m3bytes[2] = bufor[i]; ++i;
							spec->m3bytes[3] = bufor[i]; ++i;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 4:
						{
							spec->m4bytes[0] = bufor[i]; ++i;
							spec->m4bytes[1] = bufor[i]; ++i;
							spec->m4bytes[2] = bufor[i]; ++i;
							spec->m4bytes[3] = bufor[i]; ++i;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 5:
						{
							int artNumber = bufor[i]; ++i;
							for(int yy=0; yy<artNumber; ++yy)
							{
								int artid = readNormalNr(i, 2); i+=2;
								spec->m5arts.push_back(&(CGameInfo::mainObj->arth->artifacts[artid]));
							}
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 6:
						{
							int typeNumber = bufor[i]; ++i;
							for(int hh=0; hh<typeNumber; ++hh)
							{
								int creType = readNormalNr(i, 2); i+=2;
								int creNumb = readNormalNr(i, 2); i+=2;
								spec->m6cre.push_back(&(CGameInfo::mainObj->creh->creatures[creType]));
								spec->m6number.push_back(creNumb);
							}
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 7:
						{
							spec->m7wood = readNormalNr(i); i+=4;
							spec->m7mercury = readNormalNr(i); i+=4;
							spec->m7ore = readNormalNr(i); i+=4;
							spec->m7sulfur = readNormalNr(i); i+=4;
							spec->m7crystal = readNormalNr(i); i+=4;
							spec->m7gems = readNormalNr(i); i+=4;
							spec->m7gold = readNormalNr(i); i+=4;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 8:
						{
							int heroType = bufor[i]; ++i;
							spec->m8hero = CGameInfo::mainObj->heroh->heroes[heroType];
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					case 9:
						{
							spec->m9player = bufor[i]; ++i;
							int limit = readNormalNr(i); i+=4;
							if(limit == ((int)0xffffffff))
							{
								spec->isDayLimit = false;
								spec->lastDay = -1;
							}
							else
							{
								spec->isDayLimit = true;
								spec->lastDay = limit;
							}
							break;
						}
					}//internal switch end (seer huts)

					int len1 = readNormalNr(i); i+=4;
					for(int ee=0; ee<len1; ++ee)
					{
						spec->firstVisitText += bufor[i]; ++i;
					}

					int len2 = readNormalNr(i); i+=4;
					for(int ee=0; ee<len2; ++ee)
					{
						spec->nextVisitText += bufor[i]; ++i;
					}

					int len3 = readNormalNr(i); i+=4;
					for(int ee=0; ee<len3; ++ee)
					{
						spec->completedText += bufor[i]; ++i;
					}
				}
				else //RoE
				{
					int artID = bufor[i]; ++i;
					if(artID!=255) //not none quest
					{
						spec->m5arts.push_back(&(CGameInfo::mainObj->arth->artifacts[artID]));
						spec->missionType = 5;
					}
					else
					{
						spec->missionType = 255;
					}
				}

				if(spec->missionType!=255)
				{
					unsigned char rewardType = bufor[i]; ++i;
					spec->rewardType = rewardType;

					switch(rewardType)
					{
					case 1:
						{
							spec->r1exp = readNormalNr(i); i+=4;
							break;
						}
					case 2:
						{
							spec->r2mana = readNormalNr(i); i+=4;
							break;
						}
					case 3:
						{
							spec->r3morale = bufor[i]; ++i;
							break;
						}
					case 4:
						{
							spec->r4luck = bufor[i]; ++i;
							break;
						}
					case 5:
						{
							spec->r5type = bufor[i]; ++i;
							spec->r5amount = readNormalNr(i, 3); i+=3;
							i+=1;
							break;
						}
					case 6:
						{
							spec->r6type = bufor[i]; ++i;
							spec->r6amount = bufor[i]; ++i;
							break;
						}
					case 7:
						{
							int abid = bufor[i]; ++i;
							spec->r7ability = CGameInfo::mainObj->abilh->abilities[abid];
							spec->r7level = bufor[i]; ++i;
							break;
						}
					case 8:
						{
							int artid = readNormalNr(i, (map.version == RoE ? 1 : 2)); i+=(map.version == RoE ? 1 : 2);
							spec->r8art = &(CGameInfo::mainObj->arth->artifacts[artid]);
							break;
						}
					case 9:
						{
							int spellid = bufor[i]; ++i;
							spec->r9spell = &(CGameInfo::mainObj->spellh->spells[spellid]);
							break;
						}
					case 10:
						{
							int creid = readNormalNr(i, 2); i+=2;
							spec->r10creature = &(CGameInfo::mainObj->creh->creatures[creid]);
							spec->r10amount = readNormalNr(i, 2); i+=2;
							break;
						}
					}// end of internal switch
					i+=2;
				}
				else //missionType==255
				{
					i+=3;
				}
				nobj->info = spec;
				break;
			}
		case EDefType::WITCHHUT_DEF:
			{
				CWitchHutObjInfo * spec = new CWitchHutObjInfo;
				if(map.version>RoE) //in reo we cannot specify it - all are allowed (I hope)
				{
					ist=i; //starting i for loop
					for(i; i<ist+4; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < CGameInfo::mainObj->abilh->abilities.size())
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									spec->allowedAbilities.push_back(CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]);
							}
						}
					}
				}
				else //(RoE map)
				{
					for(int gg=0; gg<CGameInfo::mainObj->abilh->abilities.size(); ++gg)
					{
						spec->allowedAbilities.push_back(CGameInfo::mainObj->abilh->abilities[gg]);
					}
				}
				
				nobj->info = spec;
				break;
			}
		case EDefType::SCHOLAR_DEF:
			{
				CScholarObjInfo * spec = new CScholarObjInfo;
				spec->bonusType = bufor[i]; ++i;
				switch(spec->bonusType)
				{
				case 0xff:
					++i;
					break;
				case 0:
					spec->r0type = bufor[i]; ++i;
					break;
				case 1:
					spec->r1 = CGameInfo::mainObj->abilh->abilities[bufor[i]]; ++i;
					break;
				case 2:
					spec->r2 = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
					break;
				}
				i+=6;
				nobj->info = spec;
				break;
			}
		case EDefType::GARRISON_DEF:
			{
				CGarrisonObjInfo * spec = new CGarrisonObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->units = readCreatureSet(i); i+= (map.version==RoE ? 21 : 28);
				if(map.version > RoE)
				{
					spec->movableUnits = bufor[i]; ++i;
				}
				else
					spec->movableUnits = true;
				i+=8;
				nobj->setOwner(spec->player);
				nobj->info = spec;
				break;
			}
		case EDefType::ARTIFACT_DEF:
			{
				CArtifactObjInfo * spec = new CArtifactObjInfo;
				bool areSettings = bufor[i]; ++i;
				if(areSettings)
				{
					int messLength = readNormalNr(i, 4); i+=4;
					for(int hh=0; hh<messLength; ++hh)
					{
						spec->message += bufor[i]; ++i;
					}
					bool areGuards = bufor[i]; ++i;
					if(areGuards)
					{
						spec->areGuards = true;
						spec->guards = readCreatureSet(i); i+= (map.version == RoE ? 21 : 28) ;
					}
					else
						spec->areGuards = false;
					i+=4;
				}
				nobj->info = spec;
				break;
			}
		case EDefType::RESOURCE_DEF:
			{
				CResourceObjInfo * spec = new CResourceObjInfo;
				bool isMessGuard = bufor[i]; ++i;
				if(isMessGuard)
				{
					int messLength = readNormalNr(i); i+=4;
					for(int mm=0; mm<messLength; ++mm)
					{
						spec->message+=bufor[i]; ++i;
					}
					spec->areGuards = bufor[i]; ++i;
					if(spec->areGuards)
					{
						spec->guards = readCreatureSet(i); i+= (map.version == RoE ? 21 : 28);
					}
					i+=4;
				}
				else
				{
					spec->areGuards = false;
				}
				spec->amount = readNormalNr(i); i+=4;
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::TOWN_DEF:
			{
				CCastleObjInfo * spec = new CCastleObjInfo;
				if(map.version!=RoE)
				{
					spec->bytes[0] = bufor[i]; ++i;
					spec->bytes[1] = bufor[i]; ++i;
					spec->bytes[2] = bufor[i]; ++i;
					spec->bytes[3] = bufor[i]; ++i;
				}
				else
				{
					spec->bytes[0] = 0;
					spec->bytes[1] = 0;
					spec->bytes[2] = 0;
					spec->bytes[3] = 0;
				}
				spec->player = bufor[i]; ++i;

				bool hasName = bufor[i]; ++i;
				if(hasName)
				{
					int len = readNormalNr(i); i+=4;
					for(int gg=0; gg<len; ++gg)
					{
						spec->name += bufor[i]; ++i;
					}
				}
				bool stGarr = bufor[i]; ++i; //true if garrison isn't empty
				if(stGarr)
				{
					spec->garrison = readCreatureSet(i); i+=( map.version > RoE ? 28 : 21 );
				}
				spec->garrison.formation = bufor[i]; ++i;
				spec->unusualBuildins = bufor[i]; ++i;
				if(spec->unusualBuildins)
				{
					for(int ff=0; ff<12; ++ff)
					{
						spec->buildingSettings[ff] = bufor[i]; ++i;
					}
				}
				else
				{
					spec->hasFort = bufor[i]; ++i;
				}

				int ist = i;
				if(map.version>RoE)
				{
					for(i; i<ist+9; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									spec->obligatorySpells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
							}
						}
					}
				}

				ist = i;
				for(i; i<ist+9; ++i)
				{
					unsigned char c = bufor[i];
					for(int yy=0; yy<8; ++yy)
					{
						if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
						{
							if(c != (c|((unsigned char)intPow(2, yy))))
								spec->possibleSpells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
						}
					}
				}

				/////// reading castle events //////////////////////////////////

				int numberOfEvent = readNormalNr(i); i+=4;

				for(int gh = 0; gh<numberOfEvent; ++gh)
				{
					CCastleEvent nce;
					int nameLen = readNormalNr(i); i+=4;
					for(int ll=0; ll<nameLen; ++ll)
					{
						nce.name += bufor[i]; ++i;
					}

					int messLen = readNormalNr(i); i+=4;
					for(int ll=0; ll<messLen; ++ll)
					{
						nce.message += bufor[i]; ++i;
					}

					nce.wood = readNormalNr(i); i+=4;
					nce.mercury = readNormalNr(i); i+=4;
					nce.ore = readNormalNr(i); i+=4;
					nce.sulfur = readNormalNr(i); i+=4;
					nce.crystal = readNormalNr(i); i+=4;
					nce.gems = readNormalNr(i); i+=4;
					nce.gold = readNormalNr(i); i+=4;

					nce.players = bufor[i]; ++i;
					if(map.version > AB)
					{
						nce.forHuman = bufor[i]; ++i;
					}
					else
						nce.forHuman = true;
					nce.forComputer = bufor[i]; ++i;
					nce.firstShow = readNormalNr(i, 2); i+=2;
					nce.forEvery = bufor[i]; ++i;

					i+=17;

					for(int kk=0; kk<6; ++kk)
					{
						nce.bytes[kk] = bufor[i]; ++i;
					}

					for(int vv=0; vv<7; ++vv)
					{
						nce.gen[vv] = readNormalNr(i, 2); i+=2;
					}
					i+=4;
					spec->events.push_back(nce);
				}
				spec->x = nobj->pos.x;
				spec->y = nobj->pos.y;
				spec->z = nobj->pos.z;

				/////// castle events have been read ///////////////////////////

				if(map.version > AB)
				{
					spec->alignment = bufor[i]; ++i;
				}
				else
					spec->alignment = 0xff;
				i+=3;
				nobj->info = spec;
				//////////// rewriting info to CTownInstance class /////////////////////
				CGTownInstance * nt = new CGTownInstance();
				(*(static_cast<CGObjectInstance*>(nt))) = *nobj;
				delete nobj;
				nobj = nt;

				if(spec->unusualBuildins)
				{
					nt->builtBuildings.insert(10);
					for(int ir = 0; ir < 6; ir++)
					{
						for(int bs=0;bs<8;bs++)
						{
							if(ir==0)
							{
								if (bs<3)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(11+bs);
									}
								}
								else if (bs<6)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(7+bs-3);
									}
								}
								else if(bs==6)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(5);
									}
								}
								else// if(bs==7)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(16);
									}
								}
							} //if(ir==0)
							else if(ir==1)
							{
								if(bs<2)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(14+bs);
									}
								}
								else if (bs==2)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										std::cout<<"Hej, sprawdz co to za budynek w miescie " <<nt<<std::endl;
									}
								}//bs==3 - not known what it is, 4 in 2. byte
								else
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(0+bs-3);
									}
								}

							}//else if(ir==1)
							else if(ir==2)
							{
								if(bs==0)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(6); //stocznia
									}
								}
								else if(bs==1)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(26); //grail
									}
								}
								else if(bs==2)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(17); //latarnia
									}
								}
								else if(bs==3)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(22); //bractwo miecza
									}
								}
								else if(bs==4)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(21); //stables
									}
								}
								else if(bs==5)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										std::cout<<"Hej, sprawdz co to za budynek2 w miescie " <<nt<<std::endl;
									}
								}
								else if(bs==6)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(30); //gen1
									}
								}
								else if(bs==7)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(37); //gen1+
									}
								}
							}//else if(ir==2)
							else if (ir==3)
							{
								if(bs==0)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										std::cout<<"Hej, sprawdz co to za budynek3 w miescie " <<nt<<std::endl;
									}
									continue;
								}
								else if(bs<3)
								{
									if(bs==1) 
									{
										if(spec->buildingSettings[ir] & (1<<bs))
										{
											nt->builtBuildings.insert(31); //gen2
										}
									}
									else
									{
										if(spec->buildingSettings[ir] & (1<<bs))
										{
											nt->builtBuildings.insert(38); //gen2+
										}
									}
								}
								else if (bs==3)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										std::cout<<"Hej, sprawdz co to za budynek4 w miescie " <<nt<<std::endl;
									}
									continue;
								}
								else
								{
									if(bs%2) //nieulepszone
									{
										if(spec->buildingSettings[ir] & (1<<bs))
										{
											nt->builtBuildings.insert((int)(39+(bs/2)-2)); 
										}
									}
									else
									{
										if(spec->buildingSettings[ir] & (1<<bs))
										{
											nt->builtBuildings.insert(32+(bs/2)-2); 
										}
									}
								}

							}//else if (ir==3)
							else if (ir==4 && bs==0)
							{
								if(spec->buildingSettings[ir] & 1<<0)
									nt->builtBuildings.insert(40);
								if(spec->buildingSettings[ir] & 1<<2)
									nt->builtBuildings.insert(34); 
								if(spec->buildingSettings[ir] & 1<<3)
									nt->builtBuildings.insert(41); 
								if(spec->buildingSettings[ir] & 1<<5)
									nt->builtBuildings.insert(35); 
								if(spec->buildingSettings[ir] & 1<<6)
									nt->builtBuildings.insert(42); 
								if(spec->buildingSettings[ir] & 1<<7)
									nt->builtBuildings.insert(36); 
							}//else if (ir==4)
							else if (ir==5)
							{
								if(bs==0)
								{
									if(spec->buildingSettings[ir] & (1<<bs))
									{
										nt->builtBuildings.insert(43); //gen7+
									}
								}
							}//else if (ir==5)
						}
					}
				}
				else
				{
					if(spec->hasFort)
					{
						nt->builtBuildings.insert(7);
					}
				}

				nt->setOwner(spec->player);
				nt->town = &CGI->townh->towns[nt->defInfo->subid];
				nt->builded = 0;
				nt->destroyed = 0;
				nt->name = spec->name;
				nt->garrison = spec->garrison;
				nt->garrisonHero = NULL;// spec->garnisonHero is not readed - TODO: readit
				nt->pos = int3(spec->x, spec->y, spec->z);
				nt->possibleSpells = spec->possibleSpells;
				nt->obligatorySpells = spec->obligatorySpells;
				nt->availableSpells = std::vector<CSpell*>();
				nt->creatureIncome = std::vector<int>();
				nt->creaturesLeft = std::vector<int>();
				CGI->townh->townInstances.push_back(nt);
				break;
			}
		case EDefType::PLAYERONLY_DEF:
			{
				CPlayerOnlyObjInfo * spec = new CPlayerOnlyObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				nobj->setOwner(spec->player);
				nobj->info = spec;
				break;
			}
		case EDefType::SHRINE_DEF:
			{
				CShrineObjInfo * spec = new CShrineObjInfo;
				spec->spell = bufor[i]; i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::SPELLSCROLL_DEF:
			{
				CSpellScrollObjinfo * spec = new CSpellScrollObjinfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+=( map.version == RoE ? 21 : 28 );
					}
					i+=4;
				}
				spec->spell = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
				i+=3;
				nobj->info = spec;
				break;
			}
		case EDefType::PANDORA_DEF:
			{
				CPandorasBoxObjInfo * spec = new CPandorasBoxObjInfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+= (map.version == RoE ? 21 : 28);
					}
					i+=4;
				}
				////// copied form event handling (seems to be similar)
				spec->gainedExp = readNormalNr(i, 4); i+=4;
				spec->manaDiff = readNormalNr(i, 4); i+=4;
				spec->moraleDiff = readNormalNr(i, 1, true); ++i;
				spec->luckDiff = readNormalNr(i, 1, true); ++i;
				spec->wood = readNormalNr(i); i+=4;
				spec->mercury = readNormalNr(i); i+=4;
				spec->ore = readNormalNr(i); i+=4;
				spec->sulfur = readNormalNr(i); i+=4;
				spec->crystal = readNormalNr(i); i+=4;
				spec->gems = readNormalNr(i); i+=4;
				spec->gold = readNormalNr(i); i+=4;
				spec->attack = readNormalNr(i, 1); ++i;
				spec->defence = readNormalNr(i, 1); ++i;
				spec->power = readNormalNr(i, 1); ++i;
				spec->knowledge = readNormalNr(i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)]); ++i;
					spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
				}
				int gart = readNormalNr(i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[readNormalNr(i, 2)])); i+=2;
				}
				int gspel = readNormalNr(i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[readNormalNr(i, 1)])); ++i;
				}
				int gcre = readNormalNr(i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(i, gcre); i+=4*gcre;
				i+=8;
				nobj->info = spec;
				///////end of copied fragment
				break;
			}
		case EDefType::GRAIL_DEF:
			{
				CGrailObjInfo * spec = new CGrailObjInfo;
				spec->radius = readNormalNr(i); i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN_DEF:
			{
				CCreGenObjInfo * spec = new CCreGenObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				for(int ggg=0; ggg<4; ++ggg)
				{
					spec->bytes[ggg] = bufor[i]; ++i;
				}
				if((spec->bytes[0] == '\0') && (spec->bytes[1] == '\0') && (spec->bytes[2] == '\0') && (spec->bytes[3] == '\0'))
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
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN2_DEF:
			{
				CCreGen2ObjInfo * spec = new CCreGen2ObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				for(int ggg=0; ggg<4; ++ggg)
				{
					spec->bytes[ggg] = bufor[i]; ++i;
				}
				if((spec->bytes[0] == '\0') && (spec->bytes[1] == '\0') && (spec->bytes[2] == '\0') && (spec->bytes[3] == '\0'))
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
				if(spec->maxLevel>7)
					spec->maxLevel = 7;
				if(spec->minLevel<1)
					spec->minLevel = 1;
				nobj->setOwner(spec->player);
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN3_DEF:
			{
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
				nobj->info = spec;
				break;
			}
		case EDefType::BORDERGUARD_DEF:
			{
				CBorderGuardObjInfo * spec = new CBorderGuardObjInfo;
				spec->missionType = bufor[i]; ++i;
				switch(spec->missionType)
				{
				case 1:
					{
						spec->m1level = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 2:
					{
						spec->m2attack = bufor[i]; ++i;
						spec->m2defence = bufor[i]; ++i;
						spec->m2power = bufor[i]; ++i;
						spec->m2knowledge = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 3:
					{
						spec->m3bytes[0] = bufor[i]; ++i;
						spec->m3bytes[1] = bufor[i]; ++i;
						spec->m3bytes[2] = bufor[i]; ++i;
						spec->m3bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 4:
					{
						spec->m4bytes[0] = bufor[i]; ++i;
						spec->m4bytes[1] = bufor[i]; ++i;
						spec->m4bytes[2] = bufor[i]; ++i;
						spec->m4bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 5:
					{
						int artNumber = bufor[i]; ++i;
						for(int yy=0; yy<artNumber; ++yy)
						{
							int artid = readNormalNr(i, 2); i+=2;
							spec->m5arts.push_back(&(CGameInfo::mainObj->arth->artifacts[artid]));
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 6:
					{
						int typeNumber = bufor[i]; ++i;
						for(int hh=0; hh<typeNumber; ++hh)
						{
							int creType = readNormalNr(i, 2); i+=2;
							int creNumb = readNormalNr(i, 2); i+=2;
							spec->m6cre.push_back(&(CGameInfo::mainObj->creh->creatures[creType]));
							spec->m6number.push_back(creNumb);
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 7:
					{
						spec->m7wood = readNormalNr(i); i+=4;
						spec->m7mercury = readNormalNr(i); i+=4;
						spec->m7ore = readNormalNr(i); i+=4;
						spec->m7sulfur = readNormalNr(i); i+=4;
						spec->m7crystal = readNormalNr(i); i+=4;
						spec->m7gems = readNormalNr(i); i+=4;
						spec->m7gold = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 8:
					{
						int heroType = bufor[i]; ++i;
						spec->m8hero = CGameInfo::mainObj->heroh->heroes[heroType];
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 9:
					{
						spec->m9player = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				}//internal switch end (seer huts)

				int len1 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len1; ++ee)
				{
					spec->firstVisitText += bufor[i]; ++i;
				}

				int len2 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len2; ++ee)
				{
					spec->nextVisitText += bufor[i]; ++i;
				}

				int len3 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len3; ++ee)
				{
					spec->completedText += bufor[i]; ++i;
				}
				nobj->info = spec;
				break;
			}
		} //end of main switch
		CGameInfo::mainObj->objh->objInstances.push_back(nobj);
		//TODO - dokoñczyæ, du¿o do zrobienia - trzeba patrzeæ, co def niesie
	}//*/ //end of loading objects; commented to make application work until it will be finished
	////objects loaded

	//processMap(defsToUnpack);
	std::vector<CDefHandler *> dhandlers = CGameInfo::mainObj->spriteh->extractManyFiles(defsToUnpack);
	for (int i=0;i<dhandlers.size();i++)
		map.defy[i]->handler=dhandlers[i];
	for(int vv=0; vv<map.defy.size(); ++vv)
	{
		if(map.defy[vv]->handler->alphaTransformed)
			continue;
		for(int yy=0; yy<map.defy[vv]->handler->ourImages.size(); ++yy)
		{
			map.defy[vv]->handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(map.defy[vv]->handler->ourImages[yy].bitmap);
			SDL_Surface * bufs = CSDL_Ext::secondAlphaTransform(map.defy[vv]->handler->ourImages[yy].bitmap, alphaTransSurf);
			SDL_FreeSurface(map.defy[vv]->handler->ourImages[yy].bitmap);
			map.defy[vv]->handler->ourImages[yy].bitmap = bufs;
			map.defy[vv]->handler->alphaTransformed = true;
		}
	}

	SDL_FreeSurface(alphaTransSurf);

	//assigning defobjinfos

	for(int ww=0; ww<CGI->objh->objInstances.size(); ++ww)
	{
		for(int h=0; h<CGI->dobjinfo->objs.size(); ++h)
		{
			if(CGI->dobjinfo->objs[h].defName==CGI->objh->objInstances[ww]->defInfo->name)
			{
				CGI->objh->objInstances[ww]->defObjInfoNumber = h;
				break;
			}
		}
	}

	for(int ww=0; ww<CGI->objh->objInstances.size(); ++ww)
	{
		if (CGI->objh->objInstances[ww]->defObjInfoNumber==-1)
			std::cout<<CGI->objh->objInstances[ww]->ID<<"\t" << CGI->objh->objInstances[ww]->subID<<"\t"<<CGI->objh->objInstances[ww]->defInfo->name<<std::endl;
	}

	//assigned

	//loading events
	int numberOfEvents = readNormalNr(i); i+=4;
	for(int yyoo=0; yyoo<numberOfEvents; ++yyoo)
	{
		CMapEvent ne;
		ne.name = std::string();
		ne.message = std::string();
		int nameLen = readNormalNr(i); i+=4;
		for(int qq=0; qq<nameLen; ++qq)
		{
			ne.name += bufor[i]; ++i;
		}
		int messLen = readNormalNr(i); i+=4;
		for(int qq=0; qq<messLen; ++qq)
		{
			ne.message +=bufor[i]; ++i;
		}
		ne.wood = readNormalNr(i); i+=4;
		ne.mercury = readNormalNr(i); i+=4;
		ne.ore = readNormalNr(i); i+=4;
		ne.sulfur = readNormalNr(i); i+=4;
		ne.crystal = readNormalNr(i); i+=4;
		ne.gems = readNormalNr(i); i+=4;
		ne.gold = readNormalNr(i); i+=4;
		ne.players = bufor[i]; ++i;
		if(map.version>AB)
		{
			ne.humanAffected = bufor[i]; ++i;
		}
		else
			ne.humanAffected = true;
		ne.computerAffected = bufor[i]; ++i;
		ne.firstOccurence = bufor[i]; ++i;
		ne.nextOccurence = bufor[i]; ++i;
		i+=18;
		map.events.push_back(ne);
	}
}
int CAmbarCendamo::readNormalNr (int pos, int bytCon, bool cyclic)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[pos+i]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
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
				CDefHandler  *sdh = CGI->spriteh->giveDef(CSemiDefHandler::nameFromType(map.terrain[i][j].tertype).c_str());
				loadedTypes.insert(map.terrain[i][j].tertype);
				defs.push_back(sdh);
			}
			if (map.twoLevel && loadedTypes.find(map.undergroungTerrain[i][j].tertype)==loadedTypes.end())
			{
				CDefHandler  *sdh = CGI->spriteh->giveDef(CSemiDefHandler::nameFromType(map.undergroungTerrain[i][j].tertype).c_str());
				loadedTypes.insert(map.undergroungTerrain[i][j].tertype);
				defs.push_back(sdh);
			}
		}
	}
}

EDefType CAmbarCendamo::getDefType(CGDefInfo * a)
{
	switch(a->id)
	{
	case 5: case 65: case 66: case 67: case 68: case 69:
		return EDefType::ARTIFACT_DEF; //handled
	case 6:
		return EDefType::PANDORA_DEF; //hanled
	case 26:
		return EDefType::EVENTOBJ_DEF; //handled
	case 33:
		return EDefType::GARRISON_DEF; //handled
	case 34: case 70: case 62: //70 - random hero //62 - prison
		return EDefType::HERO_DEF; //handled
	case 36:
		return EDefType::GRAIL_DEF; //hanled
	case 53: case 17: case 18: case 19: case 20: case 42: case 87: case 220://cases 17 - 20 and 42 - tests
		return EDefType::PLAYERONLY_DEF; //handled
	case 54: case 71: case 72: case 73: case 74: case 75: case 162: case 163: case 164:
		return EDefType::CREATURES_DEF; //handled
	case 59:
		return EDefType::SIGN_DEF; //handled
	case 77:
		return EDefType::TOWN_DEF; //can be problematic, but handled
	case 79: case 76:
		return EDefType::RESOURCE_DEF; //handled
	case 81:
		return EDefType::SCHOLAR_DEF; //handled
	case 83:
		return EDefType::SEERHUT_DEF; //handled
	case 91:
		return EDefType::SIGN_DEF; //handled
	case 88: case 89: case 90:
		return SHRINE_DEF; //handled
	case 93:
		return SPELLSCROLL_DEF; //handled
	case 98:
		return EDefType::TOWN_DEF; //handled
	case 113:
		return EDefType::WITCHHUT_DEF; //handled
	case 215:
		return EDefType::BORDERGUARD_DEF; //handled by analogy to seer huts ;]
	case 216:
		return EDefType::CREGEN2_DEF; //handled
	case 217:
		return EDefType::CREGEN_DEF; //handled
	case 218:
		return EDefType::CREGEN3_DEF; //handled
	case 219:
		return EDefType::GARRISON_DEF; //handled
	default:
		return EDefType::TERRAINOBJ_DEF; // nothing to be handled
	}
}

CCreatureSet CAmbarCendamo::readCreatureSet(int pos, int number)
{
	if(map.version>RoE)
	{
		CCreatureSet ret;
		std::pair<CCreature *, int> ins;
		if(number>0 && readNormalNr(pos, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+2, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(0,ins);
			ret.slots.insert(tt);
		}
		if(number>1 && readNormalNr(pos+4, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+4, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+6, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(1,ins);
			ret.slots.insert(tt);
		}
		if(number>2 && readNormalNr(pos+8, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+8, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+10, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(2,ins);
			ret.slots.insert(tt);
		}
		if(number>3 && readNormalNr(pos+12, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+12, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+14, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(3,ins);
			ret.slots.insert(tt);
		}
		if(number>4 && readNormalNr(pos+16, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+16, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+18, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(4,ins);
			ret.slots.insert(tt);
		}
		if(number>5 && readNormalNr(pos+20, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+20, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+22, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(5,ins);
			ret.slots.insert(tt);
		}
		if(number>6 && readNormalNr(pos+24, 2)!=0xffff)
		{
			int rettt = readNormalNr(pos+24, 2);
			if(rettt>32768)
				rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+26, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(6,ins);
			ret.slots.insert(tt);
		}
		return ret;
	}
	else
	{
		CCreatureSet ret;
		std::pair<CCreature *, int> ins;
		if(number>0 && readNormalNr(pos, 1)!=0xff)
		{
			int rettt = readNormalNr(pos, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+1, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(0,ins);
			ret.slots.insert(tt);
		}
		if(number>1 && readNormalNr(pos+3, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+3, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+4, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(1,ins);
			ret.slots.insert(tt);
		}
		if(number>2 && readNormalNr(pos+6, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+6, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+7, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(2,ins);
			ret.slots.insert(tt);
		}
		if(number>3 && readNormalNr(pos+9, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+9, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+10, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(3,ins);
			ret.slots.insert(tt);
		}
		if(number>4 && readNormalNr(pos+12, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+12, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+13, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(4,ins);
			ret.slots.insert(tt);
		}
		if(number>5 && readNormalNr(pos+15, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+15, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+16, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(5,ins);
			ret.slots.insert(tt);
		}
		if(number>6 && readNormalNr(pos+18, 1)!=0xff)
		{
			int rettt = readNormalNr(pos+18, 1);
			if(rettt>220)
				rettt = 256-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
			ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
			ins.second = readNormalNr(pos+19, 2);
			std::pair<int,std::pair<CCreature *, int> > tt(6,ins);
			ret.slots.insert(tt);
		}
		return ret;
	}
}

void CAmbarCendamo::processMap(std::vector<std::string> & defsToUnpack)
{
	resDefNames.push_back("AVTRNDM0.DEF");
	resDefNames.push_back("AVTWOOD0.DEF");
	resDefNames.push_back("AVTMERC0.DEF");
	resDefNames.push_back("AVTORE0.DEF");
	resDefNames.push_back("AVTSULF0.DEF");
	resDefNames.push_back("AVTCRYS0.DEF");
	resDefNames.push_back("AVTGEMS0.DEF");
	resDefNames.push_back("AVTGOLD0.DEF");
	resDefNames.push_back("ZMITHR.DEF");

	std::vector<CGDefInfo*> resDefNumbers;
	for(int hh=0; hh<resDefNames.size(); ++hh)
	{
		resDefNumbers.push_back(NULL);
		for(int k=0; k<map.defy.size(); ++k)
		{
			std::string buf = map.defy[k]->name;
			std::transform(buf.begin(), buf.end(), buf.begin(), (int(*)(int))toupper);
			if(resDefNames[hh] == buf)
			{
				resDefNumbers[resDefNumbers.size()-1] = map.defy[k];
				break;
			}
		}
	}
	std::vector<std::string> creDefNames;
	for(int dd=0; dd<140; ++dd) //we do not use here WoG units
	{
		creDefNames.push_back(CGI->dobjinfo->objs[dd+1184].defName);
	}
	std::vector<CGDefInfo*> creDefNumbers;
	for(int ee=0; ee<creDefNames.size(); ++ee)
	{
		creDefNumbers.push_back(NULL);
	}
	std::vector<std::string> artDefNames;
	std::vector<CGDefInfo* > artDefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass!=EartClass::SartClass)
			artDefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<artDefNames.size(); ++ee)
	{
		artDefNumbers.push_back(NULL);
	}
	
	std::vector<std::string> art1DefNames;
	std::vector<CGDefInfo*> art1DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::TartClass)
			art1DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art1DefNames.size(); ++ee)
	{
		art1DefNumbers.push_back(NULL);
	}
	
	std::vector<std::string> art2DefNames;
	std::vector<CGDefInfo*> art2DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::NartClass)
			art2DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art2DefNames.size(); ++ee)
	{
		art2DefNumbers.push_back(NULL);
	}

	std::vector<std::string> art3DefNames;
	std::vector<CGDefInfo*> art3DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::JartClass)
			art3DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art3DefNames.size(); ++ee)
	{
		art3DefNumbers.push_back(NULL);
	}

	std::vector<std::string> art4DefNames;
	std::vector<CGDefInfo*> art4DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::RartClass)
			art4DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art4DefNames.size(); ++ee)
	{
		art4DefNumbers.push_back(NULL);
	}

	std::vector<std::string> town0DefNames; //without fort
	std::vector<CGDefInfo*> town0DefNumbers;
	std::vector<std::string> town1DefNames; //with fort
	std::vector<CGDefInfo*> town1DefNumbers;

	for(int dd=0; dd<F_NUMBER; ++dd)
	{
		town1DefNames.push_back(CGI->dobjinfo->objs[dd+385].defName);
		town1DefNumbers.push_back(NULL);
	}

	std::vector< std::vector<std::string> > creGenNames;
	std::vector< std::vector<CGDefInfo*> > creGenNumbers;
	creGenNames.resize(F_NUMBER);
	creGenNumbers.resize(F_NUMBER);

	for(int ff=0; ff<F_NUMBER-1; ++ff)
	{
		for(int dd=0; dd<7; ++dd)
		{
			creGenNames[ff].push_back(CGI->dobjinfo->objs[395+7*ff+dd].defName);
			creGenNumbers[ff].push_back(NULL);
		}
	}

	for(int dd=0; dd<7; ++dd)
	{
		creGenNumbers[8].push_back(NULL);
	}

	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[457].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[452].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[455].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[454].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[453].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[458].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[459].defName);

	for(int b=0; b<CGI->scenarioOps.playerInfos.size(); ++b) // choosing random player alignment
	{
		if(CGI->scenarioOps.playerInfos[b].castle==-1)
			CGI->scenarioOps.playerInfos[b].castle = rand()%F_NUMBER;
	}
	
	//variables initialized
	for(int j=0; j<CGI->objh->objInstances.size(); ++j)
	{
		CGDefInfo * curDef = CGI->objh->objInstances[j]->defInfo;
		switch(getDefType(curDef))
		{
		case EDefType::RESOURCE_DEF:
			{
				if(curDef->id==76) //resource to specify
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 79;
					nxt->subid = rand()%7;
					if(resDefNumbers[nxt->subid+1]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = resDefNumbers[nxt->subid+1];
						continue;
					}
					nxt->name = resDefNames[nxt->subid+1];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(resDefNumbers[nxt->subid+1]==NULL)
					{
						resDefNumbers[nxt->subid+1] = nxt;
					}
				}
				break;
			}
		case EDefType::CREATURES_DEF:
			{
				if(curDef->id==72) //random monster lvl 1
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==73) //random monster lvl 2
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+2;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==74) //random monster lvl 3
				{
					CGDefInfo *nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+4;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==75) //random monster lvl 4
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+6;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==162) //random monster lvl 5
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+8;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==163) //random monster lvl 6
				{
					CGDefInfo* nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+10;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==164) //random monster lvl 7
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = 14*(rand()%9)+rand()%2+12;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==71) //random monster (any level)
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 54;
					nxt->subid = rand()%126;
					if(creDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = creDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creDefNumbers[nxt->subid]==NULL)
					{
						creDefNumbers[nxt->subid] = nxt;
					}
				}
				break;
			} //end of case
		case EDefType::ARTIFACT_DEF:
			{
				if(curDef->id==65) //random atrifact (any class)
				{
					CGDefInfo *nxt = curDef;
					nxt->id = 5;
					nxt->subid = rand()%artDefNames.size();
					if(artDefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = artDefNumbers[nxt->subid];
						continue;
					}
					nxt->name = artDefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(artDefNumbers[nxt->subid]==NULL)
					{
						artDefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==66) //random atrifact (treasure)
				{
					CGDefInfo* nxt = curDef;
					nxt->id = 5;
					nxt->subid = rand()%art1DefNames.size();
					if(art1DefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = art1DefNumbers[nxt->subid];
						continue;
					}
					nxt->name = art1DefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(art1DefNumbers[nxt->subid]==NULL)
					{
						art1DefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==67) //random atrifact (minor)
				{
					CGDefInfo *nxt = curDef;
					nxt->id = 5;
					nxt->subid = rand()%art2DefNames.size();
					if(art2DefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = art2DefNumbers[nxt->subid];
						continue;
					}
					nxt->name = art2DefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(art2DefNumbers[nxt->subid]==NULL)
					{
						art2DefNumbers[nxt->subid] = nxt;
					}
				}
				if(curDef->id==68) //random atrifact (major)
				{
					CGDefInfo* nxt = curDef;
					nxt->id = 5;
					nxt->subid = rand()%art3DefNames.size();
					if(art3DefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = art3DefNumbers[nxt->subid];
						continue;
					}
					nxt->name = art3DefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(art3DefNumbers[nxt->subid]==NULL)
					{
						art3DefNumbers[nxt->subid] =nxt;
					}
				}
				if(curDef->id==69) //random atrifact (relic)
				{
					CGDefInfo *nxt = curDef;
					nxt->id = 5;
					nxt->subid = rand()%art4DefNames.size();
					if(art4DefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = art4DefNumbers[nxt->subid];
						continue;
					}
					nxt->name = art4DefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(art4DefNumbers[nxt->subid]==NULL)
					{
						art4DefNumbers[nxt->subid] = nxt;
					}
				}
				break;
			}
		case EDefType::TOWN_DEF:
			{
				if(curDef->id==77) //random town
				{
					CGDefInfo* nxt = curDef;
					nxt->id = 98;
					if(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player==0xff)
					{
						nxt->subid = rand()%town1DefNames.size();		
					}
					else
					{
						if(CGI->scenarioOps.getIthPlayersSettings(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player).castle>-1)
						{
							nxt->subid = CGI->scenarioOps.getIthPlayersSettings(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player).castle;
						}
						else
						{
							nxt->subid = rand()%town1DefNames.size();
						}
					}
					if(town1DefNumbers[nxt->subid]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = town1DefNumbers[nxt->subid];
						continue;
					}
					nxt->name = town1DefNames[nxt->subid];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(town1DefNumbers[nxt->subid]==NULL)
					{
						town1DefNumbers[nxt->subid] = nxt;
					}
					for (int ij=0;ij<CGI->townh->townInstances.size();ij++) // wyharatac gdy bedzie dziedziczenie
					{
						if (CGI->townh->townInstances[ij]->pos==CGI->objh->objInstances[j]->pos)
						{
							CGI->townh->townInstances[ij]->town = &CGI->townh->towns[nxt->subid];
							break;
						}
					}
				}
				//((CCastleObjInfo*)CGI->objh->objInstances[j].info)
				break;
			}
		case EDefType::HERO_DEF:
			{
				std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						CGI->objh->objInstances[j]->defInfo->name);

				CGI->objh->objInstances[j]->defInfo->printPriority = 0;
				if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
				{
					CGI->objh->objInstances[j]->defInfo->isOnDefList = false;
				}
				else
				{
					CGI->objh->objInstances[j]->defInfo->isOnDefList = true;
				}
				break;
			}
		} //end of main switch
	} //end of main loop
	for(int j=0; j<CGI->objh->objInstances.size(); ++j) //for creature dwellings on map (they are town type dependent)
	{
		CGDefInfo * curDef = CGI->objh->objInstances[j]->defInfo;
		switch(getDefType(curDef))
		{
		case EDefType::CREGEN_DEF:
			{
				if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->asCastle)
				{
					CGDefInfo *nxt = curDef;
					nxt->id = 17;
					for(int vv=0; vv<CGI->objh->objInstances.size(); ++vv)
					{
						if(getDefType(CGI->objh->objInstances[vv]->defInfo)==EDefType::TOWN_DEF)
						{
							if(
								((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[0]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[0]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[1]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[1]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[2]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[2]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[3]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[3])
							{
								for(int mm=0; mm<town1DefNames.size(); ++mm)
								{
									std::transform(town1DefNames[mm].begin(), town1DefNames[mm].end(), town1DefNames[mm].begin(), (int(*)(int))toupper);
									std::string hlp = CGI->objh->objInstances[vv]->defInfo->name;
									std::transform(hlp.begin(), hlp.end(), hlp.begin(), (int(*)(int))toupper);
									if(town1DefNames[mm]==hlp)
									{
										nxt->subid = mm;
									}
								}
							}
						}
					}
					int lvl = atoi(CGI->objh->objInstances[j]->defInfo->name.substr(7, 8).c_str())-1;
					nxt->name = creGenNames[nxt->subid][lvl];
					if(creGenNumbers[nxt->subid][lvl]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creGenNumbers[nxt->subid][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creGenNumbers[nxt->subid][lvl]==NULL)
					{
						creGenNumbers[nxt->subid][lvl] = nxt;
					}
				}
				else //if not as castle
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 17;
					std::vector<int> possibleTowns;
					for(int bb=0; bb<8; ++bb)
					{
						if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->castles[0] & (1<<bb))
						{
							possibleTowns.push_back(bb);
						}
					}
					if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->castles[1])
						possibleTowns.push_back(8);
					nxt->subid = possibleTowns[rand()%possibleTowns.size()];
					int lvl = atoi(CGI->objh->objInstances[j]->defInfo->name.substr(7, 8).c_str())-1;
					nxt->name = creGenNames[nxt->subid][lvl];
					if(creGenNumbers[nxt->subid][lvl]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creGenNumbers[nxt->subid][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creGenNumbers[nxt->subid][lvl]==NULL)
					{
						creGenNumbers[nxt->subid][lvl] = nxt;
					}
				}
				break;
			}
		case EDefType::CREGEN2_DEF:
			{
				if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->asCastle)
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 17;
					for(int vv=0; vv<CGI->objh->objInstances.size(); ++vv)
					{
						if(getDefType(CGI->objh->objInstances[vv]->defInfo)==EDefType::TOWN_DEF)
						{
							if(
								((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[0]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[0]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[1]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[1]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[2]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[2]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[3]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[3])
							{
								for(int mm=0; mm<town1DefNames.size(); ++mm)
								{
									std::transform(town1DefNames[mm].begin(), town1DefNames[mm].end(), town1DefNames[mm].begin(), (int(*)(int))toupper);
									std::string hlp = CGI->objh->objInstances[vv]->defInfo->name;
									std::transform(hlp.begin(), hlp.end(), hlp.begin(), (int(*)(int))toupper);
									if(town1DefNames[mm]==hlp)
									{
										nxt->subid = mm;
									}
								}
							}
						}
					}
					int lvl;
					if((((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel)!=0) 
						lvl = rand()%(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
					else lvl = 0;
					nxt->name = creGenNames[nxt->subid][lvl];
					if(creGenNumbers[nxt->subid][lvl]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creGenNumbers[nxt->subid][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creGenNumbers[nxt->subid][lvl]==NULL)
					{
						creGenNumbers[nxt->subid][lvl] = nxt;
					}
				}
				else //if not as castle
				{
					CGDefInfo * nxt = curDef;
					nxt->id = 17;
					std::vector<int> possibleTowns;
					for(int bb=0; bb<8; ++bb)
					{
						if(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->castles[0] & (1<<bb))
						{
							possibleTowns.push_back(bb);
						}
					}
					if(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->castles[1])
						possibleTowns.push_back(8);
					nxt->subid = possibleTowns[rand()%possibleTowns.size()];
					int lvl = rand()%(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
					nxt->name = creGenNames[nxt->subid][lvl];
					if(creGenNumbers[nxt->subid][lvl]!=NULL)
					{
						CGI->objh->objInstances[j]->defInfo = creGenNumbers[nxt->subid][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt->name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt->isOnDefList = false;
					}
					else
					{
						nxt->printPriority = pit->priority;
						nxt->isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt->name);
					CGI->objh->objInstances[j]->defInfo = nxt;
					if(creGenNumbers[nxt->subid][lvl]==NULL)
					{
						creGenNumbers[nxt->subid][lvl] = nxt;
					}
				}
			}
		case EDefType::CREGEN3_DEF:
			{
				CGDefInfo * nxt = curDef;
				nxt->id = 17;
				nxt->subid = atoi(CGI->objh->objInstances[j]->defInfo->name.substr(7, 8).c_str());
				int lvl = -1;
				CCreGen3ObjInfo * ct = (CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info;
				if(ct->maxLevel>7)
					ct->maxLevel = 7;
				if(ct->minLevel<1)
					ct->minLevel = 1;
				if((((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel)!=0)
					lvl = rand()%(((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
				else
					lvl = ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel;
				nxt->name = creGenNames[nxt->subid][lvl];
				if(creGenNumbers[nxt->subid][lvl]!=NULL)
				{
					CGI->objh->objInstances[j]->defInfo = creGenNumbers[nxt->subid][lvl];
					continue;
				}
				std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
					nxt->name);
				if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
				{
					nxt->isOnDefList = false;
				}
				else
				{
					nxt->printPriority = pit->priority;
					nxt->isOnDefList = true;
				}
				map.defy.push_back(nxt); // add this def to the vector
				defsToUnpack.push_back(nxt->name);
				CGI->objh->objInstances[j]->defInfo = nxt;
				if(creGenNumbers[nxt->subid][lvl]==NULL)
				{
					creGenNumbers[nxt->subid][lvl] = nxt;
				}
				break;
			}
		}//end of main switch
	} //end of sencond for loop
}
