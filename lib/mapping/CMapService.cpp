#include "StdInc.h"
#include "CMapService.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CBinaryReader.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "CMap.h"

#include "MapFormatH3M.h"
#include "MapFormatJson.h"


std::unique_ptr<CMap> CMapService::loadMap(const std::string & name)
{
	auto stream = getStreamFromFS(name);
	std::unique_ptr<CMap> map(getMapLoader(stream)->loadMap());
	std::unique_ptr<CMapHeader> header(map.get());

	getMapPatcher(name)->patchMapHeader(header);
	header.release();

	return map;
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const std::string & name)
{
	auto stream = getStreamFromFS(name);
	std::unique_ptr<CMapHeader> header = getMapLoader(stream)->loadMapHeader();
	getMapPatcher(name)->patchMapHeader(header);
	return header;
}

std::unique_ptr<CMap> CMapService::loadMap(const ui8 * buffer, int size, const std::string & name)
{
	auto stream = getStreamFromMem(buffer, size);
	std::unique_ptr<CMap> map(getMapLoader(stream)->loadMap());
	std::unique_ptr<CMapHeader> header(map.get());

	getMapPatcher(name)->patchMapHeader(header);
	header.release();

	return map;
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const ui8 * buffer, int size, const std::string & name)
{
	auto stream = getStreamFromMem(buffer, size);
	std::unique_ptr<CMapHeader> header = getMapLoader(stream)->loadMapHeader();
	getMapPatcher(name)->patchMapHeader(header);
	return header;
}

std::unique_ptr<CInputStream> CMapService::getStreamFromFS(const std::string & name)
{
	return CResourceHandler::get()->load(ResourceID(name, EResType::MAP));
}

std::unique_ptr<CInputStream> CMapService::getStreamFromMem(const ui8 * buffer, int size)
{
	return std::unique_ptr<CInputStream>(new CMemoryStream(buffer, size));
}

std::unique_ptr<IMapLoader> CMapService::getMapLoader(std::unique_ptr<CInputStream> & stream)
{
	// Read map header
	CBinaryReader reader(stream.get());
	ui32 header = reader.readUInt32();
	reader.getStream()->seek(0);

	// Check which map format is used
	// gzip header is 3 bytes only in size
	switch(header & 0xffffff)
	{
		// gzip header magic number, reversed for LE
		case 0x00088B1F:
			stream = std::unique_ptr<CInputStream>(new CCompressedStream(std::move(stream), true));
			return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(stream.get()));
		case EMapFormat::WOG :
		case EMapFormat::AB  :
		case EMapFormat::ROE :
		case EMapFormat::SOD :
			return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(stream.get()));
		default :
			throw std::runtime_error("Unknown map format");
	}
}

static JsonNode loadPatches(std::string path)
{
	JsonNode node = JsonUtils::assembleFromFiles(path);
	for (auto & entry : node.Struct())
		JsonUtils::validate(entry.second, "vcmi:mapHeader", "patch for " + entry.first);
	return node;
}

std::unique_ptr<IMapPatcher> CMapService::getMapPatcher(std::string scenarioName)
{
	static JsonNode node;

	if (node.isNull())
		node = loadPatches("config/mapOverrides.json");

	boost::to_lower(scenarioName);
	logGlobal->debugStream() << "Request to patch map " << scenarioName;
	return std::unique_ptr<IMapPatcher>(new CMapLoaderJson(node[scenarioName]));
}
