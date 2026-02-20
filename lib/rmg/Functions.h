/*
 * Functions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Zone.h"
#include <boost/heap/priority_queue.hpp> //A*

VCMI_LIB_NAMESPACE_BEGIN

class RmgMap;
class ObjectManager;
class ObjectTemplate;
class CMapGenerator;

class rmgException : public std::exception
{
	std::string msg;
public:
	explicit rmgException(const std::string& _Message) : msg(_Message)
	{
	}

	const char *what() const noexcept override
	{
		return msg.c_str();
	}
};

void replaceWithCurvedPath(rmg::Path & path, Zone & zone, const int3 & src, bool onlyStraight = true);

rmg::Tileset collectDistantTiles(const Zone & zone, int distance);

int chooseRandomAppearance(vstd::RNG & generator, si32 ObjID, TerrainId terrain);


VCMI_LIB_NAMESPACE_END
