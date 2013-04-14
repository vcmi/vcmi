
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
#include "CMapGenOptions.h"
#include "../CRandomGenerator.h"

class CMap;
class CTerrainViewPatternConfig;
class CMapEditManager;

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator
{
public:
	CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed);
	~CMapGenerator(); // required due to unique_ptr

	std::unique_ptr<CMap> generate();

private:
	/// Generation methods
	std::string getMapDescription() const;
	void addPlayerInfo();
	void addHeaderInfo();
	void genTerrain();
	void genTowns();

	CMapGenOptions mapGenOptions;
	std::unique_ptr<CMap> map;
	CRandomGenerator gen;
	int randomSeed;
	std::unique_ptr<CTerrainViewPatternConfig> terViewPatternConfig;
	std::unique_ptr<CMapEditManager> mapMgr;
};
