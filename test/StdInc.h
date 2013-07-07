#pragma once

#include "../Global.h"

#ifndef _MSC_VER
	#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE VcmiTest
#include <boost/test/unit_test.hpp>
#include "CVcmiTestConfig.h"
BOOST_GLOBAL_FIXTURE(CVcmiTestConfig);


