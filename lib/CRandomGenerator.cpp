/*
 * CRandomGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRandomGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

boost::thread_specific_ptr<CRandomGenerator> CRandomGenerator::defaultRand;

CRandomGenerator::CRandomGenerator()
{
	resetSeed();
}

CRandomGenerator::CRandomGenerator(int seed)
{
	setSeed(seed);
}

void CRandomGenerator::setSeed(int seed)
{
	rand.seed(seed);
}

void CRandomGenerator::resetSeed()
{
	boost::hash<std::string> stringHash;
	auto threadIdHash = stringHash(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
	setSeed(static_cast<int>(threadIdHash * std::time(nullptr)));
}

TRandI CRandomGenerator::getIntRange(int lower, int upper)
{
	return std::bind(TIntDist(lower, upper), std::ref(rand));
}

vstd::TRandI64 CRandomGenerator::getInt64Range(int64_t lower, int64_t upper)
{
	return std::bind(TInt64Dist(lower, upper), std::ref(rand));
}

int CRandomGenerator::nextInt(int upper)
{
	return getIntRange(0, upper)();
}

int CRandomGenerator::nextInt(int lower, int upper)
{
	return getIntRange(lower, upper)();
}

int CRandomGenerator::nextInt()
{
	return TIntDist()(rand);
}

vstd::TRand CRandomGenerator::getDoubleRange(double lower, double upper)
{
    return std::bind(TRealDist(lower, upper), std::ref(rand));
}

double CRandomGenerator::nextDouble(double upper)
{
	return getDoubleRange(0, upper)();
}

double CRandomGenerator::nextDouble(double lower, double upper)
{
	return getDoubleRange(lower, upper)();
}

double CRandomGenerator::nextDouble()
{
	return TRealDist()(rand);
}

CRandomGenerator & CRandomGenerator::getDefault()
{
	if(!defaultRand.get())
	{
		defaultRand.reset(new CRandomGenerator());
	}
	return *defaultRand;
}

TGenerator & CRandomGenerator::getStdGenerator()
{
	return rand;
}

VCMI_LIB_NAMESPACE_END
