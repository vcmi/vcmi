/*
 * RangeGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct RangeGenerator
{
	RangeGenerator(std::function<int()> _myRand, std::vector<int> indexes);
	int generateNumber();
	//get number fulfilling predicate. Never gives the same number twice.
	int getSuchNumber(std::function<bool(int)> goodNumberPred = nullptr);

	int remainingCount;
	std::vector<bool> remaining;
	std::vector<int> obstacleIndexes;
	std::function<int()> myRand;
};
