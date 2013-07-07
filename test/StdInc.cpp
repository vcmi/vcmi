// Creates the precompiled header
#include "StdInc.h"

#ifndef _MSC_VER
	// Should be defined only once, before #include of unit test header
	#define BOOST_TEST_MODULE VcmiTest
	#include <boost/test/unit_test.hpp>
	#include "CVcmiTestConfig.h"
	BOOST_GLOBAL_FIXTURE(CVcmiTestConfig);
#endif
