#define VCMI_DLL
#include "IGameCallback.h"
#include "../CGameState.h"
#include "../map.h"
#include "../hch/CObjectHandler.h"
#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../lib/VCMI_Lib.h"

/*
 * IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const CGObjectInstance* IGameCallback::getObj(int objid)
{
	if(objid < 0  ||  objid >= gs->map->objects.size())
	{
		tlog1 << "Cannot get object with id " << objid << std::endl;
		return NULL;
	}
	else if (!gs->map->objects[objid])
	{
		tlog1 << "Cannot get object with id " << objid << ". Object was removed.\n";
		return NULL;
	}
	return gs->map->objects[objid];
}
const CGHeroInstance* IGameCallback::getHero(int objid)
{
	const CGObjectInstance *obj = getObj(objid);
	if(obj)
		return dynamic_cast<const CGHeroInstance*>(obj);
	else
		return NULL;
}
const CGTownInstance* IGameCallback::getTown(int objid)
{
	const CGObjectInstance *obj = getObj(objid);
	if(obj)
		return dynamic_cast<const CGTownInstance*>(gs->map->objects[objid]);
	else
		return NULL;
}

int IGameCallback::getOwner(int heroID)
{
	return gs->map->objects[heroID]->tempOwner;
}
int IGameCallback::getResource(int player, int which)
{
	return gs->players.find(player)->second.resources[which];
}
int IGameCallback::getDate(int mode)
{
	return gs->getDate(mode);
}

const CGHeroInstance* IGameCallback::getSelectedHero( int player )
{
	return getHero(gs->players.find(player)->second.currentSelection);
}

const PlayerSettings * IGameCallback::getPlayerSettings( int color )
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}

int IGameCallback::getHeroCount( int player, bool includeGarrisoned )
{
	int ret = 0;
	if(includeGarrisoned)
		return gs->getPlayer(player)->heroes.size();
	else
		for(int i=0; i < gs->getPlayer(player)->heroes.size(); i++)
			if(!gs->getPlayer(player)->heroes[i]->inTownGarrison)
				ret++;
	return ret;
}

void IGameCallback::getTilesInRange( std::set<int3> &tiles, int3 pos, int radious, int player/*=-1*/, int mode/*=0*/ )
{
	if(player >= PLAYER_LIMIT)
	{
		tlog1 << "Illegal call to getTilesInRange!\n";
		return;
	}

	for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gs->map->width - 1); xd++)
	{
		for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gs->map->height - 1); yd++)
		{
			double distance = pos.dist2d(int3(xd,yd,pos.z)) - 0.5;
			if(distance <= radious)
			{
				if(player < 0 
					|| (mode == 1  && gs->players.find(player)->second.fogOfWarMap[xd][yd][pos.z]==0)
					|| (mode == -1 && gs->players.find(player)->second.fogOfWarMap[xd][yd][pos.z]==1)
				)
					tiles.insert(int3(xd,yd,pos.z));
			}
		}
	}
}

bool IGameCallback::isAllowed( int type, int id )
{
	switch(type)
	{
	case 0:
		return gs->map->allowedSpell[id];
	case 1:
		return gs->map->allowedArtifact[id];
	default:
		tlog1 << "Wrong call to IGameCallback::isAllowed!\n";
		return false;
	}
}

void IGameCallback::getAllowedArts(std::vector<CArtifact*> &out, std::vector<CArtifact*> CArtHandler::*arts)
{
	for(int i = 0; i < (VLC->arth->*arts).size(); i++)
	{
		CArtifact *art = (VLC->arth->*arts)[i];
		if(isAllowed(1,art->id))
		{
			out.push_back(art);
		}
	}
}

void IGameCallback::getAllowed(std::vector<CArtifact*> &out, int flags)
{
	if(flags & ART_TREASURE)
		getAllowedArts(out,&CArtHandler::treasures);
	if(flags & ART_MINOR)
		getAllowedArts(out,&CArtHandler::minors);
	if(flags & ART_MAJOR)
		getAllowedArts(out,&CArtHandler::majors);
	if(flags & ART_RELIC)
		getAllowedArts(out,&CArtHandler::relics);
}