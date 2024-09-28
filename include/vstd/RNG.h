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

class DLL_LINKAGE RNG
{
public:
	virtual ~RNG() = default;

	/// Returns random number in range [lower, upper]
	virtual int nextInt(int lower, int upper) = 0;

	/// Returns random number in range [lower, upper]
	virtual int64_t nextInt64(int64_t lower, int64_t upper) = 0;

	/// Returns random number in range [lower, upper]
	virtual double nextDouble(double lower, double upper) = 0;

	/// Returns random number in range [0, upper]
	virtual int nextInt(int upper) = 0;

	/// Returns random number in range [0, upper]
	virtual int64_t nextInt64(int64_t upper) = 0;

	/// Returns random number in range [0, upper]
	virtual double nextDouble(double upper) = 0;

	/// Generates an integer between 0 and the maximum value it can hold.
	/// Should be only used for seeding other generators
	virtual int nextInt() = 0;

	/// Returns integer using binomial distribution
	/// returned value is number of successfull coin flips with chance 'coinChance' out of 'coinsCount' attempts
	virtual int nextBinomialInt(int coinsCount, double coinChance) = 0;
};

}

namespace RandomGeneratorUtil
{
	template<typename Container>
	auto nextItem(const Container & container, vstd::RNG & rand) -> decltype(std::begin(container))
	{
		if(container.empty())
			throw std::runtime_error("Unable to select random item from empty container!");

		return std::next(container.begin(), rand.nextInt64(0, container.size() - 1));
	}

	template<typename Container>
	auto nextItem(Container & container, vstd::RNG & rand) -> decltype(std::begin(container))
	{
		if(container.empty())
			throw std::runtime_error("Unable to select random item from empty container!");

		return std::next(container.begin(), rand.nextInt64(0, container.size() - 1));
	}

	template<typename Container>
	size_t nextItemWeighted(Container & container, vstd::RNG & rand)
	{
		assert(!container.empty());

		int64_t totalWeight = std::accumulate(container.begin(), container.end(), 0);
		assert(totalWeight > 0);

		int64_t roll = rand.nextInt64(0, totalWeight - 1);

		for (size_t i = 0; i < container.size(); ++i)
		{
			roll -= container[i];
			if(roll < 0)
				return i;
		}
		return container.size() - 1;
	}

	template<typename Container>
	void randomShuffle(Container & container, vstd::RNG & rand)
	{
		int64_t n = std::distance(container.begin(), container.end());

		for(int64_t i = n - 1; i > 0; --i)
		{
			auto randIndex = rand.nextInt64(0, i);
			std::swap(*(container.begin() + i), *(container.begin() + randIndex));
		}
	}
}

VCMI_LIB_NAMESPACE_END
