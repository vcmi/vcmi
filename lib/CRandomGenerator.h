
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

/**
 * The random generator randomly generates integers and real numbers("doubles") between
 * a given range. This is a header only class and mainly a wrapper for
 * convenient usage of the boost random API.
 */
class CRandomGenerator
{
public:
	/**
	 * Constructor. Seeds the generator with the current time by default.
	 */
	CRandomGenerator() 
	{
		gen.seed(std::time(nullptr)); 
	}

	/**
	 * Seeds the generator with the given value.
	 *
	 * @param value the random seed
	 */
	void seed(int value)
	{
		gen.seed(value);
	}

	/**
	 * Gets a generator which generates integers in the given range.
	 *
	 * Example how to use:
	 * @code
	 * TRandI rand = getRangeI(0, 10);
	 * int a = rand(); // with the operator() the next value can be obtained
	 * int b = rand(); // you can generate more values
	 * @endcode
	 *
	 * @param lower the lower boundary
	 * @param upper the upper boundary
	 * @return the generator which can be used to generate integer numbers
	 */
	TRandI getRangeI(int lower, int upper)
	{
		TIntDist range(lower, upper);
		return TRandI(gen, range);
	}
	
	/**
	 * Gets a integer in the given range. In comparison to getRangeI it's
	 * a convenient method if you want to generate only one value in a given
	 * range.
	 *
	 * @param lower the lower boundary
	 * @param upper the upper boundary
	 * @return the generated integer
	 */
	int getInteger(int lower, int upper)
	{
		return getRangeI(lower, upper)();
	}
	
	/**
	 * Gets a generator which generates doubles in the given range.
	 *
	 * Example how to use:
	 * @code
	 * TRand rand = getRange(23.56, 32.10);
	 * double a = rand(); // with the operator() the next value can be obtained
	 * double b = rand(); // you can generate more values
	 * @endcode
	 *
	 * @param lower the lower boundary
	 * @param upper the upper boundary
	 * @return the generated double
	 */
	TRand getRange(double lower, double upper)
	{
		TRealDist range(lower, upper);
		return TRand(gen, range);
	}
	
	/**
	 * Gets a double in the given range. In comparison to getRange it's
	 * a convenient method if you want to generate only one value in a given
	 * range.
	 *
	 * @param lower the lower boundary
	 * @param upper the upper boundary
	 * @return the generated double
	 */
	double getDouble(double lower, double upper)
	{
		return getRange(lower, upper)();
	}

private:
	/** The actual boost random generator. */
	TGenerator gen;
};
