
/*
 * CRandomGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

typedef std::mt19937 TGenerator;
typedef std::uniform_int_distribution<int> TIntDist;
typedef std::uniform_real_distribution<double> TRealDist;
typedef std::function<int()> TRandI;
typedef std::function<double()> TRand;

/// The random generator randomly generates integers and real numbers("doubles") between
/// a given range. This is a header only class and mainly a wrapper for
/// convenient usage of the standard random API.
class CRandomGenerator : boost::noncopyable
{
public:
	/// Seeds the generator with the current time by default.
	CRandomGenerator() 
	{
		rand.seed(static_cast<unsigned long>(std::time(nullptr)));
	}

	void setSeed(int seed)
	{
		rand.seed(seed);
	}

	/// Generate several integer numbers within the same range.
	/// e.g.: auto a = gen.getIntRange(0,10); a(); a(); a();
	/// requires: lower <= upper
	TRandI getIntRange(int lower, int upper)
	{
		return boost::bind(TIntDist(lower, upper), boost::ref(rand));
	}
	
	/// Generates an integer between 0 and upper.
	/// requires: 0 <= upper
	int nextInt(int upper)
	{
		return getIntRange(0, upper)();
	}

	/// requires: lower <= upper
	int nextInt(int lower, int upper)
	{
		return getIntRange(lower, upper)();
	}
	
	/// Generates an integer between 0 and the maximum value it can hold.
	int nextInt()
	{
		return TIntDist()(rand);
	}

	/// Generate several double/real numbers within the same range.
	/// e.g.: auto a = gen.getDoubleRange(4.5,10.2); a(); a(); a();
	/// requires: lower <= upper
	TRand getDoubleRange(double lower, double upper)
	{
		return boost::bind(TRealDist(lower, upper), boost::ref(rand));
	}
	
	/// Generates a double between 0 and upper.
	/// requires: 0 <= upper
	double nextDouble(double upper)
	{
		return getDoubleRange(0, upper)();
	}

	/// requires: lower <= upper
	double nextDouble(double lower, double upper)
	{
		return getDoubleRange(lower, upper)();
	}

	/// Generates a double between 0.0 and 1.0.
	double nextDouble()
	{
		return TRealDist()(rand);
	}

private:
	TGenerator rand;
};

namespace RandomGeneratorUtil
{
	/// Gets an iterator to an element of a nonempty container randomly. Undefined behaviour if container is empty.
	template<typename Container>
	auto nextItem(const Container & container, CRandomGenerator & rand) -> decltype(std::begin(container))
	{
		assert(!container.empty());
		return std::next(container.begin(), rand.nextInt(container.size() - 1));
	}

	template<typename Container>
	auto nextItem(Container & container, CRandomGenerator & rand) -> decltype(std::begin(container))
	{
		assert(!container.empty());
		return std::next(container.begin(), rand.nextInt(container.size() - 1));
	}
}
