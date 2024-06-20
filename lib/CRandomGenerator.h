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

#include <vstd/RNG.h>
#include "serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN


/// Generator to use for all randomization in game
/// minstd_rand is selected due to following reasons:
/// 1. Its randomization quality is below mt_19937 however this is unlikely to be noticeable in game
/// 2. It has very low state size, leading to low overhead in size of saved games (due to large number of random generator instances in game)
using TGenerator = std::minstd_rand;
using TIntDist = std::uniform_int_distribution<int>;
using TInt64Dist = std::uniform_int_distribution<int64_t>;
using TRealDist = std::uniform_real_distribution<double>;

/// The random generator randomly generates integers and real numbers("doubles") between
/// a given range. This is a header only class and mainly a wrapper for
/// convenient usage of the standard random API. An instance of this RNG is not thread safe.
class DLL_LINKAGE CRandomGenerator final : public vstd::RNG, boost::noncopyable, public Serializeable
{
public:
	/// Seeds the generator by default with the product of the current time in milliseconds and the
	/// current thread ID.
	CRandomGenerator();

	/// Seeds the generator with provided initial seed
	explicit CRandomGenerator(int seed);

	void setSeed(int seed);

	/// Resets the seed to the product of the current time in milliseconds and the
	/// current thread ID.
	void resetSeed();

	/// Generates an integer between 0 and upper.
	/// requires: 0 <= upper
	int nextInt(int upper) override;
	int64_t nextInt64(int64_t upper) override;

	/// requires: lower <= upper
	int nextInt(int lower, int upper) override;
	int64_t nextInt64(int64_t lower, int64_t upper) override;

	/// Generates an integer between 0 and the maximum value it can hold.
	int nextInt() override;

	///
	int nextBinomialInt(int coinsCount, double coinChance) override;


	/// Generates a double between 0 and upper.
	/// requires: 0 <= upper
	double nextDouble(double upper) override;

	/// requires: lower <= upper
	double nextDouble(double lower, double upper) override;

	/// Gets a globally accessible RNG which will be constructed once per thread. For the
	/// seed a combination of the thread ID and current time in milliseconds will be used.
	static CRandomGenerator & getDefault();

private:
	TGenerator rand;

public:
	template <typename Handler>
	void serialize(Handler & h)
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


VCMI_LIB_NAMESPACE_END
