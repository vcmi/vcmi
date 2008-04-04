#include "../stdafx.h"
#include "../CGameInfo.h"
#include "CDefHandler.h"
#include "CCreatureHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include <boost/assign/std/vector.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "../SDL_Extensions.h"

int CCreature::getQuantityID(int quantity)
{
	if (quantity<5)
		return 0;
	if (quantity<10)
		return 1;
	if (quantity<20)
		return 2;
	if (quantity<50)
		return 3;
	if (quantity<100)
		return 4;
	if (quantity<250)
		return 5;
	if (quantity<500)
		return 5;
	if (quantity<1000)
		return 6;
	if (quantity<4000)
		return 7;
	return 8;
}

void CCreatureHandler::loadCreatures()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("ZCRTRAIT.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;

	while(i<buf.size())
	{
		CCreature ncre;
		ncre.cost.resize(RESOURCE_QUANTITY);
		ncre.level=0;
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.nameSing = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.namePl = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[0] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[1] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[2] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[3] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[4] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[5] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.cost[6] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.fightValue = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.AIValue = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.growth = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.hordeGrowth = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.hitPoints = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.speed = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.attack = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.defence = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.low1 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.high1 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.shots = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.spells = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.low2 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.high2 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.abilityText = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		ncre.abilityRefs = buf.substr(befi, i-befi);
		i+=2;
		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber = creatures.size();
			ncre.isDefinite = true;
			creatures.push_back(ncre);
		}
	}
	for(int bb=1; bb<8; ++bb)
	{
		CCreature ncre;
		ncre.isDefinite = false;
		ncre.indefLevel = bb;
		ncre.indefUpgraded = false;
		creatures.push_back(ncre);
		ncre.indefUpgraded = true;
		creatures.push_back(ncre);
	}

	//loading reference names
	std::ifstream ifs("config/crerefnam.txt"); 
	int tempi;
	std::string temps;
	for (;;)
	{
		ifs >> tempi >> temps;
		if (tempi>=creatures.size())
			break;
		boost::assign::insert(nameToID)(temps,tempi);
		creatures[tempi].nameRef=temps;
	}
	ifs.close();
	ifs.clear();
	for(int i=1;i<=10;i++)
		levelCreatures.insert(std::pair<int,std::vector<CCreature*> >(i,std::vector<CCreature*>()));
	ifs.open("config/monsters.txt"); 
	{
		while(!ifs.eof())
		{
			int id, lvl;
			ifs >> id >> lvl;
			if(lvl>0)
			{
				creatures[id].level = lvl;
				levelCreatures[lvl].push_back(&(creatures[id]));
			}
		}
	}
	ifs.close();
	ifs.clear();


	ifs.open("config/cr_bgs.txt"); 
	while(!ifs.eof())
	{
		int id;
		std::string name;
		ifs >> id >> name;
		backgrounds[id]=CGI->bitmaph->loadBitmap(name);
	}
	ifs.close();
	ifs.clear();


	ifs.open("config/cr_factions.txt"); 
	while(!ifs.eof())
	{
		int id, fact;
		ifs >> id >> fact;
		creatures[id].faction = fact;
	}
	ifs.close();
	ifs.clear();

	//loading 32x32px imgs
	CDefHandler *smi = CGI->spriteh->giveDef("CPRSMALL.DEF");
	smi->notFreeImgs = true;
	for (int i=0; i<smi->ourImages.size(); i++)
	{
		boost::assign::insert(smallImgs)(i-2,smi->ourImages[i].bitmap);
	}
	delete smi;
	smi = CGI->spriteh->giveDef("TWCRPORT.DEF");
	smi->notFreeImgs = true;
	for (int i=0; i<smi->ourImages.size(); i++)
	{
		boost::assign::insert(bigImgs)(i-2,smi->ourImages[i].bitmap);
	}
	delete smi;
	//
	
	//loading unit animation def names
	std::ifstream inp("config/CREDEFS.TXT", std::ios::in | std::ios::binary); //this file is not in lod
	inp.seekg(0,std::ios::end); // na koniec
	int andame2 = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame2]; // allocate memory 
	inp.read((char*)bufor, andame2); // read map file to buffer
	inp.close();
	buf = std::string(bufor);
	delete [andame2] bufor;

	i = 0; //buf iterator
	hmcr = 0;
	for(i; i<andame2; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	for(int s=0; s<creatures.size()-16; ++s)
	{
		int befi=i;
		std::string rub;
		for(i; i<andame2; ++i)
		{
			if(buf[i]==' ')
				break;
		}
		rub = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame2; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		std::string defName = buf.substr(befi, i-befi);
		creatures[s].animDefName = defName;
	}
	loadAnimationInfo();
}

