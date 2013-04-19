
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

#include <boost/version.hpp>
#include <boost/random/mersenne_twister.hpp>

#if BOOST_VERSION >= 104700
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#else
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#endif
#include <boost/random/variate_generator.hpp>

typedef boost::mt19937 TGenerator;
#if BOOST_VERSION >= 104700
typedef boost::random::uniform_int_distribution<int> TIntDist;
typedef boost::random::uniform_real_distribution<double> TRealDist;
#else
typedef boost::uniform_int<int> TIntDist;
typedef boost::uniform_real<double> TRealDist;
#endif
typedef boost::variate_generator<TGenerator &, TIntDist> TRandI;
typedef boost::variate_generator<TGenerator &, TRealDist> TRand;

/// The random generator randomly generates integers and real numbers("doubles") between
/// a given range. This is a header only class and mainly a wrapper for
/// convenient usage of the boost random API.
class CRandomGenerator
{
public:
	/// Seeds the generator with the current time by default.
	CRandomGenerator() 
	{
		gen.seed(std::time(nullptr));
	}

	void seed(int value)
	{
		gen.seed(value);
	}

	/// Generate several integer numbers within the same range.
	/// e.g.: auto a = gen.getRangeI(0,10); a(); a(); a();
	TRandI getRangeI(int lower, int upper)
	{
		TIntDist range(lower, upper);
		return TRandI(gen, range);
	}
	
	int getInteger(int lower, int upper)
	{
		return getRangeI(lower, upper)();
	}
	
	/// Generate several double/real numbers within the same range.
	/// e.g.: auto a = gen.getRangeI(0,10); a(); a(); a();
	TRand getRange(double lower, double upper)
	{
		TRealDist range(lower, upper);
		return TRand(gen, range);
	}
	
	double getDouble(double lower, double upper)
	{
		return getRange(lower, upper)();
	}

private:
	TGenerator gen;
};
