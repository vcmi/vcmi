#include "stdafx.h"
#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "CGameInfo.h"
#include "CObjectHandler.h"
#include "CCastleHandler.h"
#include "SDL_Extensions.h"
#include "boost\filesystem.hpp"
#include <set>
#define CGI (CGameInfo::mainObj)

unsigned int intPow(unsigned int a, unsigned int b)
{
	unsigned int ret=1;
	for(int i=0; i<b; ++i)
		ret*=a;
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
	for (int i=0; i<map.defy.size(); i++)
	{
		std::ofstream * of = new std::ofstream(map.defy[i].name.c_str());
		for (int j=0;j<46;j++)
		{
			(*of) << map.defy[i].bytes[j]<<std::endl;
		}
		of->close();
		delete of;
	}
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
	map.levelLimit = bufor[i++]; // hero level limit
	for (pom=0;pom<8;pom++)
	{
		map.players[pom].canHumanPlay = bufor[i++];
		map.players[pom].canComputerPlay = bufor[i++];
		if ((!(map.players[pom].canHumanPlay || map.players[pom].canComputerPlay)))
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
	}//allowed artifacts have been read

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
		DefInfo vinya; // info about new def
		for (int cd=0;cd<nameLength;cd++)
		{
			vinya.name += bufor[i++];
		}
		for (int v=0; v<42; v++) // read info
		{
			vinya.bytes[v] = bufor[i++];
		}
		std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
			vinya.name);
		if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
		{
			vinya.isOnDefList = false;
		}
		else
		{
			vinya.printPriority = pit->priority;
			vinya.isOnDefList = true;
		}
		map.defy.push_back(vinya); // add this def to the vector
		defsToUnpack.push_back(vinya.name);
		//testing - only fragment//////////////////////////////////////////////////////////////
		/*map.defy[idd].handler = new CDefHandler();
		CGameInfo::mainObj->lodh->extractFile(std::string("newH3sprite.lod"), map.defy[idd].name);
		map.defy[idd].handler->openDef( std::string("newH3sprite\\")+map.defy[idd].name);
		for(int ff=0; ff<map.defy[idd].handler->ourImages.size(); ++ff) //adding shadows and transparency
		{
			map.defy[idd].handler->ourImages[ff].bitmap = CSDL_Ext::alphaTransform(map.defy[idd].handler->ourImages[ff].bitmap);
			SDL_Surface * bufs = CSDL_Ext::secondAlphaTransform(map.defy[idd].handler->ourImages[ff].bitmap, alphaTransSurf);
			SDL_FreeSurface(map.defy[idd].handler->ourImages[ff].bitmap);
			map.defy[idd].handler->ourImages[ff].bitmap = bufs;
		}
		boost::filesystem::remove(boost::filesystem::path(std::string("newH3sprite\\")+map.defy[idd].name));*/
		//system((std::string("DEL newH3sprite\\")+map.defy[idd].name).c_str());
		//end fo testing - only fragment///////////////////////////////////////////////////////

		//teceDef();
	}
	THC std::cout<<"Reading defs: "<<th.getDif()<<std::endl;
	////loading objects
	int howManyObjs = readNormalNr(i, 4); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		std::cout << "object nr "<<ww<<"\ti= "<<i<<std::endl;
		CObjectInstance nobj; //we will read this object
		nobj.id = CGameInfo::mainObj->objh->objInstances.size();
		nobj.x = bufor[i++];
		nobj.y = bufor[i++];
		nobj.z = bufor[i++];
		nobj.defNumber = readNormalNr(i, 4); i+=4;

		//if (((nobj.x==0)&&(nobj.y==0)) || nobj.x>map.width || nobj.y>map.height || nobj.z>1 || nobj.defNumber>map.defy.size())
		//	std::cout << "Alarm!!! Obiekt "<<ww<<" jest kopniety (lub wystaje poza mape)\n";

		i+=5;
		unsigned char buff [30];
		for(int ccc=0; ccc<30; ++ccc)
		{
			buff[ccc] = bufor[i+ccc];
		}
		EDefType uu = getDefType(map.defy[nobj.defNumber]);
		int j = map.defy[nobj.defNumber].bytes[16];
		int p = 99;
		switch(getDefType(map.defy[nobj.defNumber]))
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
						spec->guarders = readCreatureSet(i); i+=28;
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
					spec->abilities.push_back(&((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)])); ++i;
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
				spec->availableFor = readNormalNr(i, 1); ++i;
				spec->computerActivate = readNormalNr(i, 1); ++i;
				spec->humanActivate = readNormalNr(i, 1); ++i;
				i+=4;
				nobj.info = spec;
				break;
			}
		case EDefType::HERO_DEF:
			{
				CHeroObjInfo * spec = new CHeroObjInfo;
				spec->bytes[0] = bufor[i]; ++i;
				spec->bytes[1] = bufor[i]; ++i;
				spec->bytes[2] = bufor[i]; ++i;
				spec->bytes[3] = bufor[i]; ++i;
				spec->player = bufor[i]; ++i;
				spec->type = &(CGameInfo::mainObj->heroh->heroes[readNormalNr(i, 1)]); ++i;
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
				bool isExp = bufor[i]; ++i; //true if hore's experience is greater than 0
				if(isExp)
				{
					spec->experience = readNormalNr(i); i+=4;
				}
				else spec->experience = 0;
				bool portrait=bufor[i]; ++i;
				if (portrait)
					i++; //TODO read portrait nr, save, open
				bool nonstandardAbilities = bufor[i]; ++i; //true if hero has specified abilities
				if(nonstandardAbilities)
				{
					int howMany = readNormalNr(i); i+=4;
					for(int yy=0; yy<howMany; ++yy)
					{
						spec->abilities.push_back(&(CGameInfo::mainObj->abilh->abilities[readNormalNr(i, 1)])); ++i;
						spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
					}
				}
				bool standGarrison = bufor[i]; ++i; //true if hero has nonstandard garrison
				spec->standardGarrison = standGarrison;
				if(standGarrison)
				{
					spec->garrison = readCreatureSet(i); i+=28; //4 bytes per slot
				}
				bool form = bufor[i]; ++i; //formation
				spec->garrison.formation = form;
				bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
				if(artSet)
				{
					//head art //1
					int id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artHead = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artHead = NULL;
					//shoulders art //2
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artShoulders = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artShoulders = NULL;
					//neck art //3
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artNeck = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artNeck = NULL;
					//right hand art //4
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artRhand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRhand = NULL;
					//left hand art //5
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artLHand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLHand = NULL;
					//torso art //6
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artTorso = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artTorso = NULL;
					//right hand ring //7
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artRRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRRing = NULL;
					//left hand ring //8
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artLRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLRing = NULL;
					//feet art //9
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artFeet = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artFeet = NULL;
					//misc1 art //10
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc1 = NULL;
					//misc2 art //11
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc2 = NULL;
					//misc3 art //12
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc3 = NULL;
					//misc4 art //13
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc4 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc4 = NULL;
					//machine1 art //14
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach1 = NULL;
					//machine2 art //15
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach2 = NULL;
					//machine3 art //16
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach3 = NULL;
					//misc5 art //17
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc5 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc5 = NULL;
					//spellbook
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artSpellBook = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artSpellBook = NULL;
					//19 //???what is that? gap in file or what?
					i+=2;
					//bag artifacts //20
					int amount = readNormalNr(i, 2); i+=2; //number of artifacts in hero's bag
					if(amount>0)
					{
						for(int ss=0; ss<amount; ++ss)
						{
							id = readNormalNr(i, 2); i+=2;
							if(id!=0xffff)
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
				//spells
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
				//spells loaded
				spec->defaultMianStats = bufor[i]; ++i;
				if(spec->defaultMianStats)
				{
					spec->attack = bufor[i]; ++i;
					spec->defence = bufor[i]; ++i;
					spec->power = bufor[i]; ++i;
					spec->knowledge = bufor[i]; ++i;
				}
				i+=16;
				nobj.info = spec;
				break;
			}
		case CREATURES_DEF:
			{
				CCreatureObjInfo * spec = new CCreatureObjInfo;
				spec->bytes[0] = bufor[i]; ++i;
				spec->bytes[1] = bufor[i]; ++i;
				spec->bytes[2] = bufor[i]; ++i;
				spec->bytes[3] = bufor[i]; ++i;
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
					int artID = readNormalNr(i, 2); i+=2;
					if(artID!=0xffff)
						spec->gainedArtifact = &(CGameInfo::mainObj->arth->artifacts[artID]);
					else
						spec->gainedArtifact = NULL;
				}
				spec->neverFlees = bufor[i]; ++i;
				spec->notGrowingTeam = bufor[i]; ++i;
				i+=2;
				nobj.info = spec;
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
				nobj.info = spec;
				break;
			}
		case EDefType::SEERHUT_DEF:
			{
				CSeerHutObjInfo * spec = new CSeerHutObjInfo;
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
						spec->m8hero = &(CGameInfo::mainObj->heroh->heroes[heroType]);
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
						spec->r7ability = &(CGameInfo::mainObj->abilh->abilities[abid]);
						spec->r7level = bufor[i]; ++i;
						break;
					}
				case 8:
					{
						int artid = readNormalNr(i, 2); i+=2;
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
				nobj.info = spec;
				break;
			}
		case EDefType::WITCHHUT_DEF:
			{
				CWitchHutObjInfo * spec = new CWitchHutObjInfo;
				ist=i; //starting i for loop
				for(i; i<ist+4; ++i)
				{
					unsigned char c = bufor[i];
					for(int yy=0; yy<8; ++yy)
					{
						if((i-ist)*8+yy < CGameInfo::mainObj->abilh->abilities.size())
						{
							if(c == (c|((unsigned char)intPow(2, yy))))
								spec->allowedAbilities.push_back(&(CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]));
						}
					}
				}
				
				nobj.info = spec;
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
					spec->r1 = &(CGameInfo::mainObj->abilh->abilities[bufor[i]]); ++i;
					break;
				case 2:
					spec->r2 = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
					break;
				}
				i+=6;
				nobj.info = spec;
				break;
			}
		case EDefType::GARRISON_DEF:
			{
				CGarrisonObjInfo * spec = new CGarrisonObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->units = readCreatureSet(i); i+=28;
				spec->movableUnits = bufor[i]; ++i;
				i+=8;
				nobj.info = spec;
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
						spec->guards = readCreatureSet(i); i+=28;
					}
					else
						spec->areGuards = false;
					i+=4;
				}
				nobj.info = spec;
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
						spec->guards = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				else
				{
					spec->areGuards = false;
				}
				spec->amount = readNormalNr(i); i+=4;
				i+=4;
				nobj.info = spec;
				break;
			}
		case EDefType::TOWN_DEF:
			{
				CCastleObjInfo * spec = new CCastleObjInfo;
				spec->bytes[0] = bufor[i]; ++i;
				spec->bytes[1] = bufor[i]; ++i;
				spec->bytes[2] = bufor[i]; ++i;
				spec->bytes[3] = bufor[i]; ++i;
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
					spec->garrison = readCreatureSet(i); i+=28;
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
					nce.forHuman = bufor[i]; ++i;
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

				/////// castle events have been read ///////////////////////////

				spec->alignment = bufor[i]; ++i;
				i+=3;
				nobj.info = spec;
				break;
			}
		case EDefType::PLAYERONLY_DEF:
			{
				CPlayerOnlyObjInfo * spec = new CPlayerOnlyObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				nobj.info = spec;
				break;
			}
		case EDefType::SHRINE_DEF:
			{
				CShrineObjInfo * spec = new CShrineObjInfo;
				spec->spell = bufor[i]; i+=4;
				nobj.info = spec;
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
						spec->guarders = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				spec->spell = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
				i+=3;
				nobj.info = spec;
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
						spec->guarders = readCreatureSet(i); i+=28;
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
					spec->abilities.push_back(&((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)])); ++i;
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
				nobj.info = spec;
				///////end of copied fragment
				break;
			}
		case EDefType::GRAIL_DEF:
			{
				CGrailObjInfo * spec = new CGrailObjInfo;
				spec->radius = readNormalNr(i); i+=4;
				nobj.info = spec;
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
				nobj.info = spec;
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
				nobj.info = spec;
				break;
			}
		case EDefType::CREGEN3_DEF:
			{
				CCreGen3ObjInfo * spec = new CCreGen3ObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->minLevel = bufor[i]; ++i;
				spec->maxLevel = bufor[i]; ++i;
				nobj.info = spec;
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
						spec->m8hero = &(CGameInfo::mainObj->heroh->heroes[heroType]);
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
				nobj.info = spec;
				break;
			}
		} //end of main switch
		CGameInfo::mainObj->objh->objInstances.push_back(nobj);
		//TODO - dokoñczyæ, du¿o do zrobienia - trzeba patrzeæ, co def niesie
	}//*/ //end of loading objects; commented to make application work until it will be finished
	////objects loaded

	processMap(defsToUnpack);
	std::vector<CDefHandler *> dhandlers = CGameInfo::mainObj->spriteh->extractManyFiles(defsToUnpack);
	for (int i=0;i<dhandlers.size();i++)
		map.defy[i].handler=dhandlers[i];
	for(int vv=0; vv<map.defy.size(); ++vv)
	{
		if(map.defy[vv].handler->alphaTransformed)
			continue;
		for(int yy=0; yy<map.defy[vv].handler->ourImages.size(); ++yy)
		{
			map.defy[vv].handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(map.defy[vv].handler->ourImages[yy].bitmap);
			SDL_Surface * bufs = CSDL_Ext::secondAlphaTransform(map.defy[vv].handler->ourImages[yy].bitmap, alphaTransSurf);
			SDL_FreeSurface(map.defy[vv].handler->ourImages[yy].bitmap);
			map.defy[vv].handler->ourImages[yy].bitmap = bufs;
			map.defy[vv].handler->alphaTransformed = true;
		}
	}
	SDL_FreeSurface(alphaTransSurf);

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
		ne.humanAffected = bufor[i]; ++i;
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

