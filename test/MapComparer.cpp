/*
 * MapComparer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <boost/test/unit_test.hpp>

#include "MapComparer.h"

bool MapComparer::operator() (const std::unique_ptr<CMap>& lhs, const std::unique_ptr<CMap>&  rhs)
{
	BOOST_ERROR(" MapComparer::operator() not implemented");
	return false;
}
