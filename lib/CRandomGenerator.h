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
typedef std::function<int ()> TRandI;
typedef std::function<double ()> TRand;

/// The random generator randomly generates integers and real numbers("doubles") between
/// a given range. This is a header only class and mainly a wrapper for
/// convenient usage of the standard random API. An instance of this RNG is not thread safe.
class DLL_LINKAGE CRandomGenerator : boost::noncopyable
{
public:
	/// Seeds the generator by default with the product of the current time in milliseconds and the
	/// current thread ID.
	CRandomGenerator();

	void setSeed(int seed);

	/// Resets the seed to the product of the current time in milliseconds and the
	/// current thread ID.
	void resetSeed();

	/// Generate several integer numbers within the same range.
	/// e.g.: auto a = gen.getIntRange(0,10); a(); a(); a();
	/// requires: lower <= upper
	TRandI getIntRange(int lower, int upper);

	/// Generates an integer between 0 and upper.
	/// requires: 0 <= upper
	int nextInt(int upper);

	/// requires: lower <= upper
	int nextInt(int lower, int upper);

	/// Generates an integer between 0 and the maximum value it can hold.
	int nextInt();

	/// Generate several double/real numbers within the same range.
	/// e.g.: auto a = gen.getDoubleRange(4.5,10.2); a(); a(); a();
	/// requires: lower <= upper
	TRand getDoubleRange(double lower, double upper);

	/// Generates a double between 0 and upper.
	/// requires: 0 <= upper
	double nextDouble(double upper);

	/// requires: lower <= upper
	double nextDouble(double lower, double upper);

	/// Generates a double between 0.0 and 1.0.
	double nextDouble();

	/// Gets a globally accessible RNG which will be constructed once per thread. For the
	/// seed a combination of the thread ID and current time in milliseconds will be used.
	static CRandomGenerator & getDefault();

	/// Provide method so that this RNG can be used with legacy std:: API
	TGenerator & getStdGenerator();

private:
	TGenerator rand;
	static boost::thread_specific_ptr<CRandomGenerator> defaultRand;

public:
	template<typename Handler>
	void serialize(Handler & h, const int version)
	{
		if(h.saving)
		{
			std::ostringstream stream;
			stream << rand;
			std::string str = stream.str();
			h & str;
		}
		else
		{
			std::string str;
			h & str;
			std::istringstream stream(str);
			stream >> rand;
		}
	}
};

namespace RandomGeneratorUtil
{
template<typename Container>
auto nextItem(const Container & container, CRandomGenerator & rand)->decltype(std::begin(container))
{
	assert(!container.empty());
	return std::next(container.begin(), rand.nextInt(container.size() - 1));
}

template<typename Container>
auto nextItem(Container & container, CRandomGenerator & rand)->decltype(std::begin(container))
{
	assert(!container.empty());
	return std::next(container.begin(), rand.nextInt(container.size() - 1));
}

template<typename T>
void randomShuffle(std::vector<T> & container, CRandomGenerator & rand)
{
	int n = (container.end() - container.begin());
	for(int i = n - 1; i > 0; --i)
	{
		std::swap(container.begin()[i], container.begin()[rand.nextInt(i)]);
	}
}
}
