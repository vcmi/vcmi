/*
 * CMapService.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapService.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CBinaryReader.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "../filesystem/CMemoryBuffer.h"
#include "../CModHandler.h"
#include "../Languages.h"

#include "CMap.h"
#include "MapFormat.h"

#include "MapFormatH3M.h"
#include "MapFormatJson.h"

VCMI_LIB_NAMESPACE_BEGIN


std::unique_ptr<CMap> CMapService::loadMap(const ResourceID & name) const
{
	std::string modName = VLC->modh->findResourceOrigin(name);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;

	auto stream = getStreamFromFS(name);
	return getMapLoader(stream, name.getName(), modName, encoding)->loadMap();
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const ResourceID & name) const
{
	std::string modName = VLC->modh->findResourceOrigin(name);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;

	auto stream = getStreamFromFS(name);
	return getMapLoader(stream, name.getName(), modName, encoding)->loadMapHeader();
}

std::unique_ptr<CMap> CMapService::loadMap(const ui8 * buffer, int size, const std::string & name,  const std::string & modName, const std::string & encoding) const
{
	auto stream = getStreamFromMem(buffer, size);
	std::unique_ptr<CMap> map(getMapLoader(stream, name, modName, encoding)->loadMap());
	std::unique_ptr<CMapHeader> header(map.get());

	//might be original campaign and require patch
	getMapPatcher(name)->patchMapHeader(header);
	header.release();

	return map;
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const
{
	auto stream = getStreamFromMem(buffer, size);
	std::unique_ptr<CMapHeader> header = getMapLoader(stream, name, modName, encoding)->loadMapHeader();

	//might be original campaign and require patch
	getMapPatcher(name)->patchMapHeader(header);
	return header;
}

void CMapService::saveMap(const std::unique_ptr<CMap> & map, boost::filesystem::path fullPath) const
{
	CMemoryBuffer serializeBuffer;
	{
		CMapSaverJson saver(&serializeBuffer);
		saver.saveMap(map);
	}
	{
		boost::filesystem::remove(fullPath);
		boost::filesystem::ofstream tmp(fullPath, boost::filesystem::ofstream::binary);

		tmp.write(reinterpret_cast<const char *>(serializeBuffer.getBuffer().data()), serializeBuffer.getSize());
		tmp.flush();
		tmp.close();
	}
}

ModCompatibilityInfo CMapService::verifyMapHeaderMods(const CMapHeader & map)
{
	ModCompatibilityInfo modCompatibilityInfo;
	const auto & activeMods = VLC->modh->getActiveMods();
	for(const auto & mapMod : map.mods)
	{
		if(vstd::contains(activeMods, mapMod.first))
		{
			const auto & modInfo = VLC->modh->getModInfo(mapMod.first);
			if(modInfo.version.compatible(mapMod.second))
				continue;
		}
		
		modCompatibilityInfo[mapMod.first] = mapMod.second;
	}	
	return modCompatibilityInfo;
}

std::unique_ptr<CInputStream> CMapService::getStreamFromFS(const ResourceID & name)
{
	return CResourceHandler::get()->load(name);
}

std::unique_ptr<CInputStream> CMapService::getStreamFromMem(const ui8 * buffer, int size)
{
	return std::unique_ptr<CInputStream>(new CMemoryStream(buffer, size));
}

std::unique_ptr<IMapLoader> CMapService::getMapLoader(std::unique_ptr<CInputStream> & stream, std::string mapName, std::string modName, std::string encoding)
{
	// Read map header
	CBinaryReader reader(stream.get());
	ui32 header = reader.readUInt32();
	reader.getStream()->seek(0);

	//check for ZIP magic. Zip files are VCMI maps
	switch(header)
	{
	case 0x06054b50:
	case 0x04034b50:
	case 0x02014b50:
		return std::unique_ptr<IMapLoader>(new CMapLoaderJson(stream.get()));
		break;
	default:
		// Check which map format is used
		// gzip header is 3 bytes only in size
		switch(header & 0xffffff)
		{
			// gzip header magic number, reversed for LE
			case 0x00088B1F:
				stream = std::unique_ptr<CInputStream>(new CCompressedStream(std::move(stream), true));
				return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(mapName, modName, encoding, stream.get()));
			case static_cast<int>(EMapFormat::WOG) :
			case static_cast<int>(EMapFormat::AB)  :
			case static_cast<int>(EMapFormat::ROE) :
			case static_cast<int>(EMapFormat::SOD) :
			case static_cast<int>(EMapFormat::HOTA) :
				return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(mapName, modName, encoding, stream.get()));
			default :
				throw std::runtime_error("Unknown map format");
		}
	}
}

static JsonNode loadPatches(std::string path)
{
	JsonNode node = JsonUtils::assembleFromFiles(std::move(path));
	for (auto & entry : node.Struct())
		JsonUtils::validate(entry.second, "vcmi:mapHeader", "patch for " + entry.first);

	node.setMeta(CModHandler::scopeMap());
	return node;
}

std::unique_ptr<IMapPatcher> CMapService::getMapPatcher(std::string scenarioName)
{
	static JsonNode node;

	if (node.isNull())
		node = loadPatches("config/mapOverrides.json");

	boost::to_lower(scenarioName);
	logGlobal->debug("Request to patch map %s", scenarioName);
	return std::unique_ptr<IMapPatcher>(new CMapPatcher(node[scenarioName]));
}

VCMI_LIB_NAMESPACE_END
