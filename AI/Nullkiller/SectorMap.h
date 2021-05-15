/*
* SectorMap.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/


#pragma once

#include "AIUtility.h"

enum
{
	NOT_VISIBLE = 0,
	NOT_CHECKED = 1,
	NOT_AVAILABLE
};

struct SectorMap
{
	//a sector is set of tiles that would be mutually reachable if all visitable objs would be passable (incl monsters)
	struct Sector
	{
		int id;
		std::vector<int3> tiles;
		std::vector<int3> embarkmentPoints; //tiles of other sectors onto which we can (dis)embark
		std::vector<const CGObjectInstance *> visitableObjs;
		bool water; //all tiles of sector are land or water
		Sector()
		{
			id = -1;
			water = false;
		}
	};

	typedef unsigned short TSectorID; //smaller than int to allow -1 value. Max number of sectors 65K should be enough for any proper map.
	typedef boost::multi_array<TSectorID, 3> TSectorArray;

	bool valid; //some kind of lazy eval
	std::map<int3, int3> parent;
	TSectorArray sector;
	//std::vector<std::vector<std::vector<unsigned char>>> pathfinderSector;

	std::map<int, Sector> infoOnSectors;
	std::shared_ptr<boost::multi_array<TerrainTile *, 3>> visibleTiles;

	SectorMap();
	SectorMap(HeroPtr h);
	void update();
	void clear();
	void exploreNewSector(crint3 pos, int num, CCallback * cbp);
	void write(crstring fname);

	bool markIfBlocked(TSectorID & sec, crint3 pos, const TerrainTile * t);
	bool markIfBlocked(TSectorID & sec, crint3 pos);
	TSectorID & retrieveTile(crint3 pos);
	TSectorID & retrieveTileN(TSectorArray & vectors, const int3 & pos);
	const TSectorID & retrieveTileN(const TSectorArray & vectors, const int3 & pos);
	TerrainTile * getTile(crint3 pos) const;
	std::vector<const CGObjectInstance *> getNearbyObjs(HeroPtr h, bool sectorsAround);

	void makeParentBFS(crint3 source);

	int3 firstTileToGet(HeroPtr h, crint3 dst); //if h wants to reach tile dst, which tile he should visit to clear the way?
	int3 findFirstVisitableTile(HeroPtr h, crint3 dst);
};
