
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

#include "../CRandomGenerator.h"
#include "CMapGenOptions.h"

class CMap;
class CMapEditManager;

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator
{
public:
	CMapGenerator();
	~CMapGenerator(); // required due to unique_ptr

	std::unique_ptr<CMap> generate(CMapGenOptions * mapGenOptions, int randomSeed = std::time(nullptr));

private:
	/// Generation methods
	std::string getMapDescription() const;
	void addPlayerInfo();
	void addHeaderInfo();
	void genTerrain();
	void genTowns();

	CMapGenOptions * mapGenOptions;
	std::unique_ptr<CMap> map;
	CRandomGenerator gen;
	int randomSeed;
	CMapEditManager * editManager;
};
