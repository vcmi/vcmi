/*
 * UUID.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "UUID.h"

boost::uuids::uuid UUID::getID() const
{
	return ID;
}

boost::uuids::uuid UUID::generateID()
{
	static boost::uuids::random_generator_mt19937 gen;
	return gen();
}
