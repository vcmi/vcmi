/*
 * RangeGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RangeGenerator.h"

RangeGenerator::RangeGenerator(std::function<int ()> _myRand, std::vector<int> indexes):
	remainingCount(indexes.size()),
	remaining(remainingCount, true),
	myRand(_myRand),
	obstacleIndexes(indexes)
{
}

int RangeGenerator::generateNumber()
{
	if(remainingCount == 1)
		return 0;
	return myRand() % remainingCount;
}

int RangeGenerator::getSuchNumber(std::function<bool (int)> goodNumberPred)
{
	int ret = -1;
	do
	{
		if(remainingCount == 0)
			return -1;
		int n = generateNumber();
		int i = 0;
		for(;;i++)
		{
			if(!remaining[i])
				continue;
			if(!n)
				break;
			n--;
		}
		remainingCount--;
		remaining[i] = false;
		ret = obstacleIndexes.at(i);
	} while(goodNumberPred && !goodNumberPred(ret));
	return ret;
}
