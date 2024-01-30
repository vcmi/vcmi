/*
 * PenroseTiling.h, part of VCMI engine
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
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


VCMI_LIB_NAMESPACE_BEGIN

typedef std::array<uint32_t, 3> TIndices;

class Triangle
{
public:
	const bool tiling;
	TIndices indices;

	std::vector<Triangle *> subTriangles;

	Triangle(bool t_123, const TIndices & inds);
};

class PenroseTiling
{

public:
	const float PHI = 1.0 / ((1.0 + std::sqrt(5.0)) / 2);
	// TODO: Is that the number of symmetries?
	const uint32_t POLY = 10;

	const float scale = 4.0f;
	//const uint32_t window_w = 1920 * scale;
	//const uint32_t window_h = 1080 * scale;
	const uint32_t DEPTH = 10;      //recursion depth
	
	const bool P2 = false; // Tiling type
	//const float line_w = 2.0f;      //line width

	void generatePenroseTiling(size_t numZones, CRandomGenerator * rand);

private:
	void split(Triangle& p, std::vector<glm::vec2>& points, std::array<std::vector<uint32_t>, 5>& indices, uint32_t depth); 

};

VCMI_LIB_NAMESPACE_END