EDefType CAmbarCendamo::getDefType(DefInfo &a)
{
	switch(a.bytes[16])
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
	CCreatureSet ret;
	if(number>0 && readNormalNr(pos, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot1 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s1 = readNormalNr(pos+2, 2);
	}
	else
	{
		ret.slot1 = NULL;
		ret.s1 = 0;
	}
	if(number>1 && readNormalNr(pos+4, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+4, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot2 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s2 = readNormalNr(pos+6, 2);
	}
	else
	{
		ret.slot2 = NULL;
		ret.s2 = 0;
	}
	if(number>2 && readNormalNr(pos+8, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+8, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot3 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s3 = readNormalNr(pos+10, 2);
	}
	else
	{
		ret.slot3 = NULL;
		ret.s3 = 0;
	}
	if(number>3 && readNormalNr(pos+12, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+12, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot4 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s4 = readNormalNr(pos+14, 2);
	}
	else
	{
		ret.slot4 = NULL;
		ret.s4 = 0;
	}
	if(number>4 && readNormalNr(pos+16, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+16, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot5 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s5 = readNormalNr(pos+18, 2);
	}
	else
	{
		ret.slot5 = NULL;
		ret.s5 = 0;
	}
	if(number>5 && readNormalNr(pos+20, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+20, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot6 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s6 = readNormalNr(pos+22, 2);
	}
	else
	{
		ret.slot6 = NULL;
		ret.s6 = 0;
	}
	if(number>6 && readNormalNr(pos+24, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+24, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ret.slot7 = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ret.s7 = readNormalNr(pos+26, 2);
	}
	else
	{
		ret.slot7 = NULL;
		ret.s7 = 0;
	}
	return ret;
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
	for(int j=0; j<CGI->objh->objInstances.size(); ++j)
	{
		DefInfo & curDef = map.defy[CGI->objh->objInstances[j].defNumber];
		switch(getDefType(curDef))
		{
		case EDefType::RESOURCE_DEF:
			{
				if(curDef.bytes[16]==76) //resource to specify
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 79;
					nxt.bytes[20] = rand()%7;
					nxt.name = resDefNames[nxt.bytes[20]+1];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j].defNumber = map.defy.size()-1;
				}
				break;
			}
		}
	}
}
