/*
 * CRmgPath.h, part of VCMI engine
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
#include "CRmgArea.h"

namespace Rmg
{
class Path
{
public:
	const static std::function<float(const int3 &, const int3 &)> DEFAULT_MOVEMENT_FUNCTION;
	
	Path(const Area & area);
	Path(const Area & area, const int3 & src);
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
	
private:
	
	const Area dArea;
	Area dPath;
};
}
