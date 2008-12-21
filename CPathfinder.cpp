#include "stdafx.h"
#include "global.h"
#include "CPathfinder.h"
#include "CGameInfo.h"
#include "hch/CAmbarCendamo.h"
#include "mapHandler.h"
#include "CGameState.h"
#include "hch/CObjectHandler.h"
#include "hch/CDefObjInfoHandler.h"

using namespace boost::logic;

int3 CPath::startPos()
{
	return int3(nodes[nodes.size()-1].coord.x,nodes[nodes.size()-1].coord.y,nodes[nodes.size()-1].coord.z);
}
int3 CPath::endPos()
{
	return int3(nodes[0].coord.x,nodes[0].coord.y,nodes[0].coord.z);
}

CPath * CPathfinder::getPath(int3 src, int3 dest, const CGHeroInstance * hero, unsigned char type) //TODO: test it (seems to be finished, but relies on unwritten functions :()
{
	//check input
	if ((src.x < 0)||(src.y < 0)||(src.z < 0)||(dest.x < 0)||(dest.y < 0)||(dest.z < 0))
	{
		return NULL;
	}
	if ((src.x >= CGI->mh->sizes.x)||(src.y >= CGI->mh->sizes.y)||(src.z >= CGI->mh->sizes.z)
			||(dest.x >= CGI->mh->sizes.x)||(dest.y >= CGI->mh->sizes.y)||(dest.z >= CGI->mh->sizes.z))
	{
		return NULL;
	}

	int3 hpos = hero->getPosition(false);

	tribool blockLandSea; //true - blocks sea, false - blocks land, indeterminate - allows all
	if (!hero->canWalkOnSea())
	{
		if (CGI->mh->ttiles[hpos.x][hpos.y][hpos.z].tileInfo->tertype==water)
			blockLandSea=false;
		else
			blockLandSea=true;
	}
	else
	{
		blockLandSea = indeterminate;
	}

	//graph initialization
	graph.resize(CGI->mh->sizes.x);
	for(size_t i=0; i<graph.size(); ++i)
	{
		graph[i].resize(CGI->mh->sizes.y);
		for(size_t j=0; j<graph[i].size(); ++j)
		{
			graph[i][j].accesible = !CGI->mh->ttiles[i][j][src.z].tileInfo->blocked;
			if(i==dest.x && j==dest.y && CGI->mh->ttiles[i][j][src.z].tileInfo->visitable) {
				graph[i][j].accesible = true; //for allowing visiting objects
                        }
			graph[i][j].dist = -1;
			graph[i][j].theNodeBefore = NULL;
			graph[i][j].visited = false;
			graph[i][j].coord.x = i;
			graph[i][j].coord.y = j;
			graph[i][j].coord.z = dest.z;
			if (CGI->mh->ttiles[i][j][src.z].tileInfo->tertype==rock) {
				graph[i][j].accesible = false;
                        }
			if ((blockLandSea) && (CGI->mh->ttiles[i][j][src.z].tileInfo->tertype==water)) {
				graph[i][j].accesible = false;
                        }
			else if ((!blockLandSea) && (CGI->mh->ttiles[i][j][src.z].tileInfo->tertype!=water)) {
				graph[i][j].accesible = false;
                        }
			if(graph[i][j].accesible) {
				graph[i][j].accesible = CGI->state->players[hero->tempOwner].fogOfWarMap[i][j][src.z];
                        }
		}
	}

	//graph initialized

	graph[src.x][src.y].dist = 0;

	std::queue<CPathNode> mq;
	mq.push(graph[src.x][src.y]);

	unsigned int curDist = 4000000000; //XXX 2 147 483 648 // only in C90 //but numeric limit shows 0-4294967295...confused

	while(!mq.empty())
	{
		CPathNode cp = mq.front();
		mq.pop();
		if ((cp.coord.x == dest.x) && (cp.coord.y==dest.y))
		{
			if (cp.dist < curDist)
				curDist=cp.dist;
		}
		else
		{
			if (cp.dist > curDist)
				continue;
		}
		if(cp.coord.x>0)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y];
			processNode(dp, hero, mq, cp, src, false);
		}
		if(cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x][cp.coord.y-1];
			processNode(dp, hero, mq, cp, src, false);
		}
		if(cp.coord.x>0 && cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y-1];
			processNode(dp, hero, mq, cp, src, true);
		}
		if(cp.coord.x<graph.size()-1)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y];
			processNode(dp, hero, mq, cp, src, false);
		}
		if(cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x][cp.coord.y+1];
			processNode(dp, hero, mq, cp, src, false);
		}
		if(cp.coord.x<graph.size()-1 && cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y+1];
			processNode(dp, hero, mq, cp, src, true);
		}
		if(cp.coord.x>0 && cp.coord.y<graph[0].size()-1)
		{
			CPathNode & dp = graph[cp.coord.x-1][cp.coord.y+1];
			processNode(dp, hero, mq, cp, src, true);
		}
		if(cp.coord.x<graph.size()-1 && cp.coord.y>0)
		{
			CPathNode & dp = graph[cp.coord.x+1][cp.coord.y-1];
			processNode(dp, hero, mq, cp, src, true);
		}
	}

	CPathNode curNode = graph[dest.x][dest.y];
	if(!curNode.theNodeBefore)
		return NULL;

	CPath * ret = new CPath;

	while(curNode.coord!=graph[src.x][src.y].coord)
	{
		ret->nodes.push_back(curNode);
		curNode = *(curNode.theNodeBefore);
	}

	ret->nodes.push_back(graph[src.x][src.y]);

	return ret;
}

