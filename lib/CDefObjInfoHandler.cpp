#include "StdInc.h"
#include "CDefObjInfoHandler.h"

#include "filesystem/CResourceLoader.h"
#include "../client/CGameInfo.h"
#include "../lib/VCMI_Lib.h"
#include "GameConstants.h"

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
	visitDir = (8|16|32|64|128); //4,5,6,7,8 - any not-from-up direction

	width = height = -1;
}

void CGDefInfo::fetchInfoFromMSK()
{

	auto msk = CResourceHandler::get()->loadData(ResourceID(std::string("SPRITES/") + name, EResType::MASK));

	width = msk.first.get()[0];
	height = msk.first.get()[1];
	for(int i=0; i<6; ++i)
	{
		coverageMap[i] = msk.first.get()[i+2];
		shadowCoverage[i] = msk.first.get()[i+8];
	}
}

CDefObjInfoHandler::CDefObjInfoHandler()
{
	VLC->dobjinfo = this;

	auto textFile = CResourceHandler::get()->loadData(ResourceID("DATA/ZOBJCTS.TXT"));

	std::istringstream inp(std::string((char*)textFile.first.get(), textFile.second));
	int objNumber;
	inp>>objNumber;
	std::string mapStr;
	for(int hh=0; hh<objNumber; ++hh)
	{
		CGDefInfo* nobj = new CGDefInfo();
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
		si32 id; inp >> id; nobj->id = Obj(id);
		inp>>nobj->subid;
		inp>>nobj->type;

		nobj->visitDir = (8|16|32|64|128); //disabled visiting from the top

		if(nobj->type == 2 || nobj->type == 3 || nobj->type == 4 || nobj->type == 5) //creature, hero, artifact, resource
		{
			nobj->visitDir = 0xff;
		}
		else
		{
			static const Obj visitableFromTop[] =
				{Obj::FLOTSAM,
				Obj::SEA_CHEST,
				Obj::SHIPWRECK_SURVIVOR,
				Obj::BUOY,
				Obj::OCEAN_BOTTLE,
				Obj::BOAT,
				Obj::WHIRLPOOL,
				Obj::GARRISON,
				Obj::SCHOLAR,
				Obj::CAMPFIRE,
				Obj::BORDERGUARD,
				Obj::BORDER_GATE,
				Obj::QUEST_GUARD,
				Obj::CORPSE};

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

		auto dest = nobj->id.toDefObjInfo();
		if (dest.find(nobj->subid) != dest.end() && dest[nobj->subid] != nullptr)
		{
			// there is just too many of these. Note that this data is almost unused
			// exceptions are: town(village-capitol) and creation of new objects (holes, creatures, heroes, etc)
			//logGlobal->warnStream() << "Warning: overwriting def info for " << dest[nobj->subid]->name << " with " << nobj->name;
			dest[nobj->subid].dellNull(); // do not leak
		}

		nobj->id.toDefObjInfo()[nobj->subid] = nobj;

	}

	for (int i = 0; i < 8 ; i++)
	{

		static const char *holeDefs[] = {"AVLHOLD0.DEF", "AVLHLDS0.DEF", "AVLHOLG0.DEF", "AVLHLSN0.DEF",
			"AVLHOLS0.DEF", "AVLHOLR0.DEF", "AVLHOLX0.DEF", "AVLHOLL0.DEF"};

		if(i)
		{
			gobjs[Obj::HOLE][i] = new CGDefInfo(*gobjs[Obj::HOLE][0]);
		}
		gobjs[Obj::HOLE][i]->name = holeDefs[i];
	}
}

CDefObjInfoHandler::~CDefObjInfoHandler()
{
	for(bmap<int,bmap<int, ConstTransitivePtr<CGDefInfo> > >::iterator i=gobjs.begin(); i!=gobjs.end(); i++)
		for(bmap<int, ConstTransitivePtr<CGDefInfo> >::iterator j=i->second.begin(); j!=i->second.end(); j++)
			j->second.dellNull();
}
