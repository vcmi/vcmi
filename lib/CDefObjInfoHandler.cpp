#include "StdInc.h"
#include "CDefObjInfoHandler.h"

#include "filesystem/Filesystem.h"
#include "filesystem/CBinaryReader.h"
//#include "../client/CGameInfo.h"
#include "../lib/VCMI_Lib.h"
#include "GameConstants.h"
#include "StringConstants.h"
#include "CGeneralTextHandler.h"
#include "CModHandler.h"
#include "JsonNode.h"

/*
 * CDefObjInfoHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static bool isVisitableFromTop(int identifier, int type)
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
	if (vstd::find_pos(visitableFromTop, identifier) != -1)
		return true;
	return false;
}

ObjectTemplate::ObjectTemplate():
	visitDir(8|16|32|64|128), // all but top
	id(Obj::NO_OBJ),
	subid(0),
	printPriority(0)
{
}

void ObjectTemplate::readTxt(CLegacyConfigParser & parser)
{
	std::string data = parser.readString();
	std::vector<std::string> strings;
	boost::split(strings, data, boost::is_any_of(" "));
	assert(strings.size() == 9);

	animationFile = strings[0];

	std::string & blockStr = strings[1]; //block map, 0 = blocked, 1 = unblocked
	std::string & visitStr = strings[2]; //visit map, 1 = visitable, 0 = not visitable

	assert(blockStr.size() == 6*8);
	assert(visitStr.size() == 6*8);

	setSize(8, 6);
	for (size_t i=0; i<6; i++) // 6 rows
	{
		for (size_t j=0; j<8; j++) // 8 columns
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

	if (isVisitableFromTop(id, type))
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

	setSize(8, 6);
	ui8 blockMask[6];
	ui8 visitMask[6];
	for(auto & byte : blockMask)
		byte = reader.readUInt8();
	for(auto & byte : visitMask)
		byte = reader.readUInt8();

	for (size_t i=0; i<6; i++) // 6 rows
	{
		for (size_t j=0; j<8; j++) // 8 columns
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
	for (size_t i=0; i<9; i++)
	{
		if (((terrMask >> i) & 1 ) != 0)
			allowedTerrains.insert(ETerrainType(i));
	}

	id = Obj(reader.readUInt32());
	subid = reader.readUInt32();
	int type = reader.readUInt8();
	printPriority = reader.readUInt8() * 100; // to have some space in future

	if (isVisitableFromTop(id, type))
		visitDir = 0xff;
	else
		visitDir = (8|16|32|64|128);

	reader.skip(16);
	readMsk();

	if (id == Obj::EVENT)
	{
		setSize(1,1);
		usedTiles[0][0] = VISITABLE;
	}
}

void ObjectTemplate::readJson(const JsonNode &node)
{
	id = Obj(node["basebase"].Float()); // temporary, should be replaced
	subid = node["base"].Float();
	animationFile = node["animation"].String();

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
		visitDir = 0xff;

	for (auto & entry : node["allowedTerrains"].Vector())
		allowedTerrains.insert(ETerrainType(vstd::find_pos(GameConstants::TERRAIN_NAMES, entry.String())));

	auto charToTile = [&](const char & ch) -> ui8
	{
		switch (ch)
		{
			case '0' : return 0;
			case 'V' : return VISIBLE;
			case 'B' : return VISIBLE | BLOCKED;
			case 'H' : return BLOCKED;
			case 'A' : return VISIBLE | BLOCKED | VISITABLE;
			case 'T' : return VISIBLE | VISITABLE;
			default:
				logGlobal->errorStream() << "Unrecognized char " << ch << " in template mask";
				return 0;
		}
	};

	size_t maxHeight = 0, maxWidth = 0;
	size_t width  = node["mask"].Vector()[0].String().size();
	size_t height = node["mask"].Vector().size();
	setSize(width, height);

	for (size_t i=0; i<height; i++)
	{
		const std::string & line = node["mask"].Vector()[i].String();
		assert(line.size() == width);
		for (size_t j=0; j < width; j++)
		{
			ui8 tile = charToTile(line[j]);
			if (tile != 0)
			{
				vstd::amax(maxHeight, i);
				vstd::amax(maxWidth,  j);
				usedTiles[i][j] = tile;
			}
		}
	}
	setSize(maxWidth, maxHeight);

	printPriority = node["zIndex"].Float();
}

ui32 ObjectTemplate::getWidth() const
{
	return usedTiles.empty() ? 0 : usedTiles.front().size();
}

ui32 ObjectTemplate::getHeight() const
{
	return usedTiles.size();
}

void ObjectTemplate::setSize(ui32 width, ui32 height)
{
	usedTiles.resize(height);
	for (auto & line : usedTiles)
		line.resize(width);
}

bool ObjectTemplate::isVisitable() const
{
	for (auto & line : usedTiles)
		for (auto & tile : line)
			if (tile & VISITABLE)
				return true;
	return false;
}

bool ObjectTemplate::isWithin(si32 X, si32 Y) const
{
	if (X < 0 || Y < 0)
		return false;
	if (X >= getWidth() || Y >= getHeight())
		return false;
	return true;
}

bool ObjectTemplate::isVisitableAt(si32 X, si32 Y) const
{
	if (isWithin(X, Y))
		return usedTiles[Y][X] & VISITABLE;
	return false;
}

bool ObjectTemplate::isVisibleAt(si32 X, si32 Y) const
{
	if (isWithin(X, Y))
		return usedTiles[Y][X] & VISIBLE;
	return false;
}

bool ObjectTemplate::isBlockedAt(si32 X, si32 Y) const
{
	if (isWithin(X, Y))
		return usedTiles[Y][X] & BLOCKED;
	return false;
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

bool ObjectTemplate::canBePlacedAt(ETerrainType terrain) const
{
	return allowedTerrains.count(terrain) != 0;
}

void CDefObjInfoHandler::readTextFile(std::string path)
{
	CLegacyConfigParser parser(path);
	size_t totalNumber = parser.readNumber(); // first line contains number of objects to read and nothing else
	parser.endLine();

	for (size_t i=0; i<totalNumber; i++)
	{
		ObjectTemplate templ;
		templ.readTxt(parser);
		parser.endLine();
		objects.push_back(templ);
	}
}

CDefObjInfoHandler::CDefObjInfoHandler()
{
	readTextFile("Data/Objects.txt");
	readTextFile("Data/Heroes.txt");
/*
	const JsonNode node = JsonUtils::assembleFromFiles("config/objectTemplates.json");
	std::vector<ObjectTemplate> newTemplates;
	newTemplates.reserve(node.Struct().size());

	// load all new templates
	for (auto & entry : node.Struct())
	{
		ObjectTemplate templ;
		templ.readJson(entry.second);
		newTemplates.push_back(templ);
	}

	// erase old ones to avoid conflicts
	for (auto & entry : newTemplates)
		eraseAll(entry.id, entry.subid);

	// merge new templates into storage
	objects.insert(objects.end(), newTemplates.begin(), newTemplates.end());
*/
}

