
/*
 * CMapGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../CRandomGenerator.h"
#include "CMapGenOptions.h"
#include "../CObjectHandler.h"
#include "../int3.h"

class CMap;
class CRmgTemplate;
class CRmgTemplateZone;
class CMapGenOptions;
class CTerrainViewPatternConfig;
class CMapEditManager;
class JsonNode;

typedef std::vector<JsonNode> JsonVector;

class CMapGenerator;

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator
{
public:
	explicit CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed = std::time(nullptr));
	~CMapGenerator(); // required due to unique_ptr

	ConstTransitivePtr<CMap> generate();
	
	CMapGenOptions mapGenOptions;
	ConstTransitivePtr<CMap> map;
	CRandomGenerator rand;
	int randomSeed;
	CMapEditManager * editManager;

private:
	std::map<TRmgTemplateZoneId, CRmgTemplateZone*> zones;

	/// Generation methods
	std::string getMapDescription() const;
	void addPlayerInfo();
	void addHeaderInfo();
	void genZones();
	void fillZones();

};

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

