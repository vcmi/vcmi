
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

#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/rmg/CMapGenerator.h"

static const int TEST_RANDOM_SEED = 1337;

static CMap * initialMap;

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
		
		initialMap = gen.generate(&opt, TEST_RANDOM_SEED).release();
	};
	~CMapTestFixture()
	{
		delete initialMap;
	};
};

BOOST_GLOBAL_FIXTURE(CMapTestFixture);

BOOST_AUTO_TEST_CASE(CMapFormatVCMI_Simple)
{
	try
	{

		//TODO: serialize map
		//TODO: deserialize map
		//TODO: compare results
		
	}
	catch(const std::exception & e)
	{
		logGlobal-> errorStream() << e.what();
		throw;
	}
}
