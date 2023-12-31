/*
 * RNG.h, part of VCMI engine
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

using TRandI64 = std::function<int64_t()>;
using TRand = std::function<double()>;

class DLL_LINKAGE RNG
{
public:

	virtual ~RNG() = default;

	virtual TRandI64 getInt64Range(int64_t lower, int64_t upper) = 0;

	virtual TRand getDoubleRange(double lower, double upper) = 0;
};

}

namespace RandomGeneratorUtil
{
	template<typename Container>
	auto nextItem(const Container & container, vstd::RNG & rand) -> decltype(std::begin(container))
	{
		assert(!container.empty());
		return std::next(container.begin(), rand.getInt64Range(0, container.size() - 1)());
	}

	template<typename Container>
	auto nextItem(Container & container, vstd::RNG & rand) -> decltype(std::begin(container))
	{
		assert(!container.empty());
		return std::next(container.begin(), rand.getInt64Range(0, container.size() - 1)());
	}

	template<typename T>
	void randomShuffle(std::vector<T> & container, vstd::RNG & rand)
	{
		int64_t n = (container.end() - container.begin());

		for(int64_t i = n-1; i>0; --i)
		{
			std::swap(container.begin()[i],container.begin()[rand.getInt64Range(0, i)()]);
		}
	}
}

VCMI_LIB_NAMESPACE_END
