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
#include "../TerrainHandler.h"

#include "CRewardableConstructor.h"

VCMI_LIB_NAMESPACE_BEGIN

static bool isOnVisitableFromTopList(int identifier, int type)
{
	if(type == 2 || type == 3 || type == 4 || type == 5) //creature, hero, artifact, resource
		return true;

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
	width(0),
	height(0),
	visitable(false)
{
}

ObjectTemplate::ObjectTemplate(const ObjectTemplate& other):
	visitDir(other.visitDir),
	allowedTerrains(other.allowedTerrains),
	id(other.id),
	subid(other.subid),
	printPriority(other.printPriority),
	animationFile(other.animationFile),
	editorAnimationFile(other.editorAnimationFile),
	stringID(other.stringID),
	width(other.width),
	height(other.height),
	visitable(other.visitable),
	blockedOffsets(other.blockedOffsets),
	blockMapOffset(other.blockMapOffset),
	visitableOffset(other.visitableOffset)
{
	//default copy constructor is failing with usedTiles this for unknown reason

	usedTiles.resize(other.usedTiles.size());
	for(size_t i = 0; i < usedTiles.size(); i++)
		std::copy(other.usedTiles[i].begin(), other.usedTiles[i].end(), std::back_inserter(usedTiles[i]));
}

ObjectTemplate & ObjectTemplate::operator=(const ObjectTemplate & rhs)
{
	visitDir = rhs.visitDir;
	allowedTerrains = rhs.allowedTerrains;
	id = rhs.id;
	subid = rhs.subid;
	printPriority = rhs.printPriority;
	animationFile = rhs.animationFile;
	editorAnimationFile = rhs.editorAnimationFile;
	stringID = rhs.stringID;
	width = rhs.width;
	height = rhs.height;
	visitable = rhs.visitable;
	blockedOffsets = rhs.blockedOffsets;
	blockMapOffset = rhs.blockMapOffset;
	visitableOffset = rhs.visitableOffset;

	usedTiles.clear();
	usedTiles.resize(rhs.usedTiles.size());
	for(size_t i = 0; i < usedTiles.size(); i++)
		std::copy(rhs.usedTiles[i].begin(), rhs.usedTiles[i].end(), std::back_inserter(usedTiles[i]));

	return *this;
}

void ObjectTemplate::afterLoadFixup()
{
	if(id == Obj::EVENT)
	{
		setSize(1,1);
		usedTiles[0][0] = VISITABLE;
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

	assert(blockStr.size() == 6*8);
	assert(visitStr.size() == 6*8);

	setSize(8, 6);
	for(size_t i = 0; i < 6; i++) // 6 rows
	{
		for(size_t j = 0; j < 8; j++) // 8 columns
		{
			auto & tile = usedTiles[i][j];
			tile |= VISIBLE; // assume that all tiles are visible
			if (blockStr[i*8 + j] == '0')
				tile |= BLOCKED;

			if (visitStr[i*8 + j] == '1')
				tile |= VISITABLE;
		}
	}

	// strings[3] most likely - terrains on which this object can be placed in editor.
	// e.g. Whirpool can be placed manually only on water while mines can be placed everywhere despite terrain-specific gfx
	// so these two fields can be interpreted as "strong affinity" and "weak affinity" towards terrains
	std::string & terrStr = strings[4]; // allowed terrains, 1 = object can be placed on this terrain

	assert(terrStr.size() == TerrainId(ETerrainId::ROCK).getNum()); // all terrains but rock - counting from 0
	for(TerrainId i = TerrainId(0); i < ETerrainId::ORIGINAL_REGULAR_TERRAIN_COUNT; ++i)
	{
		if (terrStr[8-i.getNum()] == '1')
			allowedTerrains.insert(i);
	}
	
	//assuming that object can be placed on other land terrains
	anyTerrain = allowedTerrains.size() >= 8 && !allowedTerrains.count(ETerrainId::WATER);

	id    = Obj(boost::lexical_cast<int>(strings[5]));
	subid = boost::lexical_cast<int>(strings[6]);
	int type  = boost::lexical_cast<int>(strings[7]);
	printPriority = boost::lexical_cast<int>(strings[8]) * 100; // to have some space in future

	if (isOnVisitableFromTopList(id, type))
		visitDir = 0xff;
	else
		visitDir = (8|16|32|64|128);

	readMsk();
	recalculate();
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
	animationFile = reader.readBaseString();

	setSize(8, 6);
	ui8 blockMask[6];
	ui8 visitMask[6];
	for(auto & byte : blockMask)
		byte = reader.readUInt8();
	for(auto & byte : visitMask)
		byte = reader.readUInt8();

	for(size_t i = 0; i < 6; i++) // 6 rows
	{
		for(size_t j = 0; j < 8; j++) // 8 columns
		{
			auto & tile = usedTiles[5 - i][7 - j];
			tile |= VISIBLE; // assume that all tiles are visible
			if (((blockMask[i] >> j) & 1 ) == 0)
				tile |= BLOCKED;

			if (((visitMask[i] >> j) & 1 ) != 0)
				tile |= VISITABLE;
		}
	}

	reader.readUInt16();
	ui16 terrMask = reader.readUInt16();
	for(TerrainId i = ETerrainId::FIRST_REGULAR_TERRAIN; i < ETerrainId::ORIGINAL_REGULAR_TERRAIN_COUNT; ++i)
	{
		if (((terrMask >> i.getNum()) & 1 ) != 0)
			allowedTerrains.insert(i);
	}
	
	//assuming that object can be placed on other land terrains
	anyTerrain = allowedTerrains.size() >= 8 && !allowedTerrains.count(ETerrainId::WATER);

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
	recalculate();
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
		for(const auto & entry : node["allowedTerrains"].Vector())
		{
			VLC->modh->identifiers.requestIdentifier("terrain", entry, [this](int32_t identifier){
				allowedTerrains.insert(TerrainId(identifier));
			});
		}
		anyTerrain = false;
	}
	else
	{
		anyTerrain = true;
	}

	auto charToTile = [&](const char & ch) -> ui8
	{
		switch (ch)
		{
		case ' ' : return 0;
		case '0' : return 0;
		case 'V' : return VISIBLE;
		case 'B' : return VISIBLE | BLOCKED;
		case 'H' : return BLOCKED;
		case 'A' : return VISIBLE | BLOCKED | VISITABLE;
		case 'T' : return BLOCKED | VISITABLE;
		default:
			logGlobal->error("Unrecognized char %s in template mask", ch);
			return 0;
		}
	};

	const JsonVector & mask = node["mask"].Vector();

	size_t height = mask.size();
	size_t width  = 0;
	for(const auto & line : mask)
		vstd::amax(width, line.String().size());

	setSize(static_cast<ui32>(width), static_cast<ui32>(height));

	for(size_t i = 0; i < mask.size(); i++)
	{
		const std::string & line = mask[i].String();
		for(size_t j = 0; j < line.size(); j++)
			usedTiles[mask.size() - 1 - i][line.size() - 1 - j] = charToTile(line[j]);
	}

	printPriority = static_cast<si32>(node["zIndex"].Float());

	afterLoadFixup();
	recalculate();
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
		//assumed that ROCK and WATER terrains are not included
		if(allowedTerrains.size() < (VLC->terrainTypeHandler->objects.size() - 2))
		{
			JsonVector & data = node["allowedTerrains"].Vector();

			for(auto type : allowedTerrains)
			{
				JsonNode value(JsonNode::JsonType::DATA_STRING);
				value.String() = VLC->terrainTypeHandler->getById(type)->getJsonKey();
				data.push_back(value);
			}
		}
	}

	auto tileToChar = [&](const ui8 & tile) -> char
	{
		if(tile & VISIBLE)
		{
			if(tile & BLOCKED)
			{
				if(tile & VISITABLE)
					return 'A';
				else
					return 'B';
			}
			else
				return 'V';
		}
		else
		{
			if(tile & BLOCKED)
			{
				if(tile & VISITABLE)
					return 'T';
				else
					return 'H';
			}
			else
				return '0';
		}
	};

	size_t height = getHeight();
	size_t width  = getWidth();

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

void ObjectTemplate::calculateWidth()
{
	//TODO: Use 2D array
	for(const auto& row : usedTiles) //copy is expensive
	{
		width = std::max<ui32>(width, static_cast<ui32>(row.size()));
	}
}

void ObjectTemplate::calculateHeight()
{
	//TODO: Use 2D array
	height = static_cast<ui32>(usedTiles.size());
}

void ObjectTemplate::setSize(ui32 width, ui32 height)
{
	usedTiles.resize(height);
	for(auto & line : usedTiles)
		line.resize(width, 0);
}

void ObjectTemplate::calculateVisitable()
{
	for(auto& line : usedTiles)
	{
		for(auto& tile : line)
		{
			if (tile & VISITABLE)
			{
				visitable = true;
				return;
			}
		}
	}
	visitable = false;
}

bool ObjectTemplate::isWithin(si32 X, si32 Y) const
{
	if (X < 0 || Y < 0)
		return false;
	return X < static_cast<si32>(getWidth()) && Y < static_cast<si32>(getHeight());
}

bool ObjectTemplate::isVisitableAt(si32 X, si32 Y) const
{
	return isWithin(X, Y) && usedTiles[Y][X] & VISITABLE;
}

bool ObjectTemplate::isVisibleAt(si32 X, si32 Y) const
{
	return isWithin(X, Y) && usedTiles[Y][X] & VISIBLE;
}

bool ObjectTemplate::isBlockedAt(si32 X, si32 Y) const
{
	return isWithin(X, Y) && usedTiles[Y][X] & BLOCKED;
}

void ObjectTemplate::calculateBlockedOffsets()
{
	blockedOffsets.clear();
	for(int w = 0; w < static_cast<int>(getWidth()); ++w)
	{
		for(int h = 0; h < static_cast<int>(getHeight()); ++h)
		{
			if (isBlockedAt(w, h))
				blockedOffsets.insert(int3(-w, -h, 0));
		}
	}
}

void ObjectTemplate::calculateBlockMapOffset()
{
	for(int w = 0; w < static_cast<int>(getWidth()); ++w)
	{
		for(int h = 0; h < static_cast<int>(getHeight()); ++h)
		{
			if (isBlockedAt(w, h))
			{
				blockMapOffset = int3(w, h, 0);
				return;
			}
		}
	}
	blockMapOffset = int3(0, 0, 0);
}

bool ObjectTemplate::isVisitableFrom(si8 X, si8 Y) const
{
	// visitDir uses format
	// 1 2 3
	// 8   4
	// 7 6 5
	//TODO: static? cached?
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

void ObjectTemplate::calculateTopVisibleOffset()
{
	for(int y = static_cast<int>(getHeight()) - 1; y >= 0; y--) //Templates start from bottom-right corner
	{
		for(int x = 0; x < static_cast<int>(getWidth()); x++)
		{
			if (isVisibleAt(x, y))
			{
				topVisibleOffset = int3(x, y, 0);
				return;
			}
		}
	}
	topVisibleOffset = int3(0, 0, 0);
}

void ObjectTemplate::calculateVisitableOffset()
{
	for(int y = 0; y < static_cast<int>(getHeight()); y++)
	{
		for(int x = 0; x < static_cast<int>(getWidth()); x++)
		{
			if (isVisitableAt(x, y))
			{
				visitableOffset = int3(x, y, 0);
				return;
			}
		}
	}
	visitableOffset = int3(0, 0, 0);
}

bool ObjectTemplate::canBePlacedAt(TerrainId terrainID) const
{
	if (anyTerrain)
	{
		const auto & terrain = VLC->terrainTypeHandler->getById(terrainID);
		return terrain->isLand() && terrain->isPassable();
	}
	return vstd::contains(allowedTerrains, terrainID);
}

void ObjectTemplate::recalculate()
{
	calculateWidth();
	calculateHeight();
	calculateVisitable();
	//The lines below use width and height
	calculateBlockedOffsets();
	calculateBlockMapOffset();
	calculateVisitableOffset();
	calculateTopVisibleOffset();

	if (visitable && visitDir == 0)
		logMod->warn("Template for %s is visitable but has no visitable directions!", animationFile);
}

VCMI_LIB_NAMESPACE_END
