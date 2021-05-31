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

namespace vstd
{

typedef std::function<int64_t()> TRandI64;
typedef std::function<double()> TRand;
typedef std::function<int()> TRandI;
typedef std::uniform_int_distribution<int> TIntDist;
typedef std::uniform_int_distribution<int64_t> TInt64Dist;
typedef std::uniform_real_distribution<double> TRealDist;

class DLL_LINKAGE RNG
{
public:

	virtual ~RNG() = default;

	virtual TRandI getIntRange(int lower, int upper) = 0;

	virtual TRandI64 getInt64Range(int64_t lower, int64_t upper) = 0;

	virtual TRand getDoubleRange(double lower, double upper) = 0;

	virtual int nextInt(int upper) = 0;

	/// requires: lower <= upper
	virtual int nextInt(int lower, int upper) = 0;

	/// Generates an integer between 0 and the maximum value it can hold.
	virtual int nextInt() = 0;

	/// Generates a double between 0 and upper.
	/// requires: 0 <= upper
	virtual double nextDouble(double upper) = 0;

	/// requires: lower <= upper
	virtual double nextDouble(double lower, double upper) = 0;

	/// Generates a double between 0.0 and 1.0.
	virtual double nextDouble() = 0;
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
