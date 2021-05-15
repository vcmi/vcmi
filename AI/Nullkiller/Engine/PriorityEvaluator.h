/*
 * Fuzzy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#pragma once
#include "fl/Headers.h"
#include "../Goals/Goals.h"
#include "../FuzzyEngines.h"

class VCAI;
class CArmedInstance;
class CBank;
struct SectorMap;

class PriorityEvaluator : public engineBase
{
public:
	PriorityEvaluator();
	~PriorityEvaluator();
	void initVisitTile();

	float evaluate(Goals::TSubgoal task);

private:
	fl::InputVariable * armyLossPersentageVariable;
	fl::InputVariable * heroStrengthVariable;
	fl::InputVariable * turnDistanceVariable;
	fl::InputVariable * goldRewardVariable;
	fl::InputVariable * armyRewardVariable;
	fl::OutputVariable * value;
};
