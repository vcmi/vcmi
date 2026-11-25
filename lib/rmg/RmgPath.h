/*
 * RmgPath.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../int3.h"
#include "RmgArea.h"

namespace rmg
{

class Path
{
public:
	using MoveCostFunction = std::function<float(const int3 &, const int3 &)>;
	using PassableFunction = std::function<bool(const int3 &)>;
	const static MoveCostFunction DEFAULT_MOVEMENT_FUNCTION;
	
	Path(const Area & area);
	Path(const Area & area, const int3 & src);
	/// VCM-169: search over tiles accepted by `passable`, ending at any tile of `target` or connected by this Path.
	Path(PassableFunction passable, const Area & target);
	Path(PassableFunction passable, Area && target) = delete; //target is kept by reference
	Path(const Path & path) = default;
	Path & operator= (const Path & path);
	bool valid() const;
	
	Path search(const Tileset & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction = DEFAULT_MOVEMENT_FUNCTION) const;
	Path search(const int3 & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction = DEFAULT_MOVEMENT_FUNCTION) const;
	Path search(const Area & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction = DEFAULT_MOVEMENT_FUNCTION) const;
	Path search(const Path & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction = DEFAULT_MOVEMENT_FUNCTION) const;
	
	void connect(const Path & path);
	void connect(const int3 & path); //TODO: force connection?
	void connect(const Area & path); //TODO: force connection?
	void connect(const Tileset & path); //TODO: force connection?
	
	const Area & getPathArea() const;
	
	static Path invalid();
	static float nonEuclideanCostFunction(const int3& src, const int3& dst, const int3& center);
	
private:
	Path() = default;
	bool isTarget(const int3 & tile) const;
	int3 nearestTarget(const int3 & tile) const;
	
	PassableFunction dPassable; //empty: this Path cannot search
	const Area * dTarget = nullptr; //tiles ending a search besides dPath, owned by the caller
	Area dPath;
};

struct BezierCurve
{
	int3 p0, p1, p2, p3; // p0: road node, p1/p2: control points, p3: zone.getPos()
	float distanceSq(const int3 & tile) const;
	static float borderPenalty(const DistanceMap & borderDistance, const int3 & tile);
	Path::MoveCostFunction costFunction() const;
	Path::MoveCostFunction costFunction(const DistanceMap & borderDistance) const;
};
}