void CCreatureHandler::loadAnimationInfo()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("CRANIM.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	for(int dd=0; dd<creatures.size()-16; ++dd)
	{
		loadUnitAnimInfo(creatures[dd], buf, i);
	}
	return;
}

void CCreatureHandler::loadUnitAnimInfo(CCreature & unit, std::string & src, int & i)
{
	int befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.timeBetweenFidgets = atof(src.substr(befi, i-befi).c_str());
	++i;

	while(unit.timeBetweenFidgets == 0.0)
	{
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\r')
				break;
		}
		i+=2;
		befi=i;
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\t')
				break;
		}
		unit.timeBetweenFidgets = atof(src.substr(befi, i-befi).c_str());
		++i;
	}

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.walkAnimationTime = atof(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.attackAnimationTime = atof(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.flightAnimationDistance = atof(src.substr(befi, i-befi).c_str());
	++i;

	///////////////////////

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.upperRightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.upperRightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.rightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.rightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.lowerRightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.lowerRightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	///////////////////////

	for(int jjj=0; jjj<12; ++jjj)
	{
		befi=i;
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\t')
				break;
		}
		unit.missleFrameAngles[jjj] = atof(src.substr(befi, i-befi).c_str());
		++i;
	}

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.troopCountLocationOffset= atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.attackClimaxFrame = atoi(src.substr(befi, i-befi).c_str());
	++i;

	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\r')
			break;
	}
	i+=2;
}

void CCreatureHandler::loadUnitAnimations()
{
	std::ifstream inp("config/CREDEFS.TXT", std::ios::in | std::ios::binary); //this file is not in lod
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	inp.close();
	std::string buf = std::string(bufor);
	delete [andame] bufor;

	int i = 0; //buf iterator
	int hmcr = 0;
	for(i; i<andame; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	for(int s=0; s<creatures.size(); ++s)
	{
		int befi=i;
		std::string rub;
		for(i; i<andame; ++i)
		{
			if(buf[i]==' ')
				break;
		}
		rub = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		std::string defName = buf.substr(befi, i-befi);
		if(defName != std::string("x"))
			creatures[s].battleAnimation = CGameInfo::mainObj->spriteh->giveDef(defName);
		else
			creatures[s].battleAnimation = NULL;
		i+=2;
	}
}

int CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(int type)
{
	this->type = type;
	curFrame = 0;
	if(type!=-1)
	{
		if(SEntries[curFrame].group!=type) //rewind
		{
			int j=-1; //first frame in displayed group
			for(int g=0; g<SEntries.size(); ++g)
			{
				if(SEntries[g].group==type && j==-1)
				{
					j = g;
					break;
				}
			}
			if(curFrame!=-1)
				curFrame = j;
		}
	}
	else
	{
		if(curFrame>=frames)
			curFrame = 0;
	}
}

CCreatureAnimation::CCreatureAnimation(std::string name) : RLEntries(NULL), RWEntries(NULL)
{
	//load main file
	std::string data2 = CGI->spriteh->getTextFile(name);
	FDef = new unsigned char[data2.size()];
	for(int g=0; g<data2.size(); ++g)
	{
		FDef[g] = data2[g];
	}

	//init anim data
	int i,j, totalInBlock;
	char Buffer[13];
	defName=name;
	int andame = data2.size();
	i = 0;
	DEFType = readNormalNr(i,4); i+=4;
	fullWidth = readNormalNr(i,4); i+=4;
	fullHeight = readNormalNr(i,4); i+=4;
	i=0xc;
	totalBlocks = readNormalNr(i,4); i+=4;

	i=0x10;
	for (int it=0;it<256;it++)
	{
		palette[it].R = FDef[i++];
		palette[it].G = FDef[i++];
		palette[it].B = FDef[i++];
		palette[it].F = 0;
	}
	i=0x310;
	totalEntries=0;
	for (int z=0; z<totalBlocks; z++)
	{
		int unknown1 = readNormalNr(i,4); i+=4;
		totalInBlock = readNormalNr(i,4); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
			SEntries.push_back(SEntry());
		int unknown2 = readNormalNr(i,4); i+=4;
		int unknown3 = readNormalNr(i,4); i+=4;
		for (j=0; j<totalInBlock; j++)
		{
			for (int k=0;k<13;k++) Buffer[k]=FDef[i+k]; 
			i+=13;
			SEntries[totalEntries+j].name=Buffer;
		}
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = readNormalNr(i,4);
			int unknown4 = readNormalNr(i,4); i+=4;
		}
		//totalEntries+=totalInBlock;
		for(int hh=0; hh<totalInBlock; ++hh)
		{
			SEntries[totalEntries].group = z;
			++totalEntries;
		}
	}
	for(j=0; j<SEntries.size(); ++j)
	{
		SEntries[j].name = SEntries[j].name.substr(0, SEntries[j].name.find('.')+4);
	}
	//pictures don't have to be readed here
	//for(int i=0; i<SEntries.size(); ++i)
	//{
	//	Cimage nimg;
	//	nimg.bitmap = getSprite(i);
	//	nimg.imName = SEntries[i].name;
	//	nimg.groupNumber = SEntries[i].group;
	//	ourImages.push_back(nimg);
	//}
	//delete FDef;
	//FDef = NULL;

	//init vars
	curFrame = 0;
	type = -1;
	frames = totalEntries;
}

