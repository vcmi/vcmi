#include "StdInc.h"
#include "CDefObjInfoHandler.h"

#include "filesystem/Filesystem.h"
#include "filesystem/CBinaryReader.h"
#include "../lib/VCMI_Lib.h"
#include "GameConstants.h"
#include "StringConstants.h"
#include "CGeneralTextHandler.h"
#include "CObjectHandler.h"
#include "CModHandler.h"
#include "JsonNode.h"

#include "CObjectConstructor.h"

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
	stringID = strings[0];

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
	//id = Obj(node["basebase"].Float()); // temporary, should be removed and determined indirectly via object type parent (e.g. base->base)
	//subid = node["base"].Float();
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
		visitDir = 0x00;

	if (!node["allowedTerrains"].isNull())
	{
		for (auto & entry : node["allowedTerrains"].Vector())
			allowedTerrains.insert(ETerrainType(vstd::find_pos(GameConstants::TERRAIN_NAMES, entry.String())));
	}
	else
	{
		for (size_t i=0; i< GameConstants::TERRAIN_TYPES; i++)
			allowedTerrains.insert(ETerrainType(i));
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
				logGlobal->errorStream() << "Unrecognized char " << ch << " in template mask";
				return 0;
		}
	};

	const JsonVector & mask = node["mask"].Vector();

	size_t height = mask.size();
	size_t width  = 0;
	for (auto & line : mask)
		vstd::amax(width, line.String().size());

	setSize(width, height);

	for (size_t i=0; i<mask.size(); i++)
	{
		const std::string & line = mask[i].String();
		for (size_t j=0; j < line.size(); j++)
			usedTiles[mask.size() - 1 - i][line.size() - 1 - j] = charToTile(line[j]);
	}

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
		line.resize(width, 0);
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
/*
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
}
*/

CObjectClassesHandler::CObjectClassesHandler()
{
	// list of all known handlers, hardcoded for now since the only way to add new objects is via C++ code
	handlerConstructors["configurable"] = std::make_shared<CObjectWithRewardConstructor>;

#define SET_HANDLER(STRING, TYPENAME) handlerConstructors[STRING] = std::make_shared<CDefaultObjectTypeHandler<TYPENAME> >

	SET_HANDLER("", CGObjectInstance);
	SET_HANDLER("generic", CGObjectInstance);

	SET_HANDLER("market", CGMarket);
	SET_HANDLER("bank", CBank);
	SET_HANDLER("cartographer", CCartographer);
	SET_HANDLER("artifact", CGArtifact);
	SET_HANDLER("blackMarket", CGBlackMarket);
	SET_HANDLER("boat", CGBoat);
	SET_HANDLER("bonusingObject", CGBonusingObject);
	SET_HANDLER("borderGate", CGBorderGate);
	SET_HANDLER("borderGuard", CGBorderGuard);
	SET_HANDLER("monster", CGCreature);
	SET_HANDLER("denOfThieves", CGDenOfthieves);
	SET_HANDLER("dwelling", CGDwelling);
	SET_HANDLER("event", CGEvent);
	SET_HANDLER("garrison", CGGarrison);
	SET_HANDLER("hero", CGHeroInstance);
	SET_HANDLER("heroPlaceholder", CGHeroPlaceholder);
	SET_HANDLER("keymaster", CGKeymasterTent);
	SET_HANDLER("lighthouse", CGLighthouse);
	SET_HANDLER("magi", CGMagi);
	SET_HANDLER("magicSpring", CGMagicSpring);
	SET_HANDLER("magicWell", CGMagicWell);
	SET_HANDLER("market", CGMarket);
	SET_HANDLER("mine", CGMine);
	SET_HANDLER("obelisk", CGObelisk);
	SET_HANDLER("observatory", CGObservatory);
	SET_HANDLER("onceVisitable", CGOnceVisitable);
	SET_HANDLER("pandora", CGPandoraBox);
	SET_HANDLER("pickable", CGPickable);
	SET_HANDLER("pyramid", CGPyramid);
	SET_HANDLER("questGuard", CGQuestGuard);
	SET_HANDLER("resource", CGResource);
	SET_HANDLER("scholar", CGScholar);
	SET_HANDLER("seerHut", CGSeerHut);
	SET_HANDLER("shipyard", CGShipyard);
	SET_HANDLER("shrine", CGShrine);
	SET_HANDLER("sign", CGSignBottle);
	SET_HANDLER("siren", CGSirens);
	SET_HANDLER("teleport", CGTeleport);
	SET_HANDLER("town", CGTownInstance);
	SET_HANDLER("university", CGUniversity);
	SET_HANDLER("oncePerHero", CGVisitableOPH);
	SET_HANDLER("oncePerWeek", CGVisitableOPW);
	SET_HANDLER("witch", CGWitchHut);

#undef SET_HANDLER
}

