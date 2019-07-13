/*
 * ObjectTemplate.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CObjectClassesHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CBinaryReader.h"
#include "../VCMI_Lib.h"
#include "../GameConstants.h"
#include "../StringConstants.h"
#include "../CGeneralTextHandler.h"
#include "CObjectHandler.h"
#include "../CModHandler.h"
#include "../JsonNode.h"

#include "CRewardableConstructor.h"

static bool isOnVisitableFromTopList(int identifier, int type)
{
	if(type == 2 || type == 3 || type == 4 || type == 5) //creature, hero, artifact, resource
		return true;

	//TODO: this list needs to be extendable by mods
	static const Obj visitableFromTop[] =
		{Obj::FLOTSAM,
		Obj::SEA_CHEST,
		Obj::SHIPWRECK_SURVIVOR,
		Obj::BUOY,
		Obj::OCEAN_BOTTLE,
		Obj::BOAT,
		Obj::WHIRLPOOL,
		Obj::GARRISON,
		Obj::GARRISON2,
		Obj::SCHOLAR,
		Obj::CAMPFIRE,
		Obj::BORDERGUARD,
		Obj::BORDER_GATE,
		Obj::QUEST_GUARD,
		Obj::CORPSE
	};
	return vstd::find_pos(visitableFromTop, identifier) != -1;
}

ObjectTemplate::ObjectTemplate():
	visitDir(8|16|32|64|128), // all but top
	id(Obj::NO_OBJ),
	subid(0),
	printPriority(0),
	stringID("")
{
	setSize(TILES_WIDTH, TILES_HEIGHT);
}

void ObjectTemplate::afterLoadFixup()
{
	if(id == Obj::EVENT)
	{
		setSize(1,1);
		usedTiles[0][0] = EBlockMapBits::VISITABLE;
		visitDir = 0xFF;
	}
	boost::algorithm::replace_all(animationFile, "\\", "/");
	boost::algorithm::replace_all(editorAnimationFile, "\\", "/");
}

void ObjectTemplate::readTxt(CLegacyConfigParser & parser)
{
	std::string data = parser.readString();
	std::vector<std::string> strings;
	boost::split(strings, data, boost::is_any_of(" "));
	assert(strings.size() == 9);

	animationFile = strings[0];
	stringID = strings[0];

	std::string & blockStr = strings[1]; //block map, 0 = blocked, 1 = unblocked
	std::string & visitStr = strings[2]; //visit map, 1 = visitable, 0 = not visitable

	assert(blockStr.size() == TILES_HEIGHT * TILES_WIDTH);
	assert(visitStr.size() == TILES_HEIGHT * TILES_WIDTH);

	setSize(TILES_WIDTH, TILES_HEIGHT);
	for (size_t i = 0; i < TILES_HEIGHT; i++) // 6 rows
	{
		for (size_t j = 0; j < TILES_WIDTH; j++) // 8 columns
		{
			auto & tile = usedTiles[i][j];
			tile |= EBlockMapBits::VISIBLE; // assume that all tiles are visible
			if (blockStr[i * TILES_WIDTH + j] == '0')
				tile |= EBlockMapBits::BLOCKED;

			if (visitStr[i * TILES_WIDTH + j] == '1')
				tile |= EBlockMapBits::VISITABLE;
		}
	}

	// strings[3] most likely - terrains on which this object can be placed in editor.
	// e.g. Whirpool can be placed manually only on water while mines can be placed everywhere despite terrain-specific gfx
	// so these two fields can be interpreted as "strong affinity" and "weak affinity" towards terrains
	std::string & terrStr = strings[4]; // allowed terrains, 1 = object can be placed on this terrain

	assert(terrStr.size() == 9); // all terrains but rock
	for (size_t i=0; i<9; i++)
	{
		if (terrStr[8-i] == '1')
			allowedTerrains.insert(ETerrainType(i));
	}

	id    = Obj(boost::lexical_cast<int>(strings[5]));
	subid = boost::lexical_cast<int>(strings[6]);
	int type  = boost::lexical_cast<int>(strings[7]);
	printPriority = boost::lexical_cast<int>(strings[8]) * 100; // to have some space in future

	if (isOnVisitableFromTopList(id, type))
		visitDir = 0xff;
	else
		visitDir = (8|16|32|64|128);

	readMsk();
}

void ObjectTemplate::readMsk()
{
	ResourceID resID("SPRITES/" + animationFile, EResType::MASK);

	if (CResourceHandler::get()->existsResource(resID))
	{
		auto msk = CResourceHandler::get()->load(resID)->readAll();
		setSize(msk.first.get()[0], msk.first.get()[1]);
	}
	else //maximum possible size of H3 object //TODO: remove hardcode and move this data into modding system
	{
		setSize(8, 6);
	}
}

void ObjectTemplate::readMap(CBinaryReader & reader)
{
	animationFile = reader.readString();

	setSize(TILES_WIDTH, TILES_HEIGHT);
	ui8 blockMask[TILES_HEIGHT];
	ui8 visitMask[TILES_HEIGHT];
	for(auto & byte : blockMask)
		byte = reader.readUInt8();
	for(auto & byte : visitMask)
		byte = reader.readUInt8();

	for (size_t i=0; i< TILES_HEIGHT; i++) // 6 rows
	{
		for (size_t j=0; j< TILES_WIDTH; j++) // 8 columns
		{
			auto & tile = usedTiles[5 - i][7 - j];
			tile |= EBlockMapBits::VISIBLE; // assume that all tiles are visible
			if (((blockMask[i] >> j) & 1 ) == 0)
				tile |= EBlockMapBits::BLOCKED;

			if (((visitMask[i] >> j) & 1 ) != 0)
				tile |= EBlockMapBits::VISITABLE;
		}
	}

	reader.readUInt16();
	ui16 terrMask = reader.readUInt16();
	for (size_t i=0; i<9; i++)
	{
		if (((terrMask >> i) & 1 ) != 0)
			allowedTerrains.insert(ETerrainType(i));
	}

	id = Obj(reader.readUInt32());
	subid = reader.readUInt32();
	int type = reader.readUInt8();
	printPriority = reader.readUInt8() * 100; // to have some space in future

	if (isOnVisitableFromTopList(id, type))
		visitDir = 0xff;
	else
		visitDir = (8|16|32|64|128);

	reader.skip(16);
	readMsk();

	afterLoadFixup();
}

void ObjectTemplate::readJson(const JsonNode &node, const bool withTerrain)
{
	animationFile = node["animation"].String();
	editorAnimationFile = node["editorAnimation"].String();

	const JsonVector & visitDirs = node["visitableFrom"].Vector();
	if (!visitDirs.empty())
	{
		if (visitDirs[0].String()[0] == '+') visitDir |= 1;
		if (visitDirs[0].String()[1] == '+') visitDir |= 2;
		if (visitDirs[0].String()[2] == '+') visitDir |= 4;
		if (visitDirs[1].String()[2] == '+') visitDir |= 8;
		if (visitDirs[2].String()[2] == '+') visitDir |= 16;
		if (visitDirs[2].String()[1] == '+') visitDir |= 32;
		if (visitDirs[2].String()[0] == '+') visitDir |= 64;
		if (visitDirs[1].String()[0] == '+') visitDir |= 128;
	}
	else
		visitDir = 0x00;

	if(withTerrain && !node["allowedTerrains"].isNull())
	{
		for (auto & entry : node["allowedTerrains"].Vector())
			allowedTerrains.insert(ETerrainType(vstd::find_pos(GameConstants::TERRAIN_NAMES, entry.String())));
	}
	else
	{
		for (size_t i=0; i< GameConstants::TERRAIN_TYPES; i++)
			allowedTerrains.insert(ETerrainType(i));

		allowedTerrains.erase(ETerrainType::ROCK);
	}

	if(withTerrain && allowedTerrains.empty())
		logGlobal->warn("Loaded template without allowed terrains!");


	auto charToTile = [&](const char & ch) -> EBlockMapBits
	{
		switch (ch)
		{
			case ' ' : return EBlockMapBits::EMPTY;
			case '0' : return EBlockMapBits::EMPTY;
			case 'V' : return EBlockMapBits::VISIBLE;
			case 'B' : return EBlockMapBits::VISIBLE | EBlockMapBits::BLOCKED;
			case 'H' : return EBlockMapBits::BLOCKED;
			case 'A' : return EBlockMapBits::VISIBLE | EBlockMapBits::BLOCKED | EBlockMapBits::VISITABLE;
			case 'T' : return EBlockMapBits::BLOCKED | EBlockMapBits::VISITABLE;
			default:
				logGlobal->error("Unrecognized char %s in template mask", ch);
				return EBlockMapBits::EMPTY;
		}
	};

	//TODO: init array with 'empty' tiles

	const JsonVector & mask = node["mask"].Vector();

	//calculate width, height
	size_t h = mask.size();
	size_t w  = 0;
	for (auto & line : mask)
		vstd::amax(w, line.String().size());

	setSize(w, h);

	for (size_t i=0; i<mask.size(); i++)
	{
		const std::string & line = mask[i].String();
		for (size_t j=0; j < line.size(); j++)
			usedTiles[mask.size() - 1 - i][line.size() - 1 - j] = charToTile(line[j]);
	}

	printPriority = node["zIndex"].Float();

	afterLoadFixup();
}

void ObjectTemplate::writeJson(JsonNode & node, const bool withTerrain) const
{
	node["animation"].String() = animationFile;
	node["editorAnimation"].String() = editorAnimationFile;

	if(visitDir != 0x0 && isVisitable())
	{
		JsonVector & visitDirs = node["visitableFrom"].Vector();
		visitDirs.resize(3);

		visitDirs[0].String().resize(3);
		visitDirs[1].String().resize(3);
		visitDirs[2].String().resize(3);

		visitDirs[0].String()[0] = (visitDir & 1)   ? '+' : '-';
		visitDirs[0].String()[1] = (visitDir & 2)   ? '+' : '-';
		visitDirs[0].String()[2] = (visitDir & 4)   ? '+' : '-';
		visitDirs[1].String()[2] = (visitDir & 8)   ? '+' : '-';
		visitDirs[2].String()[2] = (visitDir & 16)  ? '+' : '-';
		visitDirs[2].String()[1] = (visitDir & 32)  ? '+' : '-';
		visitDirs[2].String()[0] = (visitDir & 64)  ? '+' : '-';
		visitDirs[1].String()[0] = (visitDir & 128) ? '+' : '-';

		visitDirs[1].String()[1] = '-';
	}

	if(withTerrain)
	{
		//assumed that ROCK terrain not included
		if(allowedTerrains.size() < (GameConstants::TERRAIN_TYPES - 1))
		{
			JsonVector & data = node["allowedTerrains"].Vector();

			for(auto type : allowedTerrains)
			{
				JsonNode value(JsonNode::JsonType::DATA_STRING);
				value.String() = GameConstants::TERRAIN_NAMES[type.num];
				data.push_back(value);
			}
		}
	}

	auto tileToChar = [&](const EBlockMapBits & tile) -> char
	{
		if(tile & EBlockMapBits::VISIBLE)
		{
			if(tile & EBlockMapBits::BLOCKED)
			{
				if(tile & EBlockMapBits::VISITABLE)
					return 'A';
				else
					return 'B';
			}
			else
				return 'V';
		}
		else
		{
			if(tile & EBlockMapBits::BLOCKED)
			{
				if(tile & EBlockMapBits::VISITABLE)
					return 'T';
				else
					return 'H';
			}
			else
				return '0';
		}
	};

	JsonVector & mask = node["mask"].Vector();


	for(size_t i=0; i < height; i++)
	{
		JsonNode lineNode(JsonNode::JsonType::DATA_STRING);

		std::string & line = lineNode.String();
		line.resize(width);
		for(size_t j=0; j < width; j++)
			line[j] = tileToChar(usedTiles[height - 1 - i][width - 1 - j]);
		mask.push_back(lineNode);
	}

	if(printPriority != 0)
		node["zIndex"].Float() = printPriority;
}

ui32 ObjectTemplate::getWidth() const
{
	//TODO: Use 2D array
	//TODO: better precalculate and store constant value
	ui32 ret = 0;
	//TODO: calculate max
	/*
	for (const auto &row : usedTiles) //copy is expensive
	{
		ret = std::max<ui32>(ret, row.size());
	}
	*/
	return ret;
}

