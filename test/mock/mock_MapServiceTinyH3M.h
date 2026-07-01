/*
 * mock_MapServiceTinyH3M.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/mapping/CMapService.h"
#include "mock_MapService.h"

/// IMapService backed by an in-memory `.h3m` byte stream (typically produced
/// by TinyH3MBuilder::build). Lets tests skip the disk-roundtrip and the
/// JSON-archive shim that MapServiceMock relies on.
///
/// `mapListener->mapLoaded(map)` fires after every successful load so derived
/// fixtures (e.g. QuestTest) can stash the CMap pointer.
class MapServiceTinyH3M : public IMapService
{
public:
	MapServiceTinyH3M(std::vector<uint8_t> h3mBytes, MapListener * mapListener_);

	std::unique_ptr<CMap>       loadMap(const ResourcePath & name, IGameInfoCallback * cb) const override;
	std::unique_ptr<CMapHeader> loadMapHeader(const ResourcePath & name, bool isEditor = false) const override;
	std::unique_ptr<CMap>       loadMap(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding, IGameInfoCallback * cb) const override;
	std::unique_ptr<CMapHeader> loadMapHeader(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const override;

	void saveMap(const std::unique_ptr<CMap> & map, boost::filesystem::path fullPath) const override;

	MapListener * mapListener;

private:
	std::vector<uint8_t> bytes;
};
