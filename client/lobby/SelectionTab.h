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
VCMI_LIB_NAMESPACE_BEGIN
class CMap;
VCMI_LIB_NAMESPACE_END
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/filesystem/ResourcePath.h"

class CSlider;
class CLabel;
class CPicture;
class IImage;
class CAnimation;
class CToggleButton;

enum ESortBy
{
	_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps, _fileName, _changeDate
}; //_numOfMaps is for campaigns

class ElementInfo : public CMapInfo
{
public:
	ElementInfo() : CMapInfo() { }
	~ElementInfo() { }
	std::string folderName = "";
	std::string name = "";
	bool isFolder = false;
	bool isAutoSaveFolder = false;
};

/// Class which handles map sorting by different criteria
class mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(const std::shared_ptr<ElementInfo> aaa, const std::shared_ptr<ElementInfo> bbb);
	mapSorter(ESortBy es) : sortBy(es){};
};

class SelectionTab : public CIntObject
{
	struct ListItem : public CIntObject
	{
		std::shared_ptr<CLabel> labelAmountOfPlayers;
		std::shared_ptr<CLabel> labelNumberOfCampaignMaps;
		std::shared_ptr<CLabel> labelMapSizeLetter;
		std::shared_ptr<CPicture> iconFolder;
		std::shared_ptr<CAnimImage> iconFormat;
		std::shared_ptr<CAnimImage> iconVictoryCondition;
		std::shared_ptr<CAnimImage> iconLossCondition;
		std::shared_ptr<CPicture> pictureEmptyLine;
		std::shared_ptr<CLabel> labelName;

		const int LABEL_POS_X = 184;

		ListItem(Point position);
		void updateItem(std::shared_ptr<ElementInfo> info = {}, bool selected = false);
	};
	std::vector<std::shared_ptr<ListItem>> listItems;

	std::shared_ptr<CAnimation> iconsMapFormats;
	// FIXME: CSelectionBase use them too!
	std::shared_ptr<CAnimation> iconsVictoryCondition;
	std::shared_ptr<CAnimation> iconsLossCondition;

	std::vector<std::shared_ptr<ListItem>> unSupportedSaves;

	JsonNode campaignSets;
public:
	std::vector<std::shared_ptr<ElementInfo>> allItems;
	std::vector<std::shared_ptr<ElementInfo>> curItems;
	std::string curFolder;
	size_t selectionPos;
	std::function<void(std::shared_ptr<ElementInfo>)> callOnSelect;

	ESortBy sortingBy;
	ESortBy generalSortingBy;
	bool sortModeAscending;
	int currentMapSizeFilter = 0;
	bool showRandom;

	std::shared_ptr<CTextInput> inputName;

	SelectionTab(ESelectionScreen Type);
	void toggleMode();

	void clickReleased(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void clickDouble(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	bool receiveEvent(const Point & position, int eventType) const override;

	void filter(int size, bool selectFirst = false); //0 - all
	void sortBy(int criteria);
	void sort();
	void select(int position); //position: <0 - positions>  position on the screen
	void selectAbs(int position); //position: absolute position in curItems vector
	void sliderMove(int slidPos);
	void updateListItems();
	int getLine() const;
	int getLine(const Point & position) const;
	void selectFileName(std::string fname);
	void selectNewestFile();
	std::shared_ptr<ElementInfo> getSelectedMapInfo() const;
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

	std::shared_ptr<CButton> buttonDeleteMode;
	bool deleteMode;

	std::shared_ptr<CButton> buttonBattleOnlyMode;

	bool enableUiEnhancements;
	std::shared_ptr<CButton> buttonCampaignSet;

	auto checkSubfolder(std::string path);

	bool isMapSupported(const CMapInfo & info);
	void parseMaps(const std::unordered_set<ResourcePath> & files);
	std::vector<ResourcePath> parseSaves(const std::unordered_set<ResourcePath> & files);
	void parseCampaigns(const std::unordered_set<ResourcePath> & files);
	std::unordered_set<ResourcePath> getFiles(std::string dirURI, EResType resType);

	void handleUnsupportedSavegames(const std::vector<ResourcePath> & files);
};

class CampaignSetSelector : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::vector<std::shared_ptr<CToggleButton>> buttons;
	std::shared_ptr<CSlider> slider;

	const int LINES = 10;

	std::vector<std::string> texts;
	std::function<void(int selectedIndex)> cb;

	void update(int to);
public:
	CampaignSetSelector(const std::vector<std::string> & texts, const std::function<void(int selectedIndex)> & cb);
};