void CPathfinder::convertPath(CPath * path, unsigned int mode) //mode=0 -> from 'manifest' to 'object'
{
	if (mode==0)
	{
		for (int i=0;i<path->nodes.size();i++)
		{
			path->nodes[i].coord = CGHeroInstance::convertPosition(path->nodes[i].coord,true);
		}
	}
}

void CPathfinder::processNode(CPathNode & dp, const CGHeroInstance * hero, std::queue<CPathNode> & mq, const CPathNode & cp, const int3 & src, bool diagonal)
{
	const TerrainTile * tinfo = CGI->mh->ttiles[dp.coord.x][dp.coord.y][src.z].tileInfo;
	int cost = hero->getTileCost(tinfo->tertype, tinfo->malle, tinfo->nuine);
	if(diagonal)
		cost *= std::sqrt(2.0);
	if((dp.dist==-1 || (dp.dist > cp.dist + cost)) && dp.accesible && checkForVisitableDir(cp.coord, dp.coord) && checkForVisitableDir(dp.coord, cp.coord))
	{
		dp.dist = cp.dist + cost;
		dp.theNodeBefore = &graph[cp.coord.x][cp.coord.y];
		mq.push(dp);
	}
}

bool CPathfinder::checkForVisitableDir(const int3 & src, const int3 & dst) const
{
	for(int b=0; b<CGI->mh->ttiles[dst.x][dst.y][dst.z].tileInfo->visitableObjects.size(); ++b) //checking destination tile
	{
		const TerrainTile * pom = CGI->mh->ttiles[dst.x][dst.y][dst.z].tileInfo;
		if(!vstd::contains(pom->blockingObjects, pom->visitableObjects[b]))
			continue;
		CGDefInfo * di = pom->visitableObjects[b]->defInfo;
		if( (dst.x == src.x-1 && dst.y == src.y-1) && !(di->visitDir & (1<<4)) )
		{
			return false;
		}
		if( (dst.x == src.x && dst.y == src.y-1) && !(di->visitDir & (1<<5)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y-1) && !(di->visitDir & (1<<6)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y) && !(di->visitDir & (1<<7)) )
		{
			return false;
		}
		if( (dst.x == src.x+1 && dst.y == src.y+1) && !(di->visitDir & (1<<0)) )
		{
			return false;
		}
		if( (dst.x == src.x && dst.y == src.y+1) && !(di->visitDir & (1<<1)) )
		{
			return false;
		}
		if( (dst.x == src.x-1 && dst.y == src.y+1) && !(di->visitDir & (1<<2)) )
		{
			return false;
		}
		if( (dst.x == src.x-1 && dst.y == src.y) && !(di->visitDir & (1<<3)) )
		{
			return false;
		}
	}
	return true;
}
