/*
 * mock_MapService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/mapping/CMapService.h"

class CZipSaver;
class JsonNode;

//NOTE: googlemock is useless in case of non-copyable values, f.e. std::unique_ptr

class MapListener
{
public:
	virtual void mapLoaded(CMap * map) = 0;
};

class MapServiceMock : public IMapService
{
public:
	MapServiceMock(const std::string & path, MapListener * mapListener_);

	std::unique_ptr<CMap> loadMap(const ResourcePath & name) const override;
	std::unique_ptr<CMapHeader> loadMapHeader(const ResourcePath & name) const override;
	std::unique_ptr<CMap> loadMap(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const override;
	std::unique_ptr<CMapHeader> loadMapHeader(const ui8 * buffer, int size, const std::string & name, const std::string & modName, const std::string & encoding) const override;

	void saveMap(const std::unique_ptr<CMap> & map, boost::filesystem::path fullPath) const override;

	MapListener * mapListener;
private:
	mutable CMemoryBuffer initialBuffer;

	std::unique_ptr<CMap> loadMap() const;

	void addToArchive(CZipSaver & saver, const JsonNode & data, const std::string & filename);
};


