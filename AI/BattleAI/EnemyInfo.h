/*
 * EnemyInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
	class Unit;
}

VCMI_LIB_NAMESPACE_END

class EnemyInfo
{
public:
	const battle::Unit * s;
	EnemyInfo(const battle::Unit * _s) : s(_s)
	{}
	bool operator==(const EnemyInfo & ei) const;
};
