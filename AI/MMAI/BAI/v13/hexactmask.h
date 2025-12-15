/*
 * hexactmask.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BAI/v13/hexaction.h"

namespace MMAI::BAI::V13
{
/*
  * A list of flags for a single hex (see HexAction)
  */
using HexActMask = std::bitset<EI(HexAction::_count)>;

struct ActMask
{
	bool retreat = false;
	bool wait = false;

	/*
      * A list of HexActMask objects
      *
      * [0] HexActMask for hex 0
      * [1] HexActMask for hex 1
      * ...
      * [164] HexActMask for hex 164
      */
	std::array<HexActMask, 165> hexactmasks = {};
};
}