ui32 ObjectTemplate::getHeight() const
{
	//return usedTiles.size();
	return usedTiles.shape()[1]; //dimensions are [x][y]
}

void ObjectTemplate::setSize(ui32 width, ui32 height)
{
	usedTiles.resize(boost::extents[width][height]);
	/*
	usedTiles.resize(height);
	for (auto & line : usedTiles)
		line.resize(width, 0);
		*/
}

bool ObjectTemplate::isVisitable() const
{
	for (auto & line : usedTiles)
		for (auto & tile : line)
			if (tile & EBlockMapBits::VISITABLE)
				return true;
	return false;
}

bool ObjectTemplate::isWithin(si32 X, si32 Y) const
{
	if (X < 0 || Y < 0)
		return false;
	return !(X >= width || Y >= height);
}

bool ObjectTemplate::isVisitableAt(si32 X, si32 Y) const
{
	//rotated from [Y][X] to [X][Y] for better performance
	//return isWithin(X, Y) && usedTiles[Y][X] & VISITABLE;
	return isWithin(X, Y) && usedTiles[X][Y] & EBlockMapBits::VISITABLE;
}

bool ObjectTemplate::isVisibleAt(si32 X, si32 Y) const
{
	return isWithin(X, Y) && usedTiles[X][Y] & EBlockMapBits::VISIBLE;
}

