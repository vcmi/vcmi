/*
 * RandomMapTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

#include "../../lib/FunctionList.h"
#include "../../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapGenOptions;

VCMI_LIB_NAMESPACE_END

class CToggleButton;
class CLabel;
class CLabelGroup;

class RandomMapTab : public CIntObject
{
public:
	RandomMapTab();

	void updateMapInfoByHost();
	void setMapGenOptions(std::shared_ptr<CMapGenOptions> opts);

	CFunctionList<void(std::shared_ptr<CMapInfo>, std::shared_ptr<CMapGenOptions>)> mapInfoChanged;

private:
	void addButtonsWithRandToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex, int helpRandIndex, int randIndex = -1, bool animIdfromBtnId = true) const;
	void addButtonsToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex, bool animIdfromBtnId = true) const;
	void deactivateButtonsFrom(CToggleGroup * group, int startId);
	void validatePlayersCnt(int playersCnt);
	void validateCompOnlyPlayersCnt(int compOnlyPlayersCnt);
	std::vector<int> getPossibleMapSizes();


	std::shared_ptr<CPicture> background;
	std::shared_ptr<CLabel> labelHeadlineBig;
	std::shared_ptr<CLabel> labelHeadlineSmall;

	std::shared_ptr<CLabel> labelMapSize;
	std::shared_ptr<CToggleGroup> groupMapSize;
	std::shared_ptr<CToggleButton> buttonTwoLevels;

	std::shared_ptr<CLabelGroup> labelGroupForOptions;
	std::shared_ptr<CToggleGroup> groupMaxPlayers;
	std::shared_ptr<CToggleGroup> groupMaxTeams;
	std::shared_ptr<CToggleGroup> groupCompOnlyPlayers;
	std::shared_ptr<CToggleGroup> groupCompOnlyTeams;
	std::shared_ptr<CToggleGroup> groupWaterContent;
	std::shared_ptr<CToggleGroup> groupMonsterStrength;
	std::shared_ptr<CButton> buttonShowRandomMaps;
	std::shared_ptr<CMapGenOptions> mapGenOptions;
	std::shared_ptr<CMapInfo> mapInfo;
};
