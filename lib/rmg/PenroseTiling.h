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
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace vstd
{
class RNG;
}

using namespace boost::geometry;
typedef std::array<uint32_t, 3> TIndices;

const float PI_CONSTANT = 3.141592f;

class Point2D : public model::d2::point_xy<float>
{
public:
	using point_xy::point_xy;

	Point2D operator * (float scale) const;
	Point2D operator / (float scale) const;
	Point2D operator + (const Point2D& other) const;
	Point2D operator - (const Point2D& other) const;
	Point2D rotated(float radians) const;

	bool operator < (const Point2D& other) const;
	bool operator == (const Point2D& other) const;

	std::string toString() const;
};

Point2D rotatePoint(const Point2D& point, double radians, const Point2D& origin);

class Triangle
{
public:
	~Triangle();

	const bool tiling;
	TIndices indices;

	std::vector<Triangle *> subTriangles;

	Triangle(bool t_123, const TIndices & inds);
};

class PenroseTiling
{

public:
	static constexpr float MIN_TILE_SCALE = 0.25f;
	static constexpr float MAX_TILE_SCALE = 2.0f;

	static constexpr float PHI = 1.0f / ((1.0f + 2.236067977f) / 2.0f); // 1 / ((1 + sqrt(5)) / 2)
	static constexpr uint32_t POLY = 10; // Number of symmetries?

	static constexpr float BASE_SIZE = 1.25f;
	static constexpr uint32_t DEPTH = 8; // Baseline subdivision exponent for L0
	static constexpr bool P2 = false; // Tiling type

	struct Geometry
	{
		float edge = 0.f;     // long side L of the smallest triangle, normalized units
		float radius = 0.f;   // decagon radius R
		uint32_t depth = 0;   // subdivision depth
	};

	static Geometry geometry(size_t numZones, float tileScale, int mapSize);
	std::set<Point2D> generatePenroseTiling(size_t numZones, float tileScale, int mapSize, vstd::RNG * rand);

private:
	void split(Triangle& p, std::vector<Point2D>& points, std::array<std::vector<uint32_t>, 5>& indices, uint32_t depth); 

};
