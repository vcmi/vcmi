#define VCMI_DLL
#include "stdafx.h"
#include "map.h"
#include "hch/CObjectHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include "lib/VCMI_Lib.h"
#include <zlib.h>
#include <boost/crc.hpp>
std::set<si32> convertBuildings(const std::set<si32> h3m, int castleID)
{
	std::map<int,int> mapa;
	std::set<si32> ret;
	std::ifstream b5("config/buildings5.txt");
	while(!b5.eof())
	{
		int a, b;
		b5 >> a >> b;
		if(castleID==8 && b==17) //magic university ID 17 (h3m) => 21 (vcmi)
			b=21;
		mapa[a]=b;
	}

	for(std::set<si32>::const_iterator i=h3m.begin();i!=h3m.end();i++)
	{
		if(mapa[*i]>=0)
			ret.insert(mapa[*i]);
		else if(mapa[*i]  >=  (-CREATURES_PER_TOWN)) // horde buildings
		{
			int level = (-mapa[*i]);
			if(h3m.find(20+(level*3)) != h3m.end()) //upgraded creature horde building
			{
				if(((castleID==1) || (castleID==3)) && ((level==3) || (level==5)))
					ret.insert(25);
				else
					ret.insert(19);
			}
			else
			{
				if(((castleID==1) || (castleID==3)) && ((level==3) || (level==5)))
					ret.insert(24);
				else
					ret.insert(18);
			}
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
EDefType getDefType(CGDefInfo * a)
{
	switch(a->id)
	{
	case 5: case 65: case 66: case 67: case 68: case 69:
		return ARTIFACT_DEF; //handled
	case 6:
		return PANDORA_DEF; //hanled
	case 26:
		return EVENTOBJ_DEF; //handled
	case 33:
		return GARRISON_DEF; //handled
	case 34: case 70: case 62: //70 - random hero //62 - prison
		return HERO_DEF; //handled
	case 36:
		return GRAIL_DEF; //hanled
	case 53: case 17: case 18: case 19: case 20: case 42: case 87: case 220://cases 17 - 20 and 42 - tests
		return PLAYERONLY_DEF; //handled
	case 54: case 71: case 72: case 73: case 74: case 75: case 162: case 163: case 164:
		return CREATURES_DEF; //handled
	case 59:
		return SIGN_DEF; //handled
	case 77:
		return TOWN_DEF; //can be problematic, but handled
	case 79: case 76:
		return RESOURCE_DEF; //handled
	case 81:
		return SCHOLAR_DEF; //handled
	case 83:
		return SEERHUT_DEF; //handled
	case 91:
		return SIGN_DEF; //handled
	case 88: case 89: case 90:
		return SHRINE_DEF; //handled
	case 93:
		return SPELLSCROLL_DEF; //handled
	case 98:
		return TOWN_DEF; //handled
	case 113:
		return WITCHHUT_DEF; //handled
	case 214:
		return HEROPLACEHOLDER_DEF; //partially handled
	case 215:
		return BORDERGUARD_DEF; //handled by analogy to seer huts ;]
	case 216:
		return CREGEN2_DEF; //handled
	case 217:
		return CREGEN_DEF; //handled
	case 218:
		return CREGEN3_DEF; //handled
	case 219:
		return GARRISON_DEF; //handled
	default:
		return TERRAINOBJ_DEF; // nothing to be handled
	}
}
int readNormalNr (unsigned char * bufor, int pos, int bytCon = 4, bool cyclic = false)
{
	int ret=0;
	int amp=1;
	for (int ir=0; ir<bytCon; ir++)
	{
		ret+=bufor[pos+ir]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}
char readChar(unsigned char * bufor, int &i)
{
	return bufor[i++];
}
std::string readString(unsigned char * bufor, int &i)
{					
	int len = readNormalNr(bufor,i); i+=4;
	std::string ret; ret.reserve(len);
	for(int gg=0; gg<len; ++gg)
	{
		ret += bufor[i++];
	}
	return ret;
}
CCreatureSet readCreatureSet(unsigned char * bufor, int &i, int number, bool version) //version==true for >RoE maps
{
	if(version)
	{
		CCreatureSet ret;
		std::pair<ui32,si32> ins;
		for(int ir=0;ir<number;ir++)
		{
			int rettt = readNormalNr(bufor,i+ir*4, 2);
			if(rettt==0xffff) continue;
			if(rettt>32768)
				rettt = 65536-rettt+VLC->creh->creatures.size()-16;
			ins.first = rettt;
			ins.second = readNormalNr(bufor,i+ir*4+2, 2);
			std::pair<si32,std::pair<ui32,si32> > tt(ir,ins);
			ret.slots.insert(tt);
		}
		i+=number*4;
		return ret;
	}
	else
	{
		CCreatureSet ret;
		std::pair<ui32,si32> ins;
		for(int ir=0;ir<number;ir++)
		{
			int rettt = readNormalNr(bufor,i+ir*3, 1);
			if(rettt==0xff) continue;
			if(rettt>220)
				rettt = 256-rettt+VLC->creh->creatures.size()-16;
			ins.first = rettt;
			ins.second = readNormalNr(bufor,i+ir*3+1, 2);
			std::pair<si32,std::pair<ui32,si32> > tt(ir,ins);
			ret.slots.insert(tt);
		}
		i+=number*3;
		return ret;
	}
}
CMapHeader::CMapHeader(unsigned char *map)
{
	this->version = (Eformat)map[0]; //wersja mapy
	this->areAnyPLayers = map[4]; //seems to be invalid
	this->height = this->width = map[5]; // wymiary mapy
	this->twoLevel = map[9]; //czy sa lochy

	int length = map[10]; //name length
	int i=14, pom;
	while (i-14<length)	//read name
		this->name+=map[i++];
	length = map[i] + map[i+1]*256; //description length
	i+=4;
	for (pom=0;pom<length;pom++)
		this->description+=map[i++];
	this->difficulty = map[i++]; // reading map difficulty
	if(version!=RoE)
	{
		this->levelLimit = map[i++]; // hero level limit
	}
	else
	{
		levelLimit = 0;
	}
	for (pom=0;pom<8;pom++)
	{
		this->players[pom].canHumanPlay = map[i++];
		this->players[pom].canComputerPlay = map[i++];
		if ((!(this->players[pom].canHumanPlay || this->players[pom].canComputerPlay)))
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

		this->players[pom].AITactic = map[i++];

		if(version == SoD || version == WoG)
			i++;

		this->players[pom].allowedFactions = 0;
		this->players[pom].allowedFactions += map[i++];
		if(version != RoE)
			this->players[pom].allowedFactions += (map[i++])*256;

		this->players[pom].isFactionRandom = map[i++];
		this->players[pom].hasMainTown = map[i++];
		if (this->players[pom].hasMainTown)
		{
			if(version != RoE)
			{
				this->players[pom].generateHeroAtMainTown = map[i++];
				this->players[pom].generateHero = map[i++];
			}
			this->players[pom].posOfMainTown.x = map[i++];
			this->players[pom].posOfMainTown.y = map[i++];
			this->players[pom].posOfMainTown.z = map[i++];
		}
		players[pom].p8= map[i++];
		players[pom].p9= map[i++];
		if(players[pom].p9!=0xff)
		{
			players[pom].mainHeroPortrait = map[i++];
			int nameLength = map[i++];
			i+=3;
			for (int pp=0;pp<nameLength;pp++)
				players[pom].mainHeroName+=map[i++];
		}

		if(version!=RoE)
		{
			i++; ////heroes placeholders //domostwa
			int heroCount = map[i++];
			i+=3;
			for (int pp=0;pp<heroCount;pp++)
			{
				SheroName vv;
				vv.heroID=map[i++];
				int hnl = map[i++];
				i+=3;
				for (int zz=0;zz<hnl;zz++)
				{
					vv.heroName+=map[i++];
				}
				this->players[pom].heroesNames.push_back(vv);
			}
		}
	}
	this->victoryCondition = (EvictoryConditions)map[i++];
	if (this->victoryCondition != winStandard) //specific victory conditions
	{
		int nr;
		switch (this->victoryCondition) //read victory conditions
		{
		case artifact:
			{
				this->vicConDetails = new VicCon0();
				((VicCon0*)this->vicConDetails)->ArtifactID = map[i+2];
				nr=(version==RoE ? 1 : 2);
				break;
			}
		case gatherTroop:
			{
				this->vicConDetails = new VicCon1();
				int temp1 = map[i+2];
				int temp2 = map[i+3];
				((VicCon1*)this->vicConDetails)->monsterID = map[i+2];
				((VicCon1*)this->vicConDetails)->neededQuantity=readNormalNr(map, i+(version==RoE ? 3 : 4));
				nr=(version==RoE ? 5 : 6);
				break;
			}
		case gatherResource:
			{
				this->vicConDetails = new VicCon2();
				((VicCon2*)this->vicConDetails)->resourceID = map[i+2];
				((VicCon2*)this->vicConDetails)->neededQuantity=readNormalNr(map, i+3);
				nr=5;
				break;
			}
		case buildCity:
			{
				this->vicConDetails = new VicCon3();
				((VicCon3*)this->vicConDetails)->posOfCity.x = map[i+2];
				((VicCon3*)this->vicConDetails)->posOfCity.y = map[i+3];
				((VicCon3*)this->vicConDetails)->posOfCity.z = map[i+4];
				((VicCon3*)this->vicConDetails)->councilNeededLevel = map[i+5];
				((VicCon3*)this->vicConDetails)->fortNeededLevel = map[i+6];
				nr=5;
				break;
			}
		case buildGrail:
			{
				this->vicConDetails = new VicCon4();
				if (map[i+4]>2)
					((VicCon4*)this->vicConDetails)->anyLocation = true;
				else
				{
					((VicCon4*)this->vicConDetails)->whereBuildGrail.x = map[i+2];
					((VicCon4*)this->vicConDetails)->whereBuildGrail.y = map[i+3];
					((VicCon4*)this->vicConDetails)->whereBuildGrail.z = map[i+4];
				}
				nr=3;
				break;
			}
		case beatHero:
			{
				this->vicConDetails = new VicCon5();
				((VicCon5*)this->vicConDetails)->locationOfHero.x = map[i+2];
				((VicCon5*)this->vicConDetails)->locationOfHero.y = map[i+3];
				((VicCon5*)this->vicConDetails)->locationOfHero.z = map[i+4];
				nr=3;
				break;
			}
		case captureCity:
			{
				this->vicConDetails = new VicCon6();
				((VicCon6*)this->vicConDetails)->locationOfTown.x = map[i+2];
				((VicCon6*)this->vicConDetails)->locationOfTown.y = map[i+3];
				((VicCon6*)this->vicConDetails)->locationOfTown.z = map[i+4];
				nr=3;
				break;
			}
		case beatMonster:
			{
				this->vicConDetails = new VicCon7();
				((VicCon7*)this->vicConDetails)->locationOfMonster.x = map[i+2];
				((VicCon7*)this->vicConDetails)->locationOfMonster.y = map[i+3];
				((VicCon7*)this->vicConDetails)->locationOfMonster.z = map[i+4];
				nr=3;
				break;
			}
		case takeDwellings:
			{
				this->vicConDetails = new CspecificVictoryConidtions();
				nr=3;
				break;
			}
		case takeMines:
			{
				this->vicConDetails = new CspecificVictoryConidtions();
				nr=3;
				break;
			}
		case transportItem:
			{
				this->vicConDetails = new VicCona();
				((VicCona*)this->vicConDetails)->artifactID =  map[i+2];
				((VicCona*)this->vicConDetails)->destinationPlace.x = map[i+3];
				((VicCona*)this->vicConDetails)->destinationPlace.y = map[i+4];
				((VicCona*)this->vicConDetails)->destinationPlace.z = map[i+5];
				nr=4;
				break;
			}
		}
		this->vicConDetails->allowNormalVictory = map[i++];
		this->vicConDetails->appliesToAI = map[i++];
		i+=nr;
	}
	this->lossCondition.typeOfLossCon = (ElossCon)map[i++];
	switch (this->lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
		  {
			  this->lossCondition.castlePos.x=map[i++];
			  this->lossCondition.castlePos.y=map[i++];
			  this->lossCondition.castlePos.z=map[i++];
		  }
	case lossHero:
		  {
			  this->lossCondition.heroPos.x=map[i++];
			  this->lossCondition.heroPos.y=map[i++];
			  this->lossCondition.heroPos.z=map[i++];
		  }
	case timeExpires:
		{
			this->lossCondition.timeLimit = readNormalNr(map, i++,2);
			i++;
		}
	}
	this->howManyTeams=map[i++]; //read number of teams
	if(this->howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
			this->players[rr].team=map[i++];
		}
	}
}
void Mapa::initFromBytes(unsigned char * bufor)
{
	timeHandler th;
	th.getDif();
	int i=0;
	readHeader(bufor, i);
	tlog0<<"\tReading header: "<<th.getDif()<<std::endl;

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
	delete[] bufor; bufor=NULL;

	
	for(int f=0; f<objects.size(); ++f) //calculationg blocked / visitable positions
	{
		if(!objects[f]->defInfo)
			continue;
		addBlockVisTiles(objects[f]);
	}
	tlog0<<"\tCalculating blocked/visitable tiles: "<<th.getDif()<<std::endl;
}	
void Mapa::removeBlockVisTiles(CGObjectInstance * obj)
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
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(!((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
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
{
	tlog0<<"Opening map file: "<<filename<<"\t "<<std::flush;
	gzFile map = gzopen(filename.c_str(),"rb");
	std::vector<unsigned char> mapstr; int pom;
	while((pom=gzgetc(map))>=0)
	{
		mapstr.push_back(pom);
	}
	gzclose(map);
	unsigned char *initTable = new unsigned char[mapstr.size()];
	for(int ss=0; ss<mapstr.size(); ++ss)
	{
		initTable[ss] = mapstr[ss];
	}
	tlog0<<"done."<<std::endl;
	boost::crc_32_type  result;
	result.process_bytes(initTable,mapstr.size());
	checksum = result.checksum();
	tlog0 << "\tOur map checksum: "<<result.checksum() << std::endl;
	initFromBytes(initTable);
}

CGHeroInstance * Mapa::getHero(int ID, int mode)
{
	if (mode != 0)
#ifndef __GNUC__
		throw new std::exception("gs->getHero: This mode is not supported!");
#else
		throw new std::exception();
#endif

	for(int i=0; i<heroes.size();i++)
		if(heroes[i]->subID == ID)
			return heroes[i];
	return NULL;
}

int Mapa::loadSeerHut( unsigned char * bufor, int i, CGObjectInstance * nobj )
{
	CSeerHutObjInfo * spec = new CSeerHutObjInfo;
	if(version>RoE)
	{
		spec->missionType = bufor[i]; ++i;
		switch(spec->missionType)
		{
		case 0:
			i+=3;
			return i;
		case 1:
			{
				spec->m1level = readNormalNr(bufor,i); i+=4;
				int limit = readNormalNr(bufor,i); i+=4;
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
				int limit = readNormalNr(bufor,i); i+=4;
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
				int limit = readNormalNr(bufor,i); i+=4;
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
				int limit = readNormalNr(bufor,i); i+=4;
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
					int artid = readNormalNr(bufor,i, 2); i+=2;
					spec->m5arts.push_back(artid);
				}
				int limit = readNormalNr(bufor,i); i+=4;
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
					int creType = readNormalNr(bufor,i, 2); i+=2;
					int creNumb = readNormalNr(bufor,i, 2); i+=2;
					spec->m6cre.push_back(&(VLC->creh->creatures[creType]));
					spec->m6number.push_back(creNumb);
				}
				int limit = readNormalNr(bufor,i); i+=4;
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
				spec->m7wood = readNormalNr(bufor,i); i+=4;
				spec->m7mercury = readNormalNr(bufor,i); i+=4;
				spec->m7ore = readNormalNr(bufor,i); i+=4;
				spec->m7sulfur = readNormalNr(bufor,i); i+=4;
				spec->m7crystal = readNormalNr(bufor,i); i+=4;
				spec->m7gems = readNormalNr(bufor,i); i+=4;
				spec->m7gold = readNormalNr(bufor,i); i+=4;
				int limit = readNormalNr(bufor,i); i+=4;
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
				spec->m8hero = heroType;
				int limit = readNormalNr(bufor,i); i+=4;
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
				int limit = readNormalNr(bufor,i); i+=4;
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

		int len1 = readNormalNr(bufor,i); i+=4;
		for(int ee=0; ee<len1; ++ee)
		{
			spec->firstVisitText += bufor[i]; ++i;
		}

		int len2 = readNormalNr(bufor,i); i+=4;
		for(int ee=0; ee<len2; ++ee)
		{
			spec->nextVisitText += bufor[i]; ++i;
		}

		int len3 = readNormalNr(bufor,i); i+=4;
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
			spec->m5arts.push_back(artID);
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
				spec->r1exp = readNormalNr(bufor,i); i+=4;
				break;
			}
		case 2:
			{
				spec->r2mana = readNormalNr(bufor,i); i+=4;
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
				spec->r5amount = readNormalNr(bufor,i, 3); i+=3;
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
				spec->r7ability = bufor[i]; ++i;
				spec->r7level = bufor[i]; ++i;
				break;
			}
		case 8:
			{
				spec->r8art = readNormalNr(bufor,i, (version == RoE ? 1 : 2)); i+=(version == RoE ? 1 : 2);
				break;
			}
		case 9:
			{
				spec->r9spell = bufor[i]; ++i;
				break;
			}
		case 10:
			{
				if(version>RoE)
				{
					spec->r10creature = readNormalNr(bufor,i, 2); i+=2;
					spec->r10amount = readNormalNr(bufor,i, 2); i+=2;
				}
				else
				{
					spec->r10creature = bufor[i]; ++i;
					spec->r10amount = readNormalNr(bufor,i, 2); i+=2;
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
	nobj->info = spec;
	return i;
}

void Mapa::loadTown( CGObjectInstance * &nobj, unsigned char * bufor, int &i )
{
	CGTownInstance * nt = new CGTownInstance();
	(*(static_cast<CGObjectInstance*>(nt))) = *nobj;
	delete nobj;
	nobj = nt;
	nt->identifier = 0;
	if(version>RoE)
	{	
		readNormalNr(bufor,i); i+=4;
	}
	nt->tempOwner = bufor[i]; ++i;
	if(readChar(bufor,i)) //has name
		nt->name = readString(bufor,i);
	if(readChar(bufor,i))//true if garrison isn't empty
		nt->army = readCreatureSet(bufor,i,7,(version>RoE));
	nt->army.formation = bufor[i]; ++i;
	if(readChar(bufor,i)) //unusualBuildings
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
		nt->builtBuildings = convertBuildings(nt->builtBuildings,nt->subID);
		nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings,nt->subID);
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
		for(i; i<ist+9; ++i)
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
	for(i; i<ist+9; ++i)
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
		CCastleEvent nce;
		int nameLen = readNormalNr(bufor,i); i+=4;
		for(int ll=0; ll<nameLen; ++ll)
		{
			nce.name += bufor[i]; ++i;
		}

		int messLen = readNormalNr(bufor,i); i+=4;
		for(int ll=0; ll<messLen; ++ll)
		{
			nce.message += bufor[i]; ++i;
		}

		nce.wood = readNormalNr(bufor,i); i+=4;
		nce.mercury = readNormalNr(bufor,i); i+=4;
		nce.ore = readNormalNr(bufor,i); i+=4;
		nce.sulfur = readNormalNr(bufor,i); i+=4;
		nce.crystal = readNormalNr(bufor,i); i+=4;
		nce.gems = readNormalNr(bufor,i); i+=4;
		nce.gold = readNormalNr(bufor,i); i+=4;

		nce.players = bufor[i]; ++i;
		if(version > AB)
		{
			nce.forHuman = bufor[i]; ++i;
		}
		else
			nce.forHuman = true;
		nce.forComputer = bufor[i]; ++i;
		nce.firstShow = readNormalNr(bufor,i, 2); i+=2;
		nce.forEvery = bufor[i]; ++i;

		i+=17;

		for(int kk=0; kk<6; ++kk)
		{
			nce.bytes[kk] = bufor[i]; ++i;
		}

		for(int vv=0; vv<7; ++vv)
		{
			nce.gen[vv] = readNormalNr(bufor,i, 2); i+=2;
		}
		i+=4;
		nt->events.insert(nce);
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
	if(nt->ID==98)
		towns.push_back(nt);
}

void Mapa::loadHero( CGObjectInstance * &nobj, unsigned char * bufor, int &i )
{
	CGHeroInstance * nhi = new CGHeroInstance;
	int identifier = 0;
	if(version>RoE)
	{
		identifier = readNormalNr(bufor,i, 4); i+=4;
	}
	nobj->setOwner(bufor[i]); ++i;
	nobj->subID = readNormalNr(bufor,i, 1); ++i;	
	
	for(int j=0; j<predefinedHeroes.size(); j++)
	{
		if(predefinedHeroes[j]->subID == nobj->subID)
		{
			*nhi = *predefinedHeroes[j];
			break;
		}
	}

	(*(static_cast<CGObjectInstance*>(nhi))) = *nobj;
	delete nobj;
	nobj=nhi;
	nhi->portrait = nhi->subID;

	for(int j=0; j<disposedHeroes.size(); j++)
	{
		if(disposedHeroes[j].ID == nhi->subID)
		{
			nhi->name = disposedHeroes[j].name;
			nhi->portrait = disposedHeroes[j].portrait;
			break;
		}
	}
	nhi->identifier = identifier;
	if(readChar(bufor,i))//true if hero has nonstandard name
		nhi->name = readString(bufor,i);
	if(version>AB)
	{
		if(readChar(bufor,i))//true if hore's experience is greater than 0
		{	nhi->exp = readNormalNr(bufor,i); i+=4;	}
		else
			nhi->exp = 0xffffffff;
	}
	else
	{	nhi->exp = readNormalNr(bufor,i); i+=4;	}

	bool portrait=bufor[i]; ++i;
	if (portrait)
		nhi->portrait = readChar(bufor,i);
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
		nhi->army = readCreatureSet(bufor,i,7,(version>RoE));
	nhi->army.formation =bufor[i]; ++i; //formation
	bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
	int artmask = version == RoE ? 0xff : 0xffff;
	int artidlen = version == RoE ? 1 : 2;
	if(artSet)
	{
		for(int pom=0;pom<16;pom++)
		{
			int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
			if(id!=artmask)
				nhi->artifWorn[pom] = id;
		}
		//misc5 art //17
		if(version>=SoD)
		{
			int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
			if(id!=artmask)
				nhi->artifWorn[16] = id;
		}
		//spellbook
		int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
		if(id!=artmask)
			nhi->artifWorn[17] = id;
		//19 //???what is that? gap in file or what? - it's probably fifth slot..
		if(version>RoE)
		{
			id = readNormalNr(bufor,i, artidlen); i+=artidlen;
			if(id!=artmask)
				nhi->artifWorn[18] = id;
		}
		else
			i+=1;
		//bag artifacts //20
		int amount = readNormalNr(bufor,i, 2); i+=2; //number of artifacts in hero's bag
		if(amount>0)
		{
			for(int ss=0; ss<amount; ++ss)
			{
				id = readNormalNr(bufor,i, artidlen); i+=artidlen;
				if(id!=artmask)
					nhi->artifacts.push_back(id);
			}
		}
	} //artifacts

	nhi->patrol.patrolRadious = readNormalNr(bufor,i, 1); ++i;
	if(nhi->patrol.patrolRadious == 0xff)
		nhi->patrol.patrolling = false;
	else 
		nhi->patrol.patrolling = true;

	if(version>RoE)
	{
		if(readChar(bufor,i))//true if hero has nonstandard (mapmaker defined) biography
			nhi->biography = readString(bufor,i);
		nhi->sex = !(bufor[i]); ++i;
	}
	//spells
	if(version>AB)
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
		if(buff!=254)
		{
			nhi->spells.insert(buff);
		}
	}
	//spells loaded
	if(version>AB)
	{
		if(readChar(bufor,i))//customPrimSkills
		{
			nhi->primSkills.resize(4);
			for(int xx=0;xx<4;xx++)
				nhi->primSkills[xx] = bufor[i++];
		}
	}
	i+=16;
	nhi->moveDir = 4;
	nhi->isStanding = true;
	nhi->level = -1;
	nhi->mana = -1;
	nhi->movement = -1;
	if(nhi->ID==34)
		heroes.push_back(nhi);
}

void Mapa::loadPlayerInfo( int &pom, unsigned char * bufor, int &i )
{
	for (pom=0;pom<8;pom++)
	{
		players[pom].canHumanPlay = bufor[i++];
		players[pom].canComputerPlay = bufor[i++];
		if ((!(players[pom].canHumanPlay || players[pom].canComputerPlay)))
		{
			memset(&players[pom],0,sizeof(PlayerInfo));
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
				players[pom].generateHeroAtMainTown = false;
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
			int nameLength = bufor[i++];
			i+=3; 
			for (int pp=0;pp<nameLength;pp++)
				players[pom].mainHeroName+=bufor[i++];
		}

		if(version != RoE)
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
				players[pom].heroesNames.push_back(vv);
			}
		}
	}
}

void Mapa::loadViCLossConditions( unsigned char * bufor, int &i)
{
	victoryCondition = (EvictoryConditions)bufor[i++];
	if (victoryCondition != winStandard) //specific victory conditions
	{
		int nr;
		switch (victoryCondition) //read victory conditions
		{
		case artifact:
			{
				vicConDetails = new VicCon0();
				((VicCon0*)vicConDetails)->ArtifactID = bufor[i+2];
				nr=(version==RoE ? 1 : 2);
				break;
			}
		case gatherTroop:
			{
				vicConDetails = new VicCon1();
				int temp1 = bufor[i+2];
				int temp2 = bufor[i+3];
				((VicCon1*)vicConDetails)->monsterID = bufor[i+2];
				((VicCon1*)vicConDetails)->neededQuantity=readNormalNr(bufor,i+(version==RoE ? 3 : 4));
				nr=(version==RoE ? 5 : 6);
				break;
			}
		case gatherResource:
			{
				vicConDetails = new VicCon2();
				((VicCon2*)vicConDetails)->resourceID = bufor[i+2];
				((VicCon2*)vicConDetails)->neededQuantity=readNormalNr(bufor,i+3);
				nr=5;
				break;
			}
		case buildCity:
			{
				vicConDetails = new VicCon3();
				((VicCon3*)vicConDetails)->posOfCity.x = bufor[i+2];
				((VicCon3*)vicConDetails)->posOfCity.y = bufor[i+3];
				((VicCon3*)vicConDetails)->posOfCity.z = bufor[i+4];
				((VicCon3*)vicConDetails)->councilNeededLevel = bufor[i+5];
				((VicCon3*)vicConDetails)->fortNeededLevel = bufor[i+6];
				nr=5;
				break;
			}
		case buildGrail:
			{
				vicConDetails = new VicCon4();
				if (bufor[i+4]>2)
					((VicCon4*)vicConDetails)->anyLocation = true;
				else
				{
					((VicCon4*)vicConDetails)->whereBuildGrail.x = bufor[i+2];
					((VicCon4*)vicConDetails)->whereBuildGrail.y = bufor[i+3];
					((VicCon4*)vicConDetails)->whereBuildGrail.z = bufor[i+4];
				}
				nr=3;
				break;
			}
		case beatHero:
			{
				vicConDetails = new VicCon5();
				((VicCon5*)vicConDetails)->locationOfHero.x = bufor[i+2];
				((VicCon5*)vicConDetails)->locationOfHero.y = bufor[i+3];
				((VicCon5*)vicConDetails)->locationOfHero.z = bufor[i+4];				
				nr=3;
				break;
			}
		case captureCity:
			{
				vicConDetails = new VicCon6();
				((VicCon6*)vicConDetails)->locationOfTown.x = bufor[i+2];
				((VicCon6*)vicConDetails)->locationOfTown.y = bufor[i+3];
				((VicCon6*)vicConDetails)->locationOfTown.z = bufor[i+4];				
				nr=3;
				break;
			}
		case beatMonster:
			{
				vicConDetails = new VicCon7();
				((VicCon7*)vicConDetails)->locationOfMonster.x = bufor[i+2];
				((VicCon7*)vicConDetails)->locationOfMonster.y = bufor[i+3];
				((VicCon7*)vicConDetails)->locationOfMonster.z = bufor[i+4];				
				nr=3;
				break;
			}
		case takeDwellings:
			{		
				vicConDetails = new CspecificVictoryConidtions();
				nr=0;
				break;
			}
		case takeMines:
			{	
				vicConDetails = new CspecificVictoryConidtions();	
				nr=0;
				break;
			}
		case transportItem:
			{
				vicConDetails = new VicCona();
				((VicCona*)vicConDetails)->artifactID =  bufor[i+2];
				((VicCona*)vicConDetails)->destinationPlace.x = bufor[i+3];
				((VicCona*)vicConDetails)->destinationPlace.y = bufor[i+4];
				((VicCona*)vicConDetails)->destinationPlace.z = bufor[i+5];				
				nr=4;
				break;
			}
		}
		vicConDetails->allowNormalVictory = bufor[i++];
		vicConDetails->appliesToAI = bufor[i++];
		i+=nr;
	}
	lossCondition.typeOfLossCon = (ElossCon)bufor[i++];
	switch (lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
		{
			lossCondition.castlePos.x=bufor[i++];
			lossCondition.castlePos.y=bufor[i++];
			lossCondition.castlePos.z=bufor[i++];
			break;
		}
	case lossHero:
		{
			lossCondition.heroPos.x=bufor[i++];
			lossCondition.heroPos.y=bufor[i++];
			lossCondition.heroPos.z=bufor[i++];
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

void Mapa::readRumors( unsigned char * bufor, int &i)
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

void Mapa::readHeader( unsigned char * bufor, int &i)
{
	version = (Eformat)(readNormalNr(bufor,i)); i+=4; //map version
	areAnyPLayers = readChar(bufor,i); //invalid on some maps
	height = width = (readNormalNr(bufor,i)); i+=4; // wymiary mapy
	twoLevel = readChar(bufor,i); //czy sa lochy
	terrain = new TerrainTile**[width]; // allocate memory 
	for (int ii=0;ii<width;ii++)
	{
		terrain[ii] = new TerrainTile*[height]; // allocate memory 
		for(int jj=0;jj<height;jj++)
			terrain[ii][jj] = new TerrainTile[twoLevel+1];
	}
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
			players[rr].team=bufor[i++];
		}
	}
	//reading allowed heroes (20 bytes)
	int ist;

	ist=i; //starting i for loop

	allowedHeroes.resize(HEROES_QUANTITY);
	for(int xx=0;xx<HEROES_QUANTITY;xx++)
		allowedHeroes[xx] = true;

	for(i; i<ist+ (version == RoE ? 16 : 20) ; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
			if((i-ist)*8+yy < HEROES_QUANTITY)
				if(c != (c|((unsigned char)intPow(2, yy))))
					allowedHeroes[(i-ist)*8+yy] = false;
	}
	if(version>RoE) //probably reserved for further heroes
		i+=4;
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
			//int players = bufor[i++];
			//for(int zz=0;zz<8;zz++)
			//{
			//	int por = (1<<zz);
			//	if(players & por)
			//		disposedHeroes[g].players[zz] = true;
			//	else 
			//		disposedHeroes[g].players[zz] = false;
			//}
		}
	}

	i+=31; //omitting NULLS

	allowedArtifact.resize(ARTIFACTS_QUANTITY);
	for(int x=0;x<allowedArtifact.size();x++)
		allowedArtifact[x] = true;

	//reading allowed artifacts:  17 or 18 bytes
	if(version!=RoE)
	{
		ist=i; //starting i for loop
		for(i; i<ist+(version==AB ? 17 : 18); ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < ARTIFACTS_QUANTITY)
				{
					if(c == (c|((unsigned char)intPow(2, yy))))
						allowedArtifact[(i-ist)*8+yy] = false;
				}
			}
		}//allowed artifacts have been read
	}


	allowedSpell.resize(SPELLS_QUANTITY);
	for(int x=0;x<allowedSpell.size();x++)
		allowedSpell[x] = true;

	allowedAbilities.resize(SKILL_QUANTITY);
	for(int x=0;x<allowedAbilities.size();x++)
		allowedAbilities[x] = true;

	if(version>=SoD)
	{
		//reading allowed spells (9 bytes)
		ist=i; //starting i for loop
		for(i; i<ist+9; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
				if((i-ist)*8+yy < SPELLS_QUANTITY)
					if(c == (c|((unsigned char)intPow(2, yy))))
						allowedSpell[(i-ist)*8+yy] = false;
		}


		//allowed hero's abilities (4 bytes)
		ist=i; //starting i for loop
		for(i; i<ist+4; ++i)
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

void Mapa::readPredefinedHeroes( unsigned char * bufor, int &i)
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
				cgh->ID = 34;
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
				bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
				int artmask = version == RoE ? 0xff : 0xffff;
				int artidlen = version == RoE ? 1 : 2;
				if(artSet)
				{
					for(int pom=0;pom<16;pom++)
					{
						int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
						if(id!=artmask)
							cgh->artifWorn[pom] = id;
					}
					//misc5 art //17
					if(version>=SoD)
					{
						i+=2;
						//int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
						//if(id!=artmask)
						//	spec->artifWorn[16] = id;
					}
					//spellbook
					int id = readNormalNr(bufor,i, artidlen); i+=artidlen;
					if(id!=artmask)
						cgh->artifWorn[17] = id;
					//19 //???what is that? gap in file or what? - it's probably fifth slot..
					if(version>RoE)
					{
						id = readNormalNr(bufor,i, artidlen); i+=artidlen;
						if(id!=artmask)
							cgh->artifWorn[18] = id;
					}
					else
						i+=1;
					//bag artifacts //20
					int amount = readNormalNr(bufor,i, 2); i+=2; //number of artifacts in hero's bag
					if(amount>0)
					{
						for(int ss=0; ss<amount; ++ss)
						{
							id = readNormalNr(bufor,i, artidlen); i+=artidlen;
							if(id!=artmask)
								cgh->artifacts.push_back(id);
						}
					}
				} //artifacts
				if(readChar(bufor,i))//customBio
					cgh->biography = readString(bufor,i);
				int sex = bufor[i++]; // 0xFF is default, 00 male, 01 female
				if(readChar(bufor,i))//are spells
				{
					int ist = i;
					for(i; i<ist+9; ++i)
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
					cgh->primSkills.resize(4);
					for(int xx=0;xx<4;xx++)
						cgh->primSkills[xx] = bufor[i++];
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

void Mapa::readTerrain( unsigned char * bufor, int &i)
{
	for (int c=0; c<width; c++) // reading terrain
	{
		for (int z=0; z<height; z++)
		{
			terrain[z][c][0].tertype = (EterrainType)(bufor[i++]);
			terrain[z][c][0].terview = bufor[i++];
			terrain[z][c][0].nuine = (Eriver)bufor[i++];
			terrain[z][c][0].rivDir = bufor[i++];
			terrain[z][c][0].malle = (Eroad)bufor[i++];
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
				terrain[z][c][1].tertype = (EterrainType)(bufor[i++]);
				terrain[z][c][1].terview = bufor[i++];
				terrain[z][c][1].nuine = (Eriver)bufor[i++];
				terrain[z][c][1].rivDir = bufor[i++];
				terrain[z][c][1].malle = (Eroad)bufor[i++];
				terrain[z][c][1].roadDir = bufor[i++];
				terrain[z][c][1].siodmyTajemniczyBajt = bufor[i++];
				terrain[z][c][1].blocked = 0;
				terrain[z][c][1].visitable = 0;
			}
		}
	}
}

void Mapa::readDefInfo( unsigned char * bufor, int &i)
{
	int defAmount = readNormalNr(bufor,i); i+=4;
	defy.reserve(defAmount);
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
		if(vinya->id!=34 && vinya->id!=70)
			vinya->visitDir = VLC->dobjinfo->gobjs[vinya->id][vinya->subid]->visitDir;
		else
			vinya->visitDir = 0xff;
		defy.push_back(vinya); // add this def to the vector
	}
}

void Mapa::readObjects( unsigned char * bufor, int &i)
{
	int howManyObjs = readNormalNr(bufor,i, 4); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		//std::cout << "object nr "<<ww<<"\ti= "<<i<<std::endl;
		CGObjectInstance * nobj = new CGObjectInstance(); //we will read this object
		nobj->id = objects.size();
		nobj->pos.x = bufor[i++];
		nobj->pos.y = bufor[i++];
		nobj->pos.z = bufor[i++];

		int tempd = readNormalNr(bufor,i, 4); i+=4;
		nobj->defInfo = defy[tempd];
		nobj->ID = nobj->defInfo->id;
		nobj->subID = nobj->defInfo->subid;

		//if (((nobj.x==0)&&(nobj.y==0)) || nobj.x>width || nobj.y>height || nobj.z>1 || nobj.defNumber>defy.size())	std::cout << "Alarm!!! Obiekt "<<ww<<" jest kopniety (lub wystaje poza mape)\n";

		i+=5;
		unsigned char buff [30];
		for(int ccc=0; ccc<30; ++ccc)
		{
			buff[ccc] = bufor[i+ccc];
		}
		int j = nobj->defInfo->id;
		int p = 99;
		switch(getDefType(nobj->defInfo))
		{
		case EVENTOBJ_DEF: //for event - objects
			{
				CEventObjInfo * spec = new CEventObjInfo;
				bool guardMess;
				guardMess = bufor[i]; ++i;
				if(guardMess)
				{
					int messLong = readNormalNr(bufor,i, 4); i+=4;
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
						spec->guarders = readCreatureSet(bufor,i,7,(version>RoE)); 
					}
					i+=4;
				}
				else
				{
					spec->isMessage = false;
					spec->areGuarders = false;
					spec->message = std::string("");
				}
				spec->gainedExp = readNormalNr(bufor,i, 4); i+=4;
				spec->manaDiff = readNormalNr(bufor,i, 4); i+=4;
				spec->moraleDiff = readNormalNr(bufor,i, 1, true); ++i;
				spec->luckDiff = readNormalNr(bufor,i, 1, true); ++i;
				spec->wood = readNormalNr(bufor,i); i+=4;
				spec->mercury = readNormalNr(bufor,i); i+=4;
				spec->ore = readNormalNr(bufor,i); i+=4;
				spec->sulfur = readNormalNr(bufor,i); i+=4;
				spec->crystal = readNormalNr(bufor,i); i+=4;
				spec->gems = readNormalNr(bufor,i); i+=4;
				spec->gold = readNormalNr(bufor,i); i+=4;
				spec->attack = readNormalNr(bufor,i, 1); ++i;
				spec->defence = readNormalNr(bufor,i, 1); ++i;
				spec->power = readNormalNr(bufor,i, 1); ++i;
				spec->knowledge = readNormalNr(bufor,i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(bufor,i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back(readNormalNr(bufor,i, 1)); ++i;
					spec->abilityLevels.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gart = readNormalNr(bufor,i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					spec->artifacts.push_back(readNormalNr(bufor,i, (version == RoE ? 1 : 2))); i+=(version == RoE ? 1 : 2);
				}
				int gspel = readNormalNr(bufor,i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gcre = readNormalNr(bufor,i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(bufor,i,gcre,(version>RoE));
				i+=8;
				spec->availableFor = readNormalNr(bufor,i, 1); ++i;
				spec->computerActivate = readNormalNr(bufor,i, 1); ++i;
				spec->humanActivate = readNormalNr(bufor,i, 1); ++i;
				i+=4;
				nobj->info = spec;
				break;
			}
		case HERO_DEF:
			{
				loadHero(nobj, bufor, i);
				break;
			}
		case CREATURES_DEF:
			{
				CCreatureObjInfo * spec = new CCreatureObjInfo;
				if(version>RoE)
				{
					spec->bytes[0] = bufor[i]; ++i;
					spec->bytes[1] = bufor[i]; ++i;
					spec->bytes[2] = bufor[i]; ++i;
					spec->bytes[3] = bufor[i]; ++i;
				}
				spec->number = readNormalNr(bufor,i, 2); i+=2;
				spec->character = bufor[i]; ++i;
				bool isMesTre = bufor[i]; ++i; //true if there is message or treasury
				if(isMesTre)
				{
					int messLength = readNormalNr(bufor,i); i+=4;
					if(messLength>0)
					{
						for(int tt=0; tt<messLength; ++tt)
						{
							spec->message += bufor[i]; ++i;
						}
					}
					spec->wood = readNormalNr(bufor,i); i+=4;
					spec->mercury = readNormalNr(bufor,i); i+=4;
					spec->ore = readNormalNr(bufor,i); i+=4;
					spec->sulfur = readNormalNr(bufor,i); i+=4;
					spec->crytal = readNormalNr(bufor,i); i+=4;
					spec->gems = readNormalNr(bufor,i); i+=4;
					spec->gold = readNormalNr(bufor,i); i+=4;
					int artID = readNormalNr(bufor,i, (version == RoE ? 1 : 2)); i+=(version == RoE ? 1 : 2);
					if(version==RoE)
					{
						if(artID!=0xff)
							spec->gainedArtifact = artID;
						else
							spec->gainedArtifact = -1;
					}
					else
					{
						if(artID!=0xffff)
							spec->gainedArtifact = artID;
						else
							spec->gainedArtifact = -1;
					}
				}
				spec->neverFlees = bufor[i]; ++i;
				spec->notGrowingTeam = bufor[i]; ++i;
				i+=2;
				nobj->info = spec;
				break;
			}
		case SIGN_DEF:
			{
				CSignObjInfo * spec = new CSignObjInfo;
				int length = readNormalNr(bufor,i); i+=4;
				for(int rr=0; rr<length; ++rr)
				{
					spec->message += bufor[i]; ++i;
				}
				i+=4;
				nobj->info = spec;
				break;
			}
		case SEERHUT_DEF:
			{
				i = loadSeerHut(bufor, i, nobj);
				break;
			}
		case WITCHHUT_DEF:
			{
				CWitchHutObjInfo * spec = new CWitchHutObjInfo;
				if(version>RoE) //in reo we cannot specify it - all are allowed (I hope)
				{
					int ist=i; //starting i for loop
					for(i; i<ist+4; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < SKILL_QUANTITY)
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									spec->allowedAbilities.push_back((i-ist)*8+yy);
							}
						}
					}
				}
				else //(RoE map)
				{
					for(int gg=0; gg<SKILL_QUANTITY; ++gg)
					{
						spec->allowedAbilities.push_back(gg);
					}
				}

				nobj->info = spec;
				break;
			}
		case SCHOLAR_DEF:
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
					spec->r1 = bufor[i]; ++i;
					break;
				case 2:
					spec->r2 = bufor[i]; ++i;
					break;
				}
				i+=6;
				nobj->info = spec;
				break;
			}
		case GARRISON_DEF:
			{
				CGarrisonObjInfo * spec = new CGarrisonObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->units = readCreatureSet(bufor,i,7,(version>RoE));
				if(version > RoE)
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
		case ARTIFACT_DEF:
			{
				CArtifactObjInfo * spec = new CArtifactObjInfo;
				bool areSettings = bufor[i]; ++i;
				if(areSettings)
				{
					int messLength = readNormalNr(bufor,i, 4); i+=4;
					for(int hh=0; hh<messLength; ++hh)
					{
						spec->message += bufor[i]; ++i;
					}
					bool areGuards = bufor[i]; ++i;
					if(areGuards)
					{
						spec->areGuards = true;
						spec->guards = readCreatureSet(bufor,i,7,(version>RoE));
					}
					else
						spec->areGuards = false;
					i+=4;
				}
				nobj->info = spec;
				break;
			}
		case RESOURCE_DEF:
			{
				CResourceObjInfo * spec = new CResourceObjInfo;
				bool isMessGuard = bufor[i]; ++i;
				if(isMessGuard)
				{
					int messLength = readNormalNr(bufor,i); i+=4;
					for(int mm=0; mm<messLength; ++mm)
					{
						spec->message+=bufor[i]; ++i;
					}
					spec->areGuards = bufor[i]; ++i;
					if(spec->areGuards)
					{
						spec->guards = readCreatureSet(bufor,i,7,(version>RoE));
					}
					i+=4;
				}
				else
				{
					spec->areGuards = false;
				}
				spec->amount = readNormalNr(bufor,i); i+=4;
				i+=4;
				nobj->info = spec;
				break;
			}
		case TOWN_DEF:
			{
				loadTown(nobj, bufor, i);
				break;
			}
		case PLAYERONLY_DEF:
			{
				CPlayerOnlyObjInfo * spec = new CPlayerOnlyObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				nobj->setOwner(spec->player);
				nobj->info = spec;
				break;
			}
		case SHRINE_DEF:
			{
				CShrineObjInfo * spec = new CShrineObjInfo;
				spec->spell = bufor[i]; i+=4;
				nobj->info = spec;
				break;
			}
		case SPELLSCROLL_DEF:
			{
				CSpellScrollObjinfo * spec = new CSpellScrollObjinfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(bufor,i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(bufor,i,7,(version>RoE));
					}
					i+=4;
				}
				spec->spell = bufor[i]; ++i;
				i+=3;
				nobj->info = spec;
				break;
			}
		case PANDORA_DEF:
			{
				CPandorasBoxObjInfo * spec = new CPandorasBoxObjInfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(bufor,i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(bufor,i,7,(version>RoE));
					}
					i+=4;
				}
				////// copied form event handling (seems to be similar)
				spec->gainedExp = readNormalNr(bufor,i, 4); i+=4;
				spec->manaDiff = readNormalNr(bufor,i, 4); i+=4;
				spec->moraleDiff = readNormalNr(bufor,i, 1, true); ++i;
				spec->luckDiff = readNormalNr(bufor,i, 1, true); ++i;
				spec->wood = readNormalNr(bufor,i); i+=4;
				spec->mercury = readNormalNr(bufor,i); i+=4;
				spec->ore = readNormalNr(bufor,i); i+=4;
				spec->sulfur = readNormalNr(bufor,i); i+=4;
				spec->crystal = readNormalNr(bufor,i); i+=4;
				spec->gems = readNormalNr(bufor,i); i+=4;
				spec->gold = readNormalNr(bufor,i); i+=4;
				spec->attack = readNormalNr(bufor,i, 1); ++i;
				spec->defence = readNormalNr(bufor,i, 1); ++i;
				spec->power = readNormalNr(bufor,i, 1); ++i;
				spec->knowledge = readNormalNr(bufor,i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(bufor,i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back(readNormalNr(bufor,i, 1)); ++i;
					spec->abilityLevels.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gart = readNormalNr(bufor,i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					if(version > RoE)
					{
						spec->artifacts.push_back(readNormalNr(bufor,i, 2)); i+=2;
					}
					else
					{
						spec->artifacts.push_back(readNormalNr(bufor,i, 1)); i+=1;
					}
				}
				int gspel = readNormalNr(bufor,i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(readNormalNr(bufor,i, 1)); ++i;
				}
				int gcre = readNormalNr(bufor,i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(bufor,i,gcre,(version>RoE));
				i+=8;
				nobj->info = spec;
				///////end of copied fragment
				break;
			}
		case GRAIL_DEF:
			{
				CGrailObjInfo * spec = new CGrailObjInfo;
				spec->radius = readNormalNr(bufor,i); i+=4;
				nobj->info = spec;
				break;
			}
		case CREGEN_DEF:
			{
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
				nobj->info = spec;
				break;
			}
		case CREGEN2_DEF:
			{
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
				//if(spec->maxLevel>7)
				//	spec->maxLevel = 7;
				//if(spec->minLevel<1)
				//	spec->minLevel = 1;
				nobj->setOwner(spec->player);
				nobj->info = spec;
				break;
			}
		case CREGEN3_DEF:
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
		case BORDERGUARD_DEF:
			{
				CBorderGuardObjInfo * spec = new CBorderGuardObjInfo;
				spec->missionType = bufor[i]; ++i;
				int len1, len2, len3;
				switch(spec->missionType)
				{
				case 0:
					{
						goto borderguardend;
						break;
					}
				case 1:
					{
						spec->m1level = readNormalNr(bufor,i); i+=4;
						int limit = readNormalNr(bufor,i); i+=4;
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
						int limit = readNormalNr(bufor,i); i+=4;
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
						int limit = readNormalNr(bufor,i); i+=4;
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
						int limit = readNormalNr(bufor,i); i+=4;
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
							spec->m5arts.push_back(readNormalNr(bufor,i, 2)); i+=2;
						}
						int limit = readNormalNr(bufor,i); i+=4;
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
							int creType = readNormalNr(bufor,i, 2); i+=2;
							int creNumb = readNormalNr(bufor,i, 2); i+=2;
							spec->m6cre.push_back(&(VLC->creh->creatures[creType]));
							spec->m6number.push_back(creNumb);
						}
						int limit = readNormalNr(bufor,i); i+=4;
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
						spec->m7wood = readNormalNr(bufor,i); i+=4;
						spec->m7mercury = readNormalNr(bufor,i); i+=4;
						spec->m7ore = readNormalNr(bufor,i); i+=4;
						spec->m7sulfur = readNormalNr(bufor,i); i+=4;
						spec->m7crystal = readNormalNr(bufor,i); i+=4;
						spec->m7gems = readNormalNr(bufor,i); i+=4;
						spec->m7gold = readNormalNr(bufor,i); i+=4;
						int limit = readNormalNr(bufor,i); i+=4;
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
						spec->m8hero = heroType;
						int limit = readNormalNr(bufor,i); i+=4;
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
						int limit = readNormalNr(bufor,i); i+=4;
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

				len1 = readNormalNr(bufor,i); i+=4;
				for(int ee=0; ee<len1; ++ee)
				{
					spec->firstVisitText += bufor[i]; ++i;
				}

				len2 = readNormalNr(bufor,i); i+=4;
				for(int ee=0; ee<len2; ++ee)
				{
					spec->nextVisitText += bufor[i]; ++i;
				}

				len3 = readNormalNr(bufor,i); i+=4;
				for(int ee=0; ee<len3; ++ee)
				{
					spec->completedText += bufor[i]; ++i;
				}
				nobj->info = spec;
borderguardend:
				break;
			}
		case HEROPLACEHOLDER_DEF:
			{
				i+=3; //TODO: handle it more properly
				break;
			}
		} //end of main switch
		objects.push_back(nobj);
	}
}

void Mapa::readEvents( unsigned char * bufor, int &i )
{
	int numberOfEvents = readNormalNr(bufor,i); i+=4;
	for(int yyoo=0; yyoo<numberOfEvents; ++yyoo)
	{
		CMapEvent ne;
		ne.name = std::string();
		ne.message = std::string();
		int nameLen = readNormalNr(bufor,i); i+=4;
		for(int qq=0; qq<nameLen; ++qq)
		{
			ne.name += bufor[i]; ++i;
		}
		int messLen = readNormalNr(bufor,i); i+=4;
		for(int qq=0; qq<messLen; ++qq)
		{
			ne.message +=bufor[i]; ++i;
		}
		ne.wood = readNormalNr(bufor,i); i+=4;
		ne.mercury = readNormalNr(bufor,i); i+=4;
		ne.ore = readNormalNr(bufor,i); i+=4;
		ne.sulfur = readNormalNr(bufor,i); i+=4;
		ne.crystal = readNormalNr(bufor,i); i+=4;
		ne.gems = readNormalNr(bufor,i); i+=4;
		ne.gold = readNormalNr(bufor,i); i+=4;
		ne.players = bufor[i]; ++i;
		if(version>AB)
		{
			ne.humanAffected = bufor[i]; ++i;
		}
		else
			ne.humanAffected = true;
		ne.computerAffected = bufor[i]; ++i;
		ne.firstOccurence = bufor[i]; ++i;
		ne.nextOccurence = bufor[i]; ++i;
		i+=18;
		events.push_back(ne);
	}
}

bool Mapa::isInTheMap( int3 pos )
{
	if(pos.x<0 || pos.y<0 || pos.z<0 || pos.x >= width || pos.y >= height || pos.z > twoLevel)
		return false;
	else return true;
}