int CCreatureAnimation::readNormalNr (int pos, int bytCon, unsigned char * str, bool cyclic)
{
	int ret=0;
	int amp=1;
	if (str)
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp*=256;
		}
	}
	else 
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=FDef[pos+i]*amp;
			amp*=256;
		}
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}
int CCreatureAnimation::nextFrameMiddle(SDL_Surface *dest, int x, int y, bool attacker, bool incrementFrame, bool yellowBorder)
{
	return nextFrame(dest,x-fullWidth/2,y-fullHeight/2,attacker,incrementFrame,yellowBorder);
}
int CCreatureAnimation::nextFrame(SDL_Surface *dest, int x, int y, bool attacker, bool incrementFrame, bool yellowBorder)
{
	if(dest->format->BytesPerPixel<3)
		return -1; //not enough depth

	//increasing frame numer
	int SIndex = -1;
	if(incrementFrame)
	{
		SIndex = curFrame++;
		if(type!=-1)
		{
			if(SEntries[curFrame].group!=type) //rewind
			{
				int j=-1; //first frame in displayed group
				for(int g=0; g<SEntries.size(); ++g)
				{
					if(SEntries[g].group==type && j==-1)
					{
						j = g;
						break;
					}
				}
				if(curFrame!=-1)
					curFrame = j;
			}
		}
		else
		{
			if(curFrame>=frames)
				curFrame = 0;
		}
	}
	else
	{
		SIndex = curFrame;
	}
	//frame number increased

	long BaseOffset, 
		SpriteWidth, SpriteHeight, //sprite format
		LeftMargin, RightMargin, TopMargin,BottomMargin,
		i, add, FullHeight,FullWidth,
		TotalRowLength, // length of read segment
		NextSpriteOffset, RowAdd;
	unsigned char SegmentType, SegmentLength;
	
	i=BaseOffset=SEntries[SIndex].offset;
	int prSize=readNormalNr(i,4,FDef);i+=4;
	int defType2 = readNormalNr(i,4,FDef);i+=4;
	FullWidth = readNormalNr(i,4,FDef);i+=4;
	FullHeight = readNormalNr(i,4,FDef);i+=4;
	SpriteWidth = readNormalNr(i,4,FDef);i+=4;
	SpriteHeight = readNormalNr(i,4,FDef);i+=4;
	LeftMargin = readNormalNr(i,4,FDef);i+=4;
	TopMargin = readNormalNr(i,4,FDef);i+=4;
	RightMargin = FullWidth - SpriteWidth - LeftMargin;
	BottomMargin = FullHeight - SpriteHeight - TopMargin;
	
	add = 4 - FullWidth%4;

	int BaseOffsetor = BaseOffset = i;

	int ftcp = 0;

	if (defType2==1) //as it should be allways in creature animations
	{
		if (TopMargin>0)
		{
			for (int i=0;i<TopMargin;i++)
			{
				ftcp+=FullWidth+add;
			}
		}
		RLEntries = new int[SpriteHeight];
		for (int i=0;i<SpriteHeight;i++)
		{
			RLEntries[i]=readNormalNr(BaseOffset,4,FDef);BaseOffset+=4;
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor+RLEntries[i];
			if (LeftMargin>0)
			{
				ftcp+=LeftMargin;
			}
			TotalRowLength=0;
			do
			{
				SegmentType=FDef[BaseOffset++];
				SegmentLength=FDef[BaseOffset++];
				if (SegmentType==0xFF)
				{
					for (int k=0;k<=SegmentLength;k++)
					{
						int xB = (attacker ? ftcp%(FullWidth+add) : (FullWidth+add) - ftcp%(FullWidth+add) - 1) + x;
						int yB = ftcp/(FullWidth+add) + y;
						if(xB>=0 && yB>=0 && xB<dest->w && yB<dest->h)
						{
							putPixel(dest, xB + yB*dest->w, palette[FDef[BaseOffset+k]], FDef[BaseOffset+k], yellowBorder);
						}
						ftcp++; //increment pos
						if ((TotalRowLength+k+1)>=SpriteWidth)
							break;
					}
					BaseOffset+=SegmentLength+1;////
					TotalRowLength+=SegmentLength+1;
				}
				else
				{
					for (int k=0;k<SegmentLength+1;k++)
					{
						int xB = (attacker ? ftcp%(FullWidth+add) : (FullWidth+add) - ftcp%(FullWidth+add) - 1) + x;
						int yB = ftcp/(FullWidth+add) + y;
						if(xB>=0 && yB>=0 && xB<dest->w && yB<dest->h)
						{
							putPixel(dest, xB + yB*dest->w, palette[SegmentType], SegmentType, yellowBorder);
						}
						ftcp++; //increment pos
					}
					TotalRowLength+=SegmentLength+1;
				}
			}while(TotalRowLength<SpriteWidth);
			RowAdd=SpriteWidth-TotalRowLength;
			if (RightMargin>0)
			{
				ftcp+=RightMargin;
			}
			if (add>0)
			{
				ftcp+=add+RowAdd;
			}
		}
		delete [] RLEntries;
		RLEntries = NULL;
		if (BottomMargin>0)
		{
			ftcp += BottomMargin * (FullWidth+add);
		}
	}

	//for (int i=0; i<FullHeight; ++i)
	//{
	//	for (int j=0;j<FullWidth+add;j++)
	//	{
	//		if( i+y<dest->h && j+x<dest->w && i+y>=0 && j+x>=0)
	//		{
	//			unsigned char coln = FTemp[i*(FullWidth+add)+j]; //number of color from palette
	//			if(coln==0)
	//				continue;
	//			unsigned char* ptr = ((unsigned char*)dest->pixels + dest->format->BytesPerPixel * ((i + y)*dest->w + j + x));
	//			if(coln>7 || coln == 5) //normal or yellow border
	//			{
	//				*ptr = palette[coln].B;
	//				*(ptr+1) = palette[coln].G;
	//				*(ptr+2) = palette[coln].R;
	//			}
	//			else if(coln<5) //shadow
	//			{
	//				*ptr = ((*ptr) * (palette[coln].G + 50)) /200;
	//				*(ptr+1) = ((*(ptr+1)) * (palette[coln].G + 50)) /200 ;
	//				*(ptr+2) = ((*(ptr+2)) * (palette[coln].G + 50)) /200 ;
	//			}
	//			else if(coln == 6) //yellow border shadowed
	//			{
	//				*ptr = ((*ptr) + palette[coln-1].B) / 2;
	//				*(ptr+1) = ((*(ptr+1)) + palette[coln-1].G) / 2;
	//				*(ptr+2) = ((*(ptr+2)) + palette[coln-1].R) / 2;
	//			}

	//		}
	//	}
	//}

	//SDL_UpdateRect(dest, x, y, FullWidth+add, FullHeight);

	return 0;
}

