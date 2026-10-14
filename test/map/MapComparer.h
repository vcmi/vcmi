/*
 * MapComparer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class CMap;
class CGObjectInstance;

struct MapComparer
{
	const CMap * actual;
	const CMap * expected;

	void compareHeader();
	void compareOptions();
	void compareObject(const CGObjectInstance * actual, const CGObjectInstance * expected);
	void compareObjects();
	void compareTerrain();

	void compare();

	void operator() (const std::unique_ptr<CMap>& actual, const std::unique_ptr<CMap>& expected);
};




