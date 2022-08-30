#include "StdInc.h"
#include "maphandler.h"
#include "../lib/mapping/CMap.h"

const int tileSize = 32;

MapHandler::MapHandler(const CMap * Map):
	map(Map), surface(Map->width * tileSize, Map->height * tileSize), painter(&surface)
{
	init();
}

void MapHandler::init()
{
	//sizes of terrain
	sizes.x = map->width;
	sizes.y = map->height;
	sizes.z = map->twoLevel ? 2 : 1;
	
	initTerrainGraphics();
	logGlobal->info("\tPreparing terrain, roads, rivers, borders");
	initObjectRects();
	logGlobal->info("\tMaking object rects");
}

void MapHandler::initTerrainGraphics()
{
	static const std::map<std::string, std::string> ROAD_FILES =
	{
		{ROAD_NAMES[1], "dirtrd"},
		{ROAD_NAMES[2], "gravrd"},
		{ROAD_NAMES[3], "cobbrd"}
	};
	
	static const std::map<std::string, std::string> RIVER_FILES =
	{
		{RIVER_NAMES[1], "clrrvr"},
		{RIVER_NAMES[2], "icyrvr"},
		{RIVER_NAMES[3], "mudrvr"},
		{RIVER_NAMES[4], "lavrvr"}
	};
	
	
	auto loadFlipped = [](TFlippedAnimations & animation, TFlippedCache & cache, const std::map<std::string, std::string> & files)
	{
		for(auto & type : files)
		{
			animation[type.first] = make_unique<Animation>(type.second);
			animation[type.first]->preload();
			const size_t views = animation[type.first]->size(0);
			cache[type.first].resize(views);
			
			for(int j = 0; j < views; j++)
				cache[type.first][j] = animation[type.first]->getImage(j);
		}
	};
	
	std::map<std::string, std::string> terrainFiles;
	for(auto & terrain : Terrain::Manager::terrains())
	{
		terrainFiles[terrain] = Terrain::Manager::getInfo(terrain).tilesFilename;
	}
	
	loadFlipped(terrainAnimations, terrainImages, terrainFiles);
	loadFlipped(roadAnimations, roadImages, ROAD_FILES);
	loadFlipped(riverAnimations, riverImages, RIVER_FILES);
}

void MapHandler::drawTerrainTile(int x, int y, const TerrainTile & tinfo)
{
	//Rect destRect(realTileRect);
	
	ui8 rotation = tinfo.extTileFlags % 4;
	
	if(terrainImages.at(tinfo.terType).size() <= tinfo.terView)
		return;
	
	bool hflip = (rotation == 1 || rotation == 3), vflip = (rotation == 2 || rotation == 3);
	
	painter.drawImage(x * tileSize, y * tileSize, terrainImages.at(tinfo.terType)[tinfo.terView]->mirrored(hflip, vflip));
}

void MapHandler::initObjectRects()
{
	//initializing objects / rects
	/*for(auto & elem : map->objects)
	{
		const CGObjectInstance *obj = elem;
		if(	!obj
		   || (obj->ID==Obj::HERO && static_cast<const CGHeroInstance*>(obj)->inTownGarrison) //garrisoned hero
		   || (obj->ID==Obj::BOAT && static_cast<const CGBoat*>(obj)->hero)) //boat with hero (hero graphics is used)
		{
			continue;
		}
		
		std::shared_ptr<Animation> animation = graphics->getAnimation(obj);
		
		//no animation at all
		if(!animation)
			continue;
		
		//empty animation
		if(animation->size(0) == 0)
			continue;
		
		auto image = animation->getImage(0,0);
		
		for(int fx=0; fx < obj->getWidth(); ++fx)
		{
			for(int fy=0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);
				SDL_Rect cr;
				cr.w = 32;
				cr.h = 32;
				cr.x = image->width() - fx * 32 - 32;
				cr.y = image->height() - fy * 32 - 32;
				TerrainTileObject toAdd(obj, cr, obj->visitableAt(currTile.x, currTile.y));
				
				
				if( map->isInTheMap(currTile) && // within map
				   cr.x + cr.w > 0 &&           // image has data on this tile
				   cr.y + cr.h > 0 &&
				   obj->coveringAt(currTile.x, currTile.y) // object is visible here
				   )
				{
					ttiles[currTile.x][currTile.y][currTile.z].objects.push_back(toAdd);
				}
			}
		}
	}
	
	for(int ix=0; ix<ttiles.size()-frameW; ++ix)
	{
		for(int iy=0; iy<ttiles[0].size()-frameH; ++iy)
		{
			for(int iz=0; iz<ttiles[0][0].size(); ++iz)
			{
				stable_sort(ttiles[ix][iy][iz].objects.begin(), ttiles[ix][iy][iz].objects.end(), objectBlitOrderSorter);
			}
		}
	}*/
}
