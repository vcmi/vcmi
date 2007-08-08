#include "stdafx.h"
#include "global.h"
#include "CPathfinder.h"

#define CGI (CGameInfo::mainObj)

CPath * CPathfinder::getPath(int3 &src, int3 &dest)
{
	if(src.z!=dest.z) //first check
		return NULL;

	//graph initialization
	graph.resize(CGI->ac->map.width);
	for(int i=0; i<graph.size(); ++i)
	{
		graph[i].resize(CGI->ac->map.height);
		for(int j=0; j<graph[i].size(); ++j)
		{
			graph[i][j].accesible = true;
			graph[i][j].dist = -1;
			graph[i][j].theNodeBefore = NULL;
			graph[i][j].x = i;
			graph[i][j].y = j;
		}
	}

	for(int h=0; h<CGI->objh->objInstances.size(); ++h)
	{
		if(CGI->objh->objInstances[h]->pos.z == src.z)
		{
			unsigned char blockMap[6];
			std::string ourName = CGI->ac->map.defy[CGI->objh->objInstances[h]->defNumber].name;
			std::transform(ourName.begin(), ourName.end(), ourName.begin(), (int(*)(int))toupper);
			for(int y=0; y<CGI->dobjinfo->objs.size(); ++y)
			{
				std::string cName = CGI->dobjinfo->objs[y].defName;
				std::transform(cName.begin(), cName.end(), cName.begin(), (int(*)(int))toupper);
				if(cName==ourName)
				{
					for(int u=0; u<6; ++u)
					{
						blockMap[u] = CGI->dobjinfo->objs[y].blockMap[u];
					}
					break;
				}
			}
			for(int i=0; i<6; ++i)
			{
				for(int j=0; j<8; ++j)
				{
					int cPosX = CGI->objh->objInstances[h]->pos.x - j;
					int cPosY = CGI->objh->objInstances[h]->pos.y - i;
					if(cPosX>0 && cPosY>0)
					{
						graph[cPosX][cPosY].accesible = blockMap[i] & (128 >> j);
					}
				}
			}
		}
	}
	//graph initialized


	return NULL;
}
