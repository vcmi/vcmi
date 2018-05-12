
#include "../../CCallback.h"
#include "SectorMap.h"

extern boost::thread_specific_ptr<CCallback> cb;

enum
{
	NOT_VISIBLE = 0,
	NOT_CHECKED = 1,
	NOT_AVAILABLE
};

SectorMap::SectorMap()
{
	update();
}

SectorMap::SectorMap(HeroPtr h)
{
	update();
	makeParentBFS(h->visitablePos());
}

bool SectorMap::markIfBlocked(TSectorID & sec, crint3 pos, const TerrainTile * t)
{
	if (t->blocked && !t->visitable)
	{
		sec = NOT_AVAILABLE;
		return true;
	}

	return false;
}

bool SectorMap::markIfBlocked(TSectorID & sec, crint3 pos)
{
	return markIfBlocked(sec, pos, getTile(pos));
}

void SectorMap::update()
{
	visibleTiles = cb->getAllVisibleTiles();
	auto shape = visibleTiles->shape();
	sector.resize(boost::extents[shape[0]][shape[1]][shape[2]]);

	clear();
	int curSector = 3; //0 is invisible, 1 is not explored

	CCallback * cbp = cb.get(); //optimization
	foreach_tile_pos([&](crint3 pos)
	{
		if (retrieveTile(pos) == NOT_CHECKED)
		{
			if (!markIfBlocked(retrieveTile(pos), pos))
				exploreNewSector(pos, curSector++, cbp);
		}
	});
	valid = true;
}

SectorMap::TSectorID & SectorMap::retrieveTileN(SectorMap::TSectorArray & a, const int3 & pos)
{
	return a[pos.x][pos.y][pos.z];
}

const SectorMap::TSectorID & SectorMap::retrieveTileN(const SectorMap::TSectorArray & a, const int3 & pos)
{
	return a[pos.x][pos.y][pos.z];
}

void SectorMap::clear()
{
	//TODO: rotate to [z][x][y]
	auto fow = cb->getVisibilityMap();
	//TODO: any magic to automate this? will need array->array conversion
	//std::transform(fow.begin(), fow.end(), sector.begin(), [](const ui8 &f) -> unsigned short
	//{
	//	return f; //type conversion
	//});
	auto width = fow.size();
	auto height = fow.front().size();
	auto depth = fow.front().front().size();
	for (size_t x = 0; x < width; x++)
	{
		for (size_t y = 0; y < height; y++)
		{
			for (size_t z = 0; z < depth; z++)
				sector[x][y][z] = fow[x][y][z];
		}
	}
	valid = false;
}

void SectorMap::exploreNewSector(crint3 pos, int num, CCallback * cbp)
{
	Sector & s = infoOnSectors[num];
	s.id = num;
	s.water = getTile(pos)->isWater();

	std::queue<int3> toVisit;
	toVisit.push(pos);
	while (!toVisit.empty())
	{
		int3 curPos = toVisit.front();
		toVisit.pop();
		TSectorID & sec = retrieveTile(curPos);
		if (sec == NOT_CHECKED)
		{
			const TerrainTile * t = getTile(curPos);
			if (!markIfBlocked(sec, curPos, t))
			{
				if (t->isWater() == s.water) //sector is only-water or only-land
				{
					sec = num;
					s.tiles.push_back(curPos);
					foreach_neighbour(cbp, curPos, [&](CCallback * cbp, crint3 neighPos)
					{
						if (retrieveTile(neighPos) == NOT_CHECKED)
						{
							toVisit.push(neighPos);
							//parent[neighPos] = curPos;
						}
						const TerrainTile * nt = getTile(neighPos);
						if (nt && nt->isWater() != s.water && canBeEmbarkmentPoint(nt, s.water))
						{
							s.embarkmentPoints.push_back(neighPos);
						}
					});

					if (t->visitable)
					{
						auto obj = t->visitableObjects.front();
						if (cb->getObj(obj->id, false)) // FIXME: we have to filter invisible objcts like events, but probably TerrainTile shouldn't be used in SectorMap at all
							s.visitableObjs.push_back(obj);
					}
				}
			}
		}
	}

	vstd::removeDuplicates(s.embarkmentPoints);
}

void SectorMap::write(crstring fname)
{
	std::ofstream out(fname);
	for (int k = 0; k < cb->getMapSize().z; k++)
	{
		for (int j = 0; j < cb->getMapSize().y; j++)
		{
			for (int i = 0; i < cb->getMapSize().x; i++)
			{
				out << (int)sector[i][j][k] << '\t';
			}
			out << std::endl;
		}
		out << std::endl;
	}
}