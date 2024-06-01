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

int CRandomGenerator::nextInt(int upper)
{
	return nextInt(0, upper);
}

int64_t CRandomGenerator::nextInt64(int64_t upper)
{
	return nextInt64(0, upper);
}

int CRandomGenerator::nextInt(int lower, int upper)
{
	if (lower > upper)
		throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

	return TIntDist(lower, upper)(rand);
}

int CRandomGenerator::nextInt()
{
	return TIntDist()(rand);
}

int CRandomGenerator::nextBinomialInt(int coinsCount, double coinChance)
{
	std::binomial_distribution<> distribution(coinsCount, coinChance);
	return distribution(rand);
}

int64_t CRandomGenerator::nextInt64(int64_t lower, int64_t upper)
{
	if (lower > upper)
		throw std::runtime_error("Invalid range provided: " + std::to_string(lower) + " ... " + std::to_string(upper));

	return TInt64Dist(lower, upper)(rand);
}

double CRandomGenerator::nextDouble(double upper)
{
	return nextDouble(0, upper);
}

double CRandomGenerator::nextDouble(double lower, double upper)
{
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
