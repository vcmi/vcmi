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
#include "../../lib/mapping/ScenarioList.h"
#include "../../lib/filesystem/ResourcePath.h"

class CMap;
struct CPackForLobby;
struct LobbyDelete;
struct LobbyRequestScenarioList;
struct LobbyScenarioList;
struct ScenarioList;
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

class ElementInfo
{
public:
	/// Map or saved game, empty for folders and campaigns
	std::optional<ScenarioListEntry> entry;
	/// Campaign with its header, null for folders, maps and saved games
	std::shared_ptr<CMapInfo> campaignInfo;
	/// Campaign entries outlive the tab that listed them, so each one keeps its own texts installed
	std::vector<TranslatorOverlay> textOverlays;
	std::string fileURI;
	std::time_t lastWrite = 0;
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

	/// Installs the text overlays of newly parsed campaigns and fills in display names of new entries
	void installTexts(size_t offset);

	void notifySelection(const std::shared_ptr<ElementInfo> & element);

	/// Called once the reply to the request sent to the server is applied, empty if no request is sent
	std::function<void(const LobbyScenarioList &)> onListReceived;
	/// False until list of the current mode is parsed or received
	bool listLoaded = false;

	/// Type of the list requested from the server for this tab
	ScenarioListEntry::EType getListType() const;
	/// Sends pack to which the server replies with a list, input of the screen is blocked until the reply
	void sendAndAwaitList(const CPackForLobby & pack, std::function<void(const LobbyScenarioList &)> onReceived);
	void sendListRequest(LobbyRequestScenarioList & request, std::function<void(const LobbyScenarioList &)> onReceived);
	/// Deletes files on the server and keeps the selection if the selected file is still listed
	void sendDelete(const LobbyDelete & pack);
	void setScenarioList(const ScenarioList & list);
	void finishToggleMode(bool restoreSelection);
	/// Lists folder of the map or save last selected on the server and selects it
	void requestLastSelection();
	/// Selects file with this name if it is in the current folder
	bool selectFileInCurrentFolder(const std::string & fname);
	void selectFirstFile();
public:
	std::vector<std::shared_ptr<ElementInfo>> allItems;
	std::vector<std::shared_ptr<ElementInfo>> curItems;
	std::string curFolder;
	size_t selectionPos;
	/// Called with the selected map, save or campaign, or with null if nothing is selected
	std::function<void(const std::shared_ptr<ElementInfo> &)> callOnSelect;

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
	/// Lists folder, from the server for maps and saved games; onOpened is called once it is listed
	void openFolder(const std::string & folder, const std::function<void()> & onOpened);
	void onScenarioListReceived(const LobbyScenarioList & pack);

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
	/// Opens folder of the file and selects it; selects the first file if it is not found
	void selectFileName(const std::string & fname);
	void selectNewestFile(bool skipAutosaves = false);
	std::shared_ptr<ElementInfo> getSelectedElement() const;
	/// Returns selected map, save or campaign in the form sent to the server
	ScenarioSelection getSelection() const;
	void setRequiredHumanPlayers(size_t players);
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
	/// Campaign files, other lists are received from the server
	std::unordered_set<ResourcePath> resourceFiles;

	auto checkSubfolder(std::string path);
	size_t getRequiredHumanPlayers() const;
	bool isMapCompatibleWithLobbyPlayerCount(const ElementInfo & info) const;
	void parseCurrentFolder();
	void parseCampaigns(const std::unordered_set<ResourcePath> & files);
	std::unordered_set<ResourcePath> getFiles(std::string dirURI, EResType resType);
	/// Absolute path of an entry, resolved on demand - too expensive to compute for every listed file
	std::string getFullFileURI(const ElementInfo & item) const;

	void handleUnsupportedSavegames(const std::vector<std::string> & files);
};
