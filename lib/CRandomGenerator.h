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

VCMI_LIB_NAMESPACE_BEGIN

using TGenerator = std::mt19937;
using TIntDist = std::uniform_int_distribution<int>;
using TInt64Dist = std::uniform_int_distribution<int64_t>;
using TRealDist = std::uniform_real_distribution<double>;
using TRandI = std::function<int()>;

/// The random generator randomly generates integers and real numbers("doubles") between
/// a given range. This is a header only class and mainly a wrapper for
/// convenient usage of the standard random API. An instance of this RNG is not thread safe.
class DLL_LINKAGE CRandomGenerator : public vstd::RNG, boost::noncopyable
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

	/// Generate several integer numbers within the same range.
	/// e.g.: auto a = gen.getIntRange(0,10); a(); a(); a();
	/// requires: lower <= upper
	TRandI getIntRange(int lower, int upper);

	vstd::TRandI64 getInt64Range(int64_t lower, int64_t upper) override;

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
	vstd::TRand getDoubleRange(double lower, double upper) override;

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

public:
	template <typename Handler>
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


VCMI_LIB_NAMESPACE_END
