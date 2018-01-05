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

/// The random map tab shows options for generating a random map.
class RandomMapTab : public CIntObject
{
public:
	RandomMapTab();

	void showAll(SDL_Surface * to) override;
	void updateMapInfoByHost();
	void setMapGenOptions(std::shared_ptr<CMapGenOptions> opts);

	CFunctionList<void(std::shared_ptr<CMapInfo>, std::shared_ptr<CMapGenOptions>)> mapInfoChanged;

private:
	void addButtonsWithRandToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex, int helpRandIndex) const;
	void addButtonsToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int startIndex, int endIndex, int btnWidth, int helpStartIndex) const;
	void deactivateButtonsFrom(CToggleGroup * group, int startId);
	void validatePlayersCnt(int playersCnt);
	void validateCompOnlyPlayersCnt(int compOnlyPlayersCnt);
	std::vector<int> getPossibleMapSizes();

	CPicture * bg;
	CToggleButton * twoLevelsBtn;
	CToggleGroup * mapSizeBtnGroup, * playersCntGroup, * teamsCntGroup, * compOnlyPlayersCntGroup,
		* compOnlyTeamsCntGroup, * waterContentGroup, * monsterStrengthGroup;
	CButton * showRandMaps;
	std::shared_ptr<CMapGenOptions> mapGenOptions;
	std::shared_ptr<CMapInfo> mapInfo;
};
