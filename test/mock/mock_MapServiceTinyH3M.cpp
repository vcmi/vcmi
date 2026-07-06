/*
 * mock_MapServiceTinyH3M.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "mock_MapServiceTinyH3M.h"

#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatH3M.h"

namespace
{
// modName "core" is the same one TinyH3MBuilderTest uses — readLocalizedString
// dispatches through mapRegisterLocalizedString and needs a resolvable mod.
constexpr const char * kModName  = "core";
constexpr const char * kEncoding = "ASCII";
constexpr const char * kMapName  = "TinyH3MFixture";
}

MapServiceTinyH3M::MapServiceTinyH3M(std::vector<uint8_t> h3mBytes, MapListener * mapListener_)
	: mapListener(mapListener_)
	, bytes(std::move(h3mBytes))
{
}

std::unique_ptr<CMap> MapServiceTinyH3M::loadMap(const ResourcePath & /*name*/, IGameInfoCallback * cb) const
{
	// CMemoryBuffer is noncopyable, so it is constructed in-place per load.
	CMemoryBuffer buf;
	buf.write(bytes.data(), static_cast<si64>(bytes.size()));
	buf.seek(0);

	CMapLoaderH3M loader(kMapName, kModName, kEncoding, &buf);
	auto map = loader.loadMap(cb);
	if(mapListener)
		mapListener->mapLoaded(map.get());
	return map;
}

std::unique_ptr<CMapHeader> MapServiceTinyH3M::loadMapHeader(const ResourcePath & /*name*/, bool /*isEditor*/) const
{
	CMemoryBuffer buf;
	buf.write(bytes.data(), static_cast<si64>(bytes.size()));
	buf.seek(0);

	CMapLoaderH3M loader(kMapName, kModName, kEncoding, &buf);
	return loader.loadMapHeader();
}

std::unique_ptr<CMap> MapServiceTinyH3M::loadMap(const ui8 * /*buffer*/, int /*size*/, const std::string & /*name*/, const std::string & /*modName*/, const std::string & /*encoding*/, IGameInfoCallback * cb) const
{
	return loadMap(ResourcePath(""), cb);
}

std::unique_ptr<CMapHeader> MapServiceTinyH3M::loadMapHeader(const ui8 * /*buffer*/, int /*size*/, const std::string & /*name*/, const std::string & /*modName*/, const std::string & /*encoding*/) const
{
	return loadMapHeader(ResourcePath(""), false);
}

void MapServiceTinyH3M::saveMap(const std::unique_ptr<CMap> & /*map*/, boost::filesystem::path /*fullPath*/) const
{
	FAIL() << "Unexpected call to MapServiceTinyH3M::saveMap";
}
