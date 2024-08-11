/*
 * CStatisticScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../windows/CWindowObject.h"
#include "../../lib/gameState/GameStatistics.h"

class FilledTexturePlayerColored;

class CStatisticScreen : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;

	std::vector<std::shared_ptr<CIntObject>> layout;
public:
	CStatisticScreen(StatisticDataSet statistic);
};
