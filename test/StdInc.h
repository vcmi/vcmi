#pragma once

#include "../Global.h"

#if !defined(_MSC_VER) && !defined(__MINGW32__)
	#define BOOST_TEST_DYN_LINK
#endif

#ifdef _MSC_VER
	#define BOOST_TEST_MODULE VcmiTest
	#include <boost/test/unit_test.hpp>
	#include "CVcmiTestConfig.h"
	BOOST_GLOBAL_FIXTURE(CVcmiTestConfig);
#endif


