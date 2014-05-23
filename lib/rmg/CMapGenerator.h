
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

class rmgException : std::exception
{
	std::string msg;
public:
	explicit rmgException(const std::string& _Message) : msg(_Message)
	{
	}

	virtual ~rmgException() throw ()
	{
	};

	const char *what() const throw () override
	{
		return msg.c_str();
	}
};

/// The map generator creates a map randomly.
class DLL_LINKAGE CMapGenerator
{
public:
	explicit CMapGenerator(shared_ptr<CMapGenOptions> mapGenOptions, int randomSeed = std::time(nullptr));
	~CMapGenerator(); // required due to unique_ptr

	std::unique_ptr<CMap> generate();
	
	shared_ptr<CMapGenOptions> mapGenOptions;
	std::unique_ptr<CMap> map;
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

