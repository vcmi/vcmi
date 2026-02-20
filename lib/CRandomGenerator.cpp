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
	logRng->trace("CRandomGenerator constructed");
	resetSeed();
}

CRandomGenerator::CRandomGenerator(int seed)
{
	logRng->trace("CRandomGenerator constructed (%d)", seed);
	setSeed(seed);
}

void CRandomGenerator::setSeed(int seed)
{
	logRng->trace("CRandomGenerator::setSeed (%d)", seed);
	rand.seed(seed);
}

void CRandomGenerator::resetSeed()
{
	logRng->trace("CRandomGenerator::resetSeed");
	std::hash<std::thread::id> hasher;
	auto threadIdHash = hasher(std::this_thread::get_id());
	setSeed(static_cast<int>(threadIdHash * std::time(nullptr)));
}

int CRandomGenerator::nextInt(int upper)
{
	logRng->trace("CRandomGenerator::nextInt (%d)", upper);
	return nextInt(0, upper);
}

int64_t CRandomGenerator::nextInt64(int64_t upper)
{
	logRng->trace("CRandomGenerator::nextInt64 (%d)", upper);
	return nextInt64(0, upper);
}

int CRandomGenerator::nextInt(int lower, int upper)
{
	logRng->trace("CRandomGenerator::nextInt64 (%d, %d)", lower, upper);

	if (lower > upper)
		throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

	return TIntDist(lower, upper)(rand);
}

int CRandomGenerator::nextInt()
{
	logRng->trace("CRandomGenerator::nextInt64");
	return TIntDist()(rand);
}

int CRandomGenerator::nextBinomialInt(int coinsCount, double coinChance)
{
	logRng->trace("CRandomGenerator::nextBinomialInt (%d, %f)", coinsCount, coinChance);
	std::binomial_distribution<> distribution(coinsCount, coinChance);
	return distribution(rand);
}

int64_t CRandomGenerator::nextInt64(int64_t lower, int64_t upper)
{
	logRng->trace("CRandomGenerator::nextInt64 (%d, %d)", lower, upper);
	if (lower > upper)
		throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

	return TInt64Dist(lower, upper)(rand);
}

double CRandomGenerator::nextDouble(double upper)
{
	logRng->trace("CRandomGenerator::nextDouble (%f)", upper);
	return nextDouble(0, upper);
}

double CRandomGenerator::nextDouble(double lower, double upper)
{
	logRng->trace("CRandomGenerator::nextDouble (%f, %f)", lower, upper);
	if(lower > upper)
		throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

	return TRealDist(lower, upper)(rand);
}

CRandomGenerator & CRandomGenerator::getDefault()
{
	static thread_local CRandomGenerator defaultRand;
	return defaultRand;
}


VCMI_LIB_NAMESPACE_END