bool ObjectTemplate::isBlockedAt(si32 X, si32 Y) const
{
	return isWithin(X, Y) && usedTiles[X][Y] & EBlockMapBits::BLOCKED;
}

std::set<int3> ObjectTemplate::getBlockedOffsets() const
{
	std::set<int3> ret;
	for(int w = 0; w < width; ++w)
	{
		for(int h = 0; h < height; ++h)
		{
			if (isBlockedAt(w, h))
				ret.insert(int3(-w, -h, 0));
		}
	}
	return ret;
}

int3 ObjectTemplate::getBlockMapOffset() const
{
	for(int w = 0; w < width; ++w)
	{
		for(int h = 0; h < height; ++h)
		{
			if (isBlockedAt(w, h))
				return int3(w, h, 0);
		}
	}
	return int3(0,0,0);
}

bool ObjectTemplate::isVisitableFrom(si8 X, si8 Y) const
{
	// visitDir uses format
	// 1 2 3
	// 8   4
	// 7 6 5
	int dirMap[3][3] =
	{
		{ visitDir &   1, visitDir &   2, visitDir &   4 },
		{ visitDir & 128,        1      , visitDir &   8 },
		{ visitDir &  64, visitDir &  32, visitDir &  16 }
	};
	// map input values to range 0..2
	int dx = X < 0 ? 0 : X == 0 ? 1 : 2;
	int dy = Y < 0 ? 0 : Y == 0 ? 1 : 2;

	return dirMap[dy][dx] != 0;
}

int3 ObjectTemplate::getVisitableOffset() const
{
	for(int y = 0; y < width; y++)
		for (int x = 0; x < height; x++)
			if (isVisitableAt(x, y))
				return int3(x,y,0);

    //logGlobal->warn("Warning: getVisitableOffset called on non-visitable obj!");
	return int3(0,0,0);
}

bool ObjectTemplate::isVisitableFromTop() const
{
	return visitDir & 2;
	//for some reason the line below is never called :?
	//return isVisitableFrom (0, 1);
}

bool ObjectTemplate::canBePlacedAt(ETerrainType terrain) const
{
	return allowedTerrains.count(terrain) != 0;
}

