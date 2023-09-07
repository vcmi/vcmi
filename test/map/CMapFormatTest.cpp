/*
 * CMapFormatTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/JsonDetail.h"

#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/filesystem/Filesystem.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/mapping/MapFormatJson.h"

#include "../lib/VCMIDirs.h"

#include "MapComparer.h"
#include "../JsonComparer.h"
#include "mock/ZoneOptionsFake.h"

static const int TEST_RANDOM_SEED = 1337;

static void saveTestMap(CMemoryBuffer & serializeBuffer, const std::string & filename)
{
	auto path = VCMIDirs::get().userDataPath() / filename;
	boost::filesystem::remove(path);
	std::ofstream tmp(path.c_str(), std::ofstream::binary);

	tmp.write((const char *)serializeBuffer.getBuffer().data(), serializeBuffer.getSize());
	tmp.flush();
	tmp.close();
}

TEST(MapFormat, Random)
{
	SCOPED_TRACE("MapFormat_Random start");

	CMapGenOptions opt;
	CRmgTemplate tmpl;
	std::shared_ptr<ZoneOptionsFake> zoneOptions = std::make_shared<ZoneOptionsFake>();

	const_cast<CRmgTemplate::CPlayerCountRange &>(tmpl.getCpuPlayers()).addRange(1, 4);
	const_cast<CRmgTemplate::Zones &>(tmpl.getZones())[0] = zoneOptions;

	zoneOptions->setOwner(1);
	opt.setMapTemplate(&tmpl);

	opt.setHeight(CMapHeader::MAP_SIZE_MIDDLE);
	opt.setWidth(CMapHeader::MAP_SIZE_MIDDLE);
	opt.setHasTwoLevels(true);
	opt.setPlayerCount(4);

	opt.setPlayerTypeForStandardPlayer(PlayerColor(0), EPlayerType::HUMAN);
	opt.setPlayerTypeForStandardPlayer(PlayerColor(1), EPlayerType::AI);
	opt.setPlayerTypeForStandardPlayer(PlayerColor(2), EPlayerType::AI);
	opt.setPlayerTypeForStandardPlayer(PlayerColor(3), EPlayerType::AI);

	CMapGenerator gen(opt, TEST_RANDOM_SEED);

	std::unique_ptr<CMap> initialMap = gen.generate();
	initialMap->name = "Test";
	SCOPED_TRACE("MapFormat_Random generated");

	CMemoryBuffer serializeBuffer;
	{
		CMapSaverJson saver(&serializeBuffer);
		saver.saveMap(initialMap);
	}
	SCOPED_TRACE("MapFormat_Random serialized");
	saveTestMap(serializeBuffer, "test_random.vmap");
	SCOPED_TRACE("MapFormat_Random saved");

	serializeBuffer.seek(0);
	{
		CMapLoaderJson loader(&serializeBuffer);
		std::unique_ptr<CMap> serialized = loader.loadMap();

		MapComparer c;
		c(serialized, initialMap);
	}
	SCOPED_TRACE("MapFormat_Random finish");
}

static JsonNode getFromArchive(CZipLoader & archive, const std::string & archiveFilename)
{
	JsonPath resource = JsonPath::builtin(archiveFilename);

	if(!archive.existsResource(resource))
		throw std::runtime_error(archiveFilename + " not found");

	auto data = archive.load(resource)->readAll();

	JsonNode res(reinterpret_cast<char*>(data.first.get()), data.second);

	return res;
}

static void addToArchive(CZipSaver & saver, const JsonNode & data, const std::string & filename)
{
	auto s = data.toJson();
	std::unique_ptr<COutputStream> stream = saver.addFile(filename);

	if(stream->write((const ui8*)s.c_str(), s.size()) != s.size())
		throw std::runtime_error("CMapSaverJson::saveHeader() zip compression failed.");
}

static std::unique_ptr<CMap> loadOriginal(const JsonNode & header, const JsonNode & objects, const JsonNode & surface, const JsonNode & underground)
{
	CMemoryBuffer initialBuffer;

	std::shared_ptr<CIOApi> originalDataIO(new CProxyIOApi(&initialBuffer));

	{
		CZipSaver initialSaver(originalDataIO, "_");

		addToArchive(initialSaver, header, CMapFormatJson::HEADER_FILE_NAME);
		addToArchive(initialSaver, objects, CMapFormatJson::OBJECTS_FILE_NAME);
		addToArchive(initialSaver, surface, "surface_terrain.json");
		addToArchive(initialSaver, underground, "underground_terrain.json");
	}

	initialBuffer.seek(0);

	CMapLoaderJson initialLoader(&initialBuffer);

	return initialLoader.loadMap();
}

static void loadActual(CMemoryBuffer * serializeBuffer, const std::unique_ptr<CMap> & originalMap, JsonNode & header, JsonNode & objects, JsonNode & surface, JsonNode & underground)
{
	{
		CMapSaverJson saver(serializeBuffer);
		saver.saveMap(originalMap);
	}

	std::shared_ptr<CIOApi> actualDataIO(new CProxyROIOApi(serializeBuffer));
	CZipLoader actualDataLoader("", "_", actualDataIO);

	header = getFromArchive(actualDataLoader, CMapFormatJson::HEADER_FILE_NAME);
	objects = getFromArchive(actualDataLoader, CMapFormatJson::OBJECTS_FILE_NAME);
	surface = getFromArchive(actualDataLoader, "surface_terrain.json");
	underground = getFromArchive(actualDataLoader, "underground_terrain.json");
}

TEST(MapFormat, Objects)
{
	static const std::string MAP_DATA_PATH = "test/ObjectPropertyTest/";

	const JsonNode initialHeader(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode expectedHeader(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));//same as initial for now

	const JsonNode initialObjects(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::OBJECTS_FILE_NAME));
	const JsonNode expectedObjects(JsonPath::builtin(MAP_DATA_PATH + "objects.ex.json"));

	const JsonNode expectedSurface(JsonPath::builtin(MAP_DATA_PATH + "surface_terrain.json"));
	const JsonNode expectedUnderground(JsonPath::builtin(MAP_DATA_PATH + "underground_terrain.json"));

	std::unique_ptr<CMap> originalMap = loadOriginal(initialHeader, initialObjects, expectedSurface, expectedUnderground);

	CMemoryBuffer serializeBuffer;

	JsonNode actualHeader;
	JsonNode actualObjects;
	JsonNode actualSurface;
	JsonNode actualUnderground;

	loadActual(&serializeBuffer, originalMap, actualHeader, actualObjects, actualSurface, actualUnderground);

	saveTestMap(serializeBuffer, "test_object_property.vmap");

	{
		JsonComparer c(false);
		c.compare("hdr", actualHeader, expectedHeader);
		c.compare("obj", actualObjects, expectedObjects);
	}

	{
		JsonComparer c(true);
		c.compare("surface", actualSurface, expectedSurface);
		c.compare("underground", actualUnderground, expectedUnderground);
	}
}

TEST(MapFormat, Terrain)
{
	static const std::string MAP_DATA_PATH = "test/TerrainTest/";

	const JsonNode expectedHeader(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode expectedObjects(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::OBJECTS_FILE_NAME));

	const JsonNode expectedSurface(JsonPath::builtin(MAP_DATA_PATH + "surface_terrain.json"));
	const JsonNode expectedUnderground(JsonPath::builtin(MAP_DATA_PATH + "underground_terrain.json"));

	std::unique_ptr<CMap> originalMap = loadOriginal(expectedHeader, expectedObjects, expectedSurface, expectedUnderground);

	CMemoryBuffer serializeBuffer;

	JsonNode actualHeader;
	JsonNode actualObjects;
	JsonNode actualSurface;
	JsonNode actualUnderground;

	loadActual(&serializeBuffer, originalMap, actualHeader, actualObjects, actualSurface, actualUnderground);

	saveTestMap(serializeBuffer, "test_terrain.vmap");

	{
		JsonComparer c(false);
		c.compare("hdr", actualHeader, expectedHeader);
		c.compare("obj", actualObjects, expectedObjects);
	}

	{
		JsonComparer c(true);
		c.compare("surface", actualSurface, expectedSurface);
		c.compare("underground", actualUnderground, expectedUnderground);
	}
}
