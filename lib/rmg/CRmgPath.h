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
	Path(const Area & area);
	Path(const Area & area, const int3 & src);
	bool valid() const;
	
	Path search(const Tileset & dst, bool straight) const;
	Path search(const int3 & dst, bool straight) const;
	Path search(const Area & dst, bool straight) const;
	Path search(const Path & dst, bool straight) const;
	
	bool connect(const Path & path);
	bool connect(const int3 & path); //TODO: force connection?
	bool connect(const Tileset & path); //TODO: force connection?
	
	const Area & getPathArea() const;
	
private:
	
	const Area & dArea;
	Area dPath;
};
}
