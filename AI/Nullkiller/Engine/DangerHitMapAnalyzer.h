/*
* DangerHitMapAnalyzer.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../VCAI.h"
#include "../AIhelper.h"

struct HitMapInfo
{
	uint64_t danger;
	uint8_t turn;

	void reset()
	{
		danger = 0;
		turn = 255;
	}
};

struct HitMapNode
{
	HitMapInfo maximumDanger;
	HitMapInfo fastestDanger;

	void reset()
	{
		maximumDanger.reset();
		fastestDanger.reset();
	}
};

class DangerHitMapAnalyzer
{
private:
	boost::multi_array<HitMapNode, 3> hitMap;

public:
	void updateHitMap();
	uint64_t enemyCanKillOurHeroesAlongThePath(const AIPath & path) const;
};