static std::vector<JsonNode> readTextFile(std::string path)
{
	//TODO
}

std::vector<JsonNode> CObjectClassesHandler::loadLegacyData(size_t dataSize)
{
	std::vector<JsonNode> ret(dataSize);// create storage for 256 objects

	auto parseFile = [&](std::string filename)
	{
		auto entries = readTextFile(filename);
		for (JsonNode & entry : entries)
		{
			si32 id = entry["basebase"].Float();
			si32 subid = entry["base"].Float();

			entry.Struct().erase("basebase");
			entry.Struct().erase("base");

			if (ret[id].Vector().size() <= subid)
				ret[id].Vector().resize(subid+1);
			ret[id]["legacyTypes"].Vector()[subid][entry["animation"].String()].swap(entry);
		}
	};

	parseFile("Data/Objects.txt");
	parseFile("Data/Heroes.txt");

	CLegacyConfigParser parser("Data/ObjNames.txt");
	for (size_t i=0; i<256; i++)
	{
		ret[i]["name"].String() = parser.readString();
		parser.endLine();
	}
	return ret;
}

CObjectClassesHandler::ObjectContainter * CObjectClassesHandler::loadFromJson(const JsonNode & json)
{
	auto obj = new ObjectContainter();
	obj->name = json["name"].String();
	obj->handlerName = json["handler"].String();
	obj->base = json["base"];
	for (auto entry : json["types"].Struct())
	{
		auto handler = handlerConstructors.at(obj->handlerName)();
		handler->init(entry.second);
	}
	return obj;
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
}

std::vector<bool> CObjectClassesHandler::getDefaultAllowed() const
{
	return std::vector<bool>(); //TODO?
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(si32 type, si32 subtype) const
{
	if (objects.count(type))
	{
		if (objects.at(type)->objects.count(subtype))
			return objects.at(type)->objects.at(subtype);
	}
	assert(0); // FIXME: throw error?
	return nullptr;
}

std::string CObjectClassesHandler::getObjectName(si32 type) const
{
	assert(objects.count(type));
	return objects.at(type)->name;
}

void AObjectTypeHandler::setType(si32 type, si32 subtype)
{
	this->type = type;
	this->subtype = subtype;
}

void AObjectTypeHandler::init(const JsonNode & input)
{
	for (auto entry : input["templates"].Struct())
	{
		JsonNode data = input["base"];
		JsonUtils::merge(data, entry.second);

		ObjectTemplate tmpl;
		tmpl.id = Obj(type);
		tmpl.subid = subtype;
		tmpl.stringID = entry.first; // FIXME: create "fullID" - type.object.template?
		tmpl.readJson(data);
		templates.push_back(tmpl);
	}
}

bool AObjectTypeHandler::objectFilter(const CGObjectInstance *, const ObjectTemplate &) const
{
	return true; // by default - accept all.
}

void AObjectTypeHandler::addTemplate(const ObjectTemplate & templ)
{
	templates.push_back(templ);
}

std::vector<ObjectTemplate> AObjectTypeHandler::getTemplates() const
{
	return templates;
}

std::vector<ObjectTemplate> AObjectTypeHandler::getTemplates(si32 terrainType) const// FIXME: replace with ETerrainType
{
	std::vector<ObjectTemplate> ret = getTemplates();
	std::vector<ObjectTemplate> filtered;

	std::copy_if(ret.begin(), ret.end(), std::back_inserter(filtered), [&](const ObjectTemplate & obj)
	{
		return obj.canBePlacedAt(ETerrainType(terrainType));
	});
	// it is possible that there are no templates usable on specific terrain. In this case - return list before filtering
	return filtered.empty() ? ret : filtered;
}

boost::optional<ObjectTemplate> AObjectTypeHandler::getOverride(si32 terrainType, const CGObjectInstance * object) const
{
	std::vector<ObjectTemplate> ret = getTemplates(terrainType);
	for (auto & tmpl : ret)
	{
		if (objectFilter(object, tmpl))
			return tmpl;
	}
	return boost::optional<ObjectTemplate>();
}
