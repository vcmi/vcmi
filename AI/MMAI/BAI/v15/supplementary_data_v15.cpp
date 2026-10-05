/*
 * supplementary_data.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/supplementary_data_v15.h"

#include "schema/v15/types.h"

namespace MMAI::BAI::V15
{

Schema::V15::AttackLogs SupplementaryData::getAttackLogs() const
{
	auto res = Schema::V15::AttackLogs{};
	res.reserve(attackLogs.size());

	for(const auto & al : attackLogs)
		res.push_back(&al);

	return res;
}

}
