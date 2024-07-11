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

CRandomGenerator::CRandomGenerator()
{
	logRng->trace("(constructor)");
	resetSeed();
}

CRandomGenerator::CRandomGenerator(int seed)
{
	setSeed(seed);
}

void CRandomGenerator::setSeed(int seed)
{
	logRng->debug("setSeed(%d)", seed);
	rand.seed(seed);
}

void CRandomGenerator::resetSeed()
{
	logRng->trace("resetSeed()");
	boost::hash<std::string> stringHash;
	auto threadIdHash = stringHash(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
	setSeed(static_cast<int>(threadIdHash * std::time(nullptr)));
}

TRandI CRandomGenerator::getIntRange(int lower, int upper)
{
	logRng->trace("getIntRange(%d, %d)", lower, upper);
	if (lower <= upper)
		return std::bind(TIntDist(lower, upper), std::ref(rand));
	throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));
}

vstd::TRandI64 CRandomGenerator::getInt64Range(int64_t lower, int64_t upper)
{
	logRng->trace("getInt64Range(%d, %d)", lower, upper);
	if(lower <= upper)
		return std::bind(TInt64Dist(lower, upper), std::ref(rand));
	throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));
}

int CRandomGenerator::nextInt(int upper)
{
	auto res = getIntRange(0, upper)();
	logRng->debug("nextInt(%d) -> %d", upper, res);
	return res;
}

int CRandomGenerator::nextInt(int lower, int upper)
{
	auto res = getIntRange(lower, upper)();
	logRng->debug("nextInt(%d, %d) -> %d", lower, upper, res);
	return res;
}

int CRandomGenerator::nextInt()
{
	auto res = TIntDist()(rand);
	logRng->debug("nextInt() -> %d", res);
	return res;
}

vstd::TRand CRandomGenerator::getDoubleRange(double lower, double upper)
{
	logRng->trace("getDoubleRange(%f, %f)", lower, upper);
	if(lower <= upper)
		return std::bind(TRealDist(lower, upper), std::ref(rand));
	throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

}

double CRandomGenerator::nextDouble(double upper)
{
	auto res = getDoubleRange(0, upper)();
	logRng->debug("nextDouble(%d) -> %d", upper, res);
	return res;
}

double CRandomGenerator::nextDouble(double lower, double upper)
{
	auto res = getDoubleRange(lower, upper)();
	logRng->debug("nextDouble(%d, %d) -> %d", lower, upper, res);
	return res;
}

double CRandomGenerator::nextDouble()
{
	auto res = TRealDist()(rand);
	logRng->debug("nextDouble() -> %d", res);
	return res;
}

CRandomGenerator & CRandomGenerator::getDefault()
{
	static thread_local CRandomGenerator defaultRand;
	return defaultRand;
}

TGenerator & CRandomGenerator::getStdGenerator()
{
	return rand;
}

VCMI_LIB_NAMESPACE_END
