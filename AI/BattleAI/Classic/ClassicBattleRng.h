/*
 * ClassicBattleRng.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vstd/RNG.h>

/// Random source used only by the classic tactical evaluator.
class IClassicBattleAIRng
{
public:
	virtual ~IClassicBattleAIRng() = default;
	virtual int32_t nextIntInclusive(int32_t lower, int32_t upper) = 0;
};

/// Production adapter for VCMI's client-side random generator.
class ClassicBattleAIRng final : public IClassicBattleAIRng
{
public:
	int32_t nextIntInclusive(int32_t lower, int32_t upper) override;
};

/// Adapts the inclusive classic stream to rule helpers that consume vstd::RNG.
class ClassicVstdRngAdapter final : public vstd::RNG
{
	IClassicBattleAIRng & source;

public:
	explicit ClassicVstdRngAdapter(IClassicBattleAIRng & source);

	int nextInt(int lower, int upper) override;
	int64_t nextInt64(int64_t lower, int64_t upper) override;
	double nextDouble(double lower, double upper) override;
	int nextInt(int upper) override;
	int64_t nextInt64(int64_t upper) override;
	double nextDouble(double upper) override;
	int nextInt() override;
	int nextBinomialInt(int coinsCount, double coinChance) override;
};
