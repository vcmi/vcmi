/*
 * CCampaignInfoScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"
#include "CBonusSelection.h"

class CCampaignInfoScreen : public CBonusSelection, ISelectionScreenInfo
{
	const StartInfo * localSi;
	CMapInfo * localMi;

public:
	CCampaignInfoScreen();
	~CCampaignInfoScreen();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
};