CCreatureAnimation::~CCreatureAnimation()
{
	delete [] FDef;
	if (RWEntries)
		delete [] RWEntries;
	if (RLEntries)
		delete [] RLEntries;
}

void CCreatureAnimation::putPixel(SDL_Surface * dest, const int & ftcp, const BMPPalette & color, const unsigned char & palc, const bool & yellowBorder) const
{
	if(palc!=0)
	{
		Uint8 * p = (Uint8*)dest->pixels + ftcp*3;
		if(palc > 7) //normal color
		{
			p[0] = color.B;
			p[1] = color.G;
			p[2] = color.R;
		}
		else if(yellowBorder && (palc == 6 || palc == 7)) //dark yellow border
		{
			p[0] = 0;
			p[1] = 0xff;
			p[2] = 0xff;
		}
		else if(yellowBorder && (palc == 5)) //yellow border
		{
			p[0] = color.B;
			p[1] = color.G;
			p[2] = color.R;
		}
		else if(palc < 5) //shadow
		{
			Uint16 alpha;
			switch(color.G)
			{
			case 0:
				alpha = 128;
				break;
			case 50:
				alpha = 50+32;
				break;
			case 100:
				alpha = 100+64;
				break;
			case 125:
				alpha = 125+64;
				break;
			case 128:
				alpha = 128+64;
				break;
			case 150:
				alpha = 150+64;
				break;
			default:
				alpha = 255;
				break;
			}
			//alpha counted
			p[0] = (p[0] * alpha)>>8;
			p[1] = (p[1] * alpha)>>8;
			p[2] = (p[2] * alpha)>>8;
		}
	}
}
