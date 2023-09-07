/*
 * mock_MapService.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_MapService.h"

#include "../../lib/filesystem/Filesystem.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatJson.h"

MapServiceMock::MapServiceMock(const std::string & path, MapListener * mapListener_)
	: mapListener(mapListener_)
{
	std::shared_ptr<CIOApi> io(new CProxyIOApi(&initialBuffer));

	CZipSaver saver(io, "_");

	const JsonNode header(JsonPath::builtin(path+CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode objects(JsonPath::builtin(path+CMapFormatJson::OBJECTS_FILE_NAME));
	const JsonNode surface(JsonPath::builtin(path+"surface_terrain.json"));

	addToArchive(saver, header, CMapFormatJson::HEADER_FILE_NAME);
	addToArchive(saver, objects, CMapFormatJson::OBJECTS_FILE_NAME);
	addToArchive(saver, surface, "surface_terrain.json");

	auto undergroundPath = JsonPath::builtin(path+"underground_terrain.json");

    if(CResourceHandler::get()->existsResource(undergroundPath))
	{
		const JsonNode underground(undergroundPath);
		addToArchive(saver, underground, "underground_terrain.json");
	}
}

std::unique_ptr<CMap> MapServiceMock::loadMap() const
{
	initialBuffer.seek(0);
	CMapLoaderJson initialLoader(&initialBuffer);

	std::unique_ptr<CMap> res = initialLoader.loadMap();

	if(mapListener)
		mapListener->mapLoaded(res.get());

	return res;
}

std::unique_ptr<CMap> MapServiceMock::loadMap(const ResourcePath & name) const
{
	return loadMap();
}

std::unique_ptr<CMapHeader> MapServiceMock::loadMapHeader(const ResourcePath & name) const
{
	initialBuffer.seek(0);
	CMapLoaderJson initialLoader(&initialBuffer);
	return initialLoader.loadMapHeader();
}

std::unique_ptr<CMap> MapServiceMock::loadMap(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const
{
	return loadMap();
}

std::unique_ptr<CMapHeader> MapServiceMock::loadMapHeader(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const
{
	initialBuffer.seek(0);
	CMapLoaderJson initialLoader(&initialBuffer);
	return initialLoader.loadMapHeader();
}

void MapServiceMock::saveMap(const std::unique_ptr<CMap> & map, boost::filesystem::path fullPath) const
{
	FAIL() << "Unexpected call to MapServiceMock::saveMap";
}

void MapServiceMock::addToArchive(CZipSaver & saver, const JsonNode & data, const std::string & filename)
{
	auto s = data.toJson();
	std::unique_ptr<COutputStream> stream = saver.addFile(filename);

	if(stream->write((const ui8*)s.c_str(), s.size()) != s.size())
		throw std::runtime_error("addToArchive: zip compression failed.");
}
