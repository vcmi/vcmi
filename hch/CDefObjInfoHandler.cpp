#define VCMI_DLL
#include "../stdafx.h"
#include "CDefObjInfoHandler.h"
#include "../client/CGameInfo.h"
#include "CLodHandler.h"
#include <sstream>
#include "../lib/VCMI_Lib.h"
#include <set>

extern CLodHandler * bitmaph;

/*
 * CDefObjInfoHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

bool CGDefInfo::isVisitable() const
{
	for (int i=0; i<6; i++)
	{
		if (visitMap[i])
			return true;
	}
	return false;
}
CGDefInfo::CGDefInfo()
{
	handler = NULL;
	visitDir = (8|16|32|64|128); //4,5,6,7,8 - any not-from-up direction

	width = height = -1;
}

void CGDefInfo::fetchInfoFromMSK()
{
	std::string nameCopy = name;
	std::string msk = spriteh->getTextFile(nameCopy.replace( nameCopy.size()-4, 4, "#MSK" ));

	width = msk[0];
	height = msk[1];
	for(int i=0; i<6; ++i)
	{
		coverageMap[i] = msk[i+2];
		shadowCoverage[i] = msk[i+8];
	}
}

void CDefObjInfoHandler::load()
{
	VLC->dobjinfo = this;
	std::istringstream inp(bitmaph->getTextFile("ZOBJCTS.TXT"));
	int objNumber;
	inp>>objNumber;
	std::string mapStr;
	for(int hh=0; hh<objNumber; ++hh)
	{
		CGDefInfo* nobj = new CGDefInfo();
		nobj->handler = NULL;
		std::string dump;
		inp>>nobj->name;
		
		std::transform(nobj->name.begin(), nobj->name.end(), nobj->name.begin(), (int(*)(int))toupper);

		for(int o=0; o<6; ++o)
		{
			nobj->blockMap[o] = 0xff;
			nobj->visitMap[o] = 0x00;
			nobj->coverageMap[o] = 0x00;
			nobj->shadowCoverage[o] = 0x00;
		}
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='0')
			{
				nobj->blockMap[v/8] &= 255 - (128 >> (v%8));
			}
		}
		inp>>mapStr;
		std::reverse(mapStr.begin(), mapStr.end());
		for(int v=0; v<mapStr.size(); ++v)
		{
			if(mapStr[v]=='1')
			{
				nobj->visitMap[v/8] |= (128 >> (v%8));
			}
		}

		for(int yy=0; yy<2; ++yy) //first - on which types of terrain object can be placed; 
			inp>>dump; //second -in which terrains' menus object in the editor will be available (?)
		inp>>nobj->id;
		inp>>nobj->subid;
		inp>>nobj->type;

		nobj->visitDir = (8|16|32|64|128); //disabled visiting from the top

		if(nobj->type == 2 || nobj->type == 3 || nobj->type == 4 || nobj->type == 5) //creature, hero, artifact, resource
		{
			nobj->visitDir = 0xff;
		}
		else 
		{
			static int visitableFromTop[] = {29, 82, 86, 11, 59, 8, 111,33,81,12,9,212,215,22}; //sea chest, flotsam, shipwreck survivor, buoy, ocean bottle, boat, whirlpool, garrison, scholar, campfire, borderguard, bordergate, questguard, corpse
			for(int i=0; i < ARRAY_COUNT(visitableFromTop); i++)
			{
				if(visitableFromTop[i] == nobj->id)
				{
					nobj->visitDir = 0xff;
					break;
				}
			}
		}
		inp >> nobj->printPriority;

		//coverageMap calculating
		nobj->fetchInfoFromMSK();


		gobjs[nobj->id][nobj->subid] = nobj;
		if(nobj->id==TOWNI_TYPE)
			castles[nobj->subid]=nobj;
	}

	for (int i = 0; i < 8 ; i++)
	{

		static const char *holeDefs[] = {"AVLHOLD0.DEF", "AVLHLDS0.DEF", "AVLHOLG0.DEF", "AVLHLSN0.DEF",
			"AVLHOLS0.DEF", "AVLHOLR0.DEF", "AVLHOLX0.DEF", "AVLHOLL0.DEF"};

		CGDefInfo *& tmp = gobjs[124][i];
		if(i)
		{
			tmp = new CGDefInfo;
			*tmp = *gobjs[124][0];
		}
		tmp->name = holeDefs[i];
	}
}
 
CDefObjInfoHandler::~CDefObjInfoHandler()
{
	for(std::map<int,std::map<int,CGDefInfo*> >::iterator i=gobjs.begin(); i!=gobjs.end(); i++)
		for(std::map<int,CGDefInfo*>::iterator j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
}
