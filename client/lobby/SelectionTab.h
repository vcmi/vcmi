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
#include "../Translator.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/filesystem/ResourcePath.h"

class CMap;
class CSlider;
class CLabel;
class CPicture;
class IImage;
class CAnimation;
class CToggleButton;
class ScenarioTabConfigurable;

enum ESortBy
{
	_playerAm, _size, _format, _name, _viccon, _loscon, _numOfMaps, _fileName, _changeDate
}; //_numOfMaps is for campaigns

class ElementInfo : public CMapInfo
{
public:
	ElementInfo() : CMapInfo() { }
	~ElementInfo() { }
	/// Entries outlive the tab that listed them, so each one keeps its own texts installed.
	/// The translator shares ownership of the containers, so a reloaded header can not strand them
	std::vector<TranslatorOverlay> textOverlays;
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

	/// Installs the text overlays of newly parsed entries and fills in their display names
	void installTexts(size_t offset);
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
	size_t requiredHumanPlayers = 1;
	size_t hiddenIncompatibleMapsCount = 0;
	size_t getHiddenIncompatibleMapsCount() const;

	std::shared_ptr<CTextInput> inputName;

	SelectionTab(ESelectionScreen Type);
	void toggleMode();
	void setCurrentFolder(std::string folder);

	void clickReleased(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void clickDouble(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	bool receiveEvent(const Point & position, int eventType) const override;

	void filter(int size, bool selectFirst = false); //0 - all
	void filter(int size, size_t requiredHumanPlayers, bool selectFirst = false);
	void sortBy(int criteria);
	void sort();
	void select(int position); //position: <0 - positions>  position on the screen
	void selectAbs(int position); //position: absolute position in curItems vector
	void sliderMove(int slidPos);
	void updateListItems();
	int getLine() const;
	int getLine(const Point & position) const;
	bool selectFileName(std::string fname);
	void selectNewestFile(bool skipAutosaves = false);
	std::shared_ptr<ElementInfo> getSelectedMapInfo() const;
	void setRequiredHumanPlayers(size_t players);
	void rememberSave(const std::string & savePath) const;
	void rememberCurrentSelection();
	void restoreLastSelection();
	bool checkNameFilter(const std::string & fullstring) const;

private:
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CSlider> slider;
	std::vector<std::shared_ptr<CButton>> buttonsSortBy;
	std::shared_ptr<CLabel> labelTabTitle;
	std::shared_ptr<CTextInput> searchInput;
	std::shared_ptr<CLabel> searchBoxLabel;
	std::shared_ptr<FilledTexturePlayerColored> searchWidgetBackground;
	std::shared_ptr<TransparentFilledRectangle> searchInputRectangle;
	ESelectionScreen tabType;
	Rect inputNameRect;
	int positionsToShow;

	std::shared_ptr<CButton> buttonDeleteMode;
	std::shared_ptr<ScenarioTabConfigurable> scenarioTabConfigurable;
	bool deleteMode;

	bool enableUiEnhancements;
	std::shared_ptr<CButton> buttonCampaignSet;
	std::unordered_set<ResourcePath> resourceFiles;
	std::unordered_map<std::string, std::optional<bool>> folderCompatibility;

	auto checkSubfolder(std::string path);
	size_t getRequiredHumanPlayers() const;
	bool isMapCompatibleWithLobbyPlayerCount(const ElementInfo & info) const;
	bool isSaveCompatible(const CMapInfo & info, ELoadMode loadMode) const;
	std::optional<bool> isFolderCompatible(const std::string & folderName, const std::vector<ResourcePath> & files);
	std::string getLastSaveSettingName() const;
	bool openSaveDirectory(std::string folder);
	void restoreLastSave();

	bool isMapSupported(const CMapInfo & info);
	void parseMaps(const std::unordered_set<ResourcePath> & files);
	void parseCurrentFolder();
	std::vector<ResourcePath> parseSaves(const std::unordered_set<ResourcePath> & files);
	void parseCampaigns(const std::unordered_set<ResourcePath> & files);
	std::unordered_set<ResourcePath> getFiles(std::string dirURI, EResType resType);
	/// Absolute path of an entry, resolved on demand - too expensive to compute for every listed file
	std::string getFullFileURI(const ElementInfo & item) const;

	void handleUnsupportedSavegames(const std::vector<ResourcePath> & files);
};
