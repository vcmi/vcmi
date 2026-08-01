/*
 * enum_flags.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "AI/MMAI/common.h"

namespace MMAI::BAI::V15
{

template<typename T>
struct EnumFlags
{
	std::bitset<EU(T::_count)> flags;

	void set(T v)
	{
		assert(EU(v) < flags.size());
		assert(!flags.test(EU(v)));
		flags.set(EU(v));
	};

	bool isSet(T v)
	{
		assert(EU(v) < flags.size());
		return flags.test(EU(v));
	};

	void require(T v)
	{
		if(!isSet(v))
			throw std::runtime_error("Required flag is not set: " + std::to_string(EU(v)));
	}

	void reject(T v)
	{
		if(isSet(v))
			throw std::runtime_error("Rejected flag is set: " + std::to_string(EU(v)));
	}

	void requireExclusive(T v)
	{
		for(int i = 0; i < EU(T::_count); ++i)
			EU(v) == i ? require(static_cast<T>(i)) : reject(static_cast<T>(i));
	}

	void requireExclusive(std::initializer_list<T> values)
	{
		for(int i = 0; i < EU(T::_count); ++i)
			std::find(values.begin(), values.end(), static_cast<T>(i)) == values.end() ? reject(static_cast<T>(i)) : require(static_cast<T>(i));
	}
};

}
