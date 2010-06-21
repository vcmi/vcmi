#define VCMI_DLL
#include "IGameCallback.h"
#include "../lib/CGameState.h"
#include "../lib/map.h"
#include "../hch/CObjectHandler.h"
#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CSpellHandler.h"
#include "../lib/VCMI_Lib.h"
#include <boost/random/linear_congruential.hpp>

/*
 * IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern boost::rand48 ran;

CGameState *const IGameCallback::gameState ()
{ 
	return gs;
}
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
	if(gs->players.find(player)->second.currentSelection==-1)
		return NULL;
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
	if (radious == -1) //reveal entire map
		getAllTiles (tiles, player, -1, 0);
	else
	{
		PlayerState * plr = &gs->players.find(player)->second;
		for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gs->map->width - 1); xd++)
		{
			for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gs->map->height - 1); yd++)
			{
				double distance = pos.dist2d(int3(xd,yd,pos.z)) - 0.5;
				if(distance <= radious)
				{
					if(player < 0 
						|| (mode == 1  && plr->fogOfWarMap[xd][yd][pos.z]==0)
						|| (mode == -1 && plr->fogOfWarMap[xd][yd][pos.z]==1)
					)
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void IGameCallback::getAllTiles (std::set<int3> &tiles, int player/*=-1*/, int level, int surface )
{
	if(player >= PLAYER_LIMIT)
	{
		tlog1 << "Illegal call to getAllTiles !\n";
		return;
	}
	bool water = surface == 0 || surface == 2,
		land = surface == 0 || surface == 1;

	std::vector<int> floors;
	if(level == -1)
	{
		
		for (int xd = 0; xd <= gs->map->width - 1; xd++)
		for(int b=0; b<gs->map->twoLevel + 1; ++b) //if gs->map->twoLevel is false then false (0) + 1 is 1, if it's true (1) then we have 2
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for (std::vector<int>::const_iterator i = floors.begin(); i!= floors.end(); i++)
	{
		register int zd = *i;
		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				if ((getTile (int3 (xd,yd,zd))->tertype == 8 && water)
					|| (getTile (int3 (xd,yd,zd))->tertype != 8 && land))
					tiles.insert(int3(xd,yd,zd));
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

void IGameCallback::getAllowedArts(std::vector<CArtifact*> &out, std::vector<CArtifact*> CArtHandler::*arts, int flag)
{
	if (!(VLC->arth->*arts).size()) //restock avaliable arts
	{
		for (int i = 0; i < VLC->arth->artifacts.size(); i++)
		{
			if (VLC->arth->artifacts[i].aClass == flag)
				(VLC->arth->*arts).push_back(&(VLC->arth->artifacts[i]));
		}
	}

	for (int i = 0; i < (VLC->arth->*arts).size(); i++)
	{
		CArtifact *art = (VLC->arth->*arts)[i];
		if(isAllowed(1, art->id))
		{
			out.push_back(art);
		}
	}
}

void IGameCallback::getAllowed(std::vector<CArtifact*> &out, int flags)
{
	if(flags & CArtifact::ART_TREASURE)
		getAllowedArts(out,&CArtHandler::treasures, CArtifact::ART_TREASURE);
	if(flags & CArtifact::ART_MINOR)
		getAllowedArts(out,&CArtHandler::minors, CArtifact::ART_MINOR);
	if(flags & CArtifact::ART_MAJOR)
		getAllowedArts(out,&CArtHandler::majors, CArtifact::ART_MAJOR);
	if(flags & CArtifact::ART_RELIC)
		getAllowedArts(out,&CArtHandler::relics, CArtifact::ART_RELIC);
	if (!out.size()) //no arts are avaliable
	{
		out.resize(64);
		std::fill_n(out.begin(), 64, &(VLC->arth->artifacts[2])); //magic
	}
}

ui16 IGameCallback::getRandomArt (int flags)
{
	std::vector<CArtifact*> out;
	getAllowed(out, flags);
	CArtifact *art = out[ran() % out.size()];
	std::vector<CArtifact*>* ptr;
	switch (art->aClass)
	{
		case CArtifact::ART_TREASURE:
			ptr = &VLC->arth->treasures;
			break;
		case CArtifact::ART_MINOR:
			ptr = &VLC->arth->minors;
			break;
		case CArtifact::ART_MAJOR:
			ptr = &VLC->arth->majors;
			break;
		case CArtifact::ART_RELIC:
			ptr = &VLC->arth->relics;
			break;
		default: //special artifacts should not be erased
			return art->id;
	}
	ptr->erase (std::find(ptr->begin(), ptr->end(), art)); //remove the artifact from avaliable list
	return art->id;
}

void IGameCallback::getAllowedSpells(std::vector<ui16> &out, ui16 level)
{

	CSpell *spell;
	for (int i = 0; i < gs->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
	{
		spell = &(VLC->spellh->spells[i]);
		if (isAllowed (0, spell->id) && spell->level == level)
		{
			out.push_back(spell->id);
		}
	}
}

int3 IGameCallback::getMapSize()
{
	return int3(gs->map->width, gs->map->height, gs->map->twoLevel + 1);
}

inline TerrainTile * IGameCallback::getTile( int3 pos )
{
	if(!gs->map->isInTheMap(pos))
		return NULL;
	return &gs->map->getTile(pos);
}

const PlayerState * IGameCallback::getPlayerState( int color )
{
	return gs->getPlayer(color, false);
}