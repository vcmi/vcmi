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

class CSlider;
class CLabel;
class CPicture;
class IImage;

enum ESortBy
{
	_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps, _fileName
}; //_numOfMaps is for campaigns

class ElementInfo : public CMapInfo
{
public:
	ElementInfo() : CMapInfo() { }
	~ElementInfo() { }
	std::string folderName = "";
	bool isFolder = false;
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

		ListItem(Point position, std::shared_ptr<CAnimation> iconsFormats, std::shared_ptr<CAnimation> iconsVictory, std::shared_ptr<CAnimation> iconsLoss);
		void updateItem(std::shared_ptr<ElementInfo> info = {}, bool selected = false);
	};
	std::vector<std::shared_ptr<ListItem>> listItems;

	std::shared_ptr<CAnimation> iconsMapFormats;
	// FIXME: CSelectionBase use them too!
	std::shared_ptr<CAnimation> iconsVictoryCondition;
	std::shared_ptr<CAnimation> iconsLossCondition;

	class CMapInfoTooltipBox : public CWindowObject
	{
		const int IMAGE_SIZE = 169;
		const int BORDER = 30;

		bool drawPlayerElements;
		bool renderImage;

		std::shared_ptr<CFilledTexture> backgroundTexture;
		std::shared_ptr<CTextBox> label;
		std::shared_ptr<CPicture> image1;
		std::shared_ptr<CPicture> image2;

		Canvas createMinimapForLayer(std::unique_ptr<CMap> & map, int layer);
		std::vector<std::shared_ptr<IImage>> createMinimaps(ResourceID resource, int size);
	public:
		CMapInfoTooltipBox(std::string text, ResourceID resource, ESelectionScreen tabType);
	};
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

	auto checkSubfolder(std::string path);

	bool isMapSupported(const CMapInfo & info);
	void parseMaps(const std::unordered_set<ResourceID> & files);
	void parseSaves(const std::unordered_set<ResourceID> & files);
	void parseCampaigns(const std::unordered_set<ResourceID> & files);
	std::unordered_set<ResourceID> getFiles(std::string dirURI, int resType);
};
