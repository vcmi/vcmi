/*
 * encoding.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include <cstdint>

namespace MMAI::Schema::V15
{
enum class Encoding : uint8_t
{
	/*
	 * Represent `v` as `n` bits, where `bits[v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => (error)
	 */
	CATEGORICAL,

	/*
	 * Normalize `v` linearly in the range `(-vmax, vmax)`.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.3`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => `-0.1`
	 */
	LINNORM,

	/*
	 * Don't normalize, use as-is.
	 */
	RAW,
};
}
