/*
 * SelectionTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

class CSlider;
class CLabel;

enum ESortBy
{
	_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps, _fileName
}; //_numOfMaps is for campaigns

/// Class which handles map sorting by different criteria
class mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(const std::shared_ptr<CMapInfo> aaa, const std::shared_ptr<CMapInfo> bbb);
	mapSorter(ESortBy es) : sortBy(es){};
};

class SelectionTab : public CIntObject
{
	struct ListItem : public CIntObject
	{
		std::shared_ptr<CLabel> labelAmountOfPlayers;
		std::shared_ptr<CLabel> labelNumberOfCampaignMaps;
		std::shared_ptr<CLabel> labelMapSizeLetter;
		std::shared_ptr<CAnimImage> iconFormat;
		std::shared_ptr<CAnimImage> iconVictoryCondition;
		std::shared_ptr<CAnimImage> iconLossCondition;
		std::shared_ptr<CLabel> labelName;

		ListItem(Point position, std::shared_ptr<CAnimation> iconsFormats, std::shared_ptr<CAnimation> iconsVictory, std::shared_ptr<CAnimation> iconsLoss);
		void updateItem(std::shared_ptr<CMapInfo> info = {}, bool selected = false);
	};
	std::vector<std::shared_ptr<ListItem>> listItems;

	std::shared_ptr<CAnimation> iconsMapFormats;
	// FIXME: CSelectionBase use them too!
	std::shared_ptr<CAnimation> iconsVictoryCondition;
	std::shared_ptr<CAnimation> iconsLossCondition;

public:
	std::vector<std::shared_ptr<CMapInfo>> allItems;
	std::vector<std::shared_ptr<CMapInfo>> curItems;
	size_t selectionPos;
	std::function<void(std::shared_ptr<CMapInfo>)> callOnSelect;

	ESortBy sortingBy;
	ESortBy generalSortingBy;
	bool sortModeAscending;

	std::shared_ptr<CTextInput> inputName;

	SelectionTab(ESelectionScreen Type);
	void toggleMode();

	void clickLeft(tribool down, bool previousState) override;
	void keyPressed(EShortcut key) override;

	void onDoubleClick() override;

	void filter(int size, bool selectFirst = false); //0 - all
	void sortBy(int criteria);
	void sort();
	void select(int position); //position: <0 - positions>  position on the screen
	void selectAbs(int position); //position: absolute position in curItems vector
	void sliderMove(int slidPos);
	void updateListItems();
	int getLine();
	void selectFileName(std::string fname);
	std::shared_ptr<CMapInfo> getSelectedMapInfo() const;
	void rememberCurrentSelection();
	void restoreLastSelection();

private:

	std::shared_ptr<CPicture> background;
	std::shared_ptr<CSlider> slider;
	std::vector<std::shared_ptr<CButton>> buttonsSortBy;
	std::shared_ptr<CLabel> labelTabTitle;
	std::shared_ptr<CLabel> labelMapSizes;
	ESelectionScreen tabType;
	Rect inputNameRect;

	void parseMaps(const std::unordered_set<ResourceID> & files);
	void parseSaves(const std::unordered_set<ResourceID> & files);
	void parseCampaigns(const std::unordered_set<ResourceID> & files);
	std::unordered_set<ResourceID> getFiles(std::string dirURI, int resType);
};
