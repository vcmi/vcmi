/*
 * ContainerUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
	template<typename K, typename V>
	std::map<V, K> invertMap(const std::map<K, V> & m)
	{
		std::map<V,K> other;
		std::transform(m.cbegin(), m.cend(), std::inserter(other, other.begin()), [](const std::pair<K, V> & p)
		{
			return std::make_pair(p.second, p.first);
		});
		return other;
	}
}

VCMI_LIB_NAMESPACE_END