void CDefObjInfoHandler::eraseAll(Obj type, si32 subtype)
{
	auto it = std::remove_if(objects.begin(), objects.end(), [&](const ObjectTemplate & obj)
	{
		return obj.id == type && obj.subid == subtype;
	});
	objects.erase(it, objects.end());
}

void CDefObjInfoHandler::registerTemplate(ObjectTemplate obj)
{
	objects.push_back(obj);
}

std::vector<ObjectTemplate> CDefObjInfoHandler::pickCandidates(Obj type, si32 subtype) const
{
	std::vector<ObjectTemplate> ret;

	std::copy_if(objects.begin(), objects.end(), std::back_inserter(ret), [&](const ObjectTemplate & obj)
	{
		return obj.id == type && obj.subid == subtype;
	});
	if (ret.empty())
		logGlobal->errorStream() << "Failed to find template for " << type << ":" << subtype;
	assert(!ret.empty()); // Can't create object of this type/subtype
	return ret;
}

std::vector<ObjectTemplate> CDefObjInfoHandler::pickCandidates(Obj type, si32 subtype, ETerrainType terrain) const
{
	std::vector<ObjectTemplate> ret = pickCandidates(type, subtype);
	std::vector<ObjectTemplate> filtered;

	std::copy_if(ret.begin(), ret.end(), std::back_inserter(filtered), [&](const ObjectTemplate & obj)
	{
		return obj.canBePlacedAt(terrain);
	});
	// it is possible that there are no templates usable on specific terrain. In this case - return list before filtering
	return filtered.empty() ? ret : filtered;
}
