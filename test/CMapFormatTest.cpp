
/*
 * CMapFormatTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <boost/test/unit_test.hpp>

#include "../lib/filesystem/CMemoryBuffer.h"

#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/mapping/MapFormatJSON.h"

#include "MapComparer.h"

static const int TEST_RANDOM_SEED = 1337;

static std::unique_ptr<CMap> initialMap;

class CMapTestFixture
{
public:	
	CMapTestFixture()
	{
		CMapGenOptions opt;
		
		opt.setHeight(72);
		opt.setWidth(72);
		opt.setHasTwoLevels(false);
		opt.setPlayerCount(2);
		
		CMapGenerator gen;
		
		initialMap = gen.generate(&opt, TEST_RANDOM_SEED);
	};
	~CMapTestFixture()
	{
		initialMap.reset();
	};
};

BOOST_GLOBAL_FIXTURE(CMapTestFixture);

BOOST_AUTO_TEST_CASE(CMapFormatVCMI_Simple)
{
	try
	{
		CMemoryBuffer serializeBuffer;
		CMapSaverJson saver(&serializeBuffer);
		saver.saveMap(initialMap);
		
		CMapLoaderJson loader(&serializeBuffer);
		serializeBuffer.seek(0);		
		std::unique_ptr<CMap> serialized = loader.loadMap();
		
		
		MapComparer c;
		
		BOOST_REQUIRE_MESSAGE(c(initialMap, serialized), "Serialize cycle failed");
	}
	catch(const std::exception & e)
	{
		logGlobal-> errorStream() << e.what();
		throw;
	}
}
