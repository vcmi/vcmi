
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
class CRandomGenerator
{
public:
	/// Seeds the generator with the current time by default.
	CRandomGenerator() 
	{
		gen.seed((unsigned long)std::time(nullptr));
	}

	void seed(int value)
	{
		gen.seed(value);
	}

	/// Generate several integer numbers within the same range.
	/// e.g.: auto a = gen.getRangeI(0,10); a(); a(); a();
	TRandI getRangeI(int lower, int upper)
	{
		return boost::bind(TIntDist(lower, upper), boost::ref(gen));
	}
	
	int getInteger(int lower, int upper)
	{
		return getRangeI(lower, upper)();
	}
	
	/// Generate several double/real numbers within the same range.
	/// e.g.: auto a = gen.getRangeI(0,10); a(); a(); a();
	TRand getRange(double lower, double upper)
	{
		return boost::bind(TRealDist(lower, upper), boost::ref(gen));
	}
	
	double getDouble(double lower, double upper)
	{
		return getRange(lower, upper)();
	}

private:
	TGenerator gen;
};
