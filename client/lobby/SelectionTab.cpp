/*
 * SelectionTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SelectionTab.h"
#include "CSelectionBase.h"
#include "CLobbyScreen.h"
#include "BattleOnlyModeTab.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../windows/CMapOverview.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../mainmenu/CCampaignScreen.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonUtils.h"

bool mapSorter::operator()(const std::shared_ptr<ElementInfo> aaa, const std::shared_ptr<ElementInfo> bbb)
{
	if(aaa->isFolder || bbb->isFolder)
	{
		if(aaa->isFolder != bbb->isFolder)
			return (aaa->isFolder > bbb->isFolder);
		else
		{
			if(boost::algorithm::starts_with(aaa->folderName, "..") || boost::algorithm::starts_with(bbb->folderName, ".."))
				return boost::algorithm::starts_with(aaa->folderName, "..");
			return TextOperations::compareLocalizedStrings(aaa->folderName, bbb->folderName);
		}
	}

	auto a = aaa->mapHeader.get();
	auto b = bbb->mapHeader.get();
	if(a && b) //if we are sorting scenarios
	{
		switch(sortBy)
		{
		case _format: //by map format (RoE, WoG, etc)
			return (a->version < b->version);
			break;
		case _loscon: //by loss conditions
			return (a->defeatIconIndex < b->defeatIconIndex);
			break;
		case _playerAm: //by player amount
			int playerAmntB;
			int humenPlayersB;
			int playerAmntA;
			int humenPlayersA;
			playerAmntB = humenPlayersB = playerAmntA = humenPlayersA = 0;
			for(int i = 0; i < 8; i++)
			{
				if(a->players[i].canHumanPlay)
				{
					playerAmntA++;
					humenPlayersA++;
				}
				else if(a->players[i].canComputerPlay)
				{
					playerAmntA++;
				}
				if(b->players[i].canHumanPlay)
				{
					playerAmntB++;
					humenPlayersB++;
				}
				else if(b->players[i].canComputerPlay)
				{
					playerAmntB++;
				}
			}
			if(playerAmntB != playerAmntA)
				return (playerAmntA < playerAmntB);
			else
				return (humenPlayersA < humenPlayersB);
			break;
		case _size: //by size of map
			return (a->width < b->width);
			break;
		case _viccon: //by victory conditions
			return (a->victoryIconIndex < b->victoryIconIndex);
			break;
		case _name: //by name
			return TextOperations::compareLocalizedStrings(aaa->name, bbb->name);
		case _fileName: //by filename
			return TextOperations::compareLocalizedStrings(aaa->fileURI, bbb->fileURI);
		case _changeDate: //by changedate
			return aaa->lastWrite < bbb->lastWrite;
		default:
			return TextOperations::compareLocalizedStrings(aaa->name, bbb->name);
		}
	}
	else //if we are sorting campaigns
	{
		switch(sortBy)
		{
		case _numOfMaps: //by number of maps in campaign
			return aaa->campaign->scenariosCount() < bbb->campaign->scenariosCount();
		case _name: //by name
			return TextOperations::compareLocalizedStrings(aaa->campaign->getNameTranslated(), bbb->campaign->getNameTranslated());
		default:
			return TextOperations::compareLocalizedStrings(aaa->campaign->getNameTranslated(), bbb->campaign->getNameTranslated());
		}
	}
}

// pick sorting order based on selection
static ESortBy getSortBySelectionScreen(ESelectionScreen Type)
{
	switch(Type)
	{
	case ESelectionScreen::newGame:
		return ESortBy::_name;
	case ESelectionScreen::loadGame:
	case ESelectionScreen::saveGame:
		return ESortBy::_fileName;
	case ESelectionScreen::campaignList:
		return ESortBy::_name;
	}
	// Should not reach here. But let's not crash the game.
	return ESortBy::_name;
}

SelectionTab::SelectionTab(ESelectionScreen Type)
	: CIntObject(LCLICK | SHOW_POPUP | KEYBOARD | DOUBLECLICK)
	, callOnSelect(nullptr)
	, tabType(Type)
	, selectionPos(0)
	, sortModeAscending(true)
	, inputNameRect{32, 539, 350, 20}
	, curFolder("")
	, currentMapSizeFilter(0)
	, showRandom(false)
	, deleteMode(false)
	, enableUiEnhancements(settings["general"]["enableUiEnhancements"].Bool())
	, campaignSets(JsonUtils::assembleFromFiles("config/campaignSets.json"))
{
	OBJECT_CONSTRUCTION;
		
	generalSortingBy = getSortBySelectionScreen(tabType);
	sortingBy = _format;

	if(tabType != ESelectionScreen::campaignList)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("SCSELBCK.bmp"), 0, 6);
		pos = background->pos;
		inputName = std::make_shared<CTextInput>(inputNameRect, Point(-32, -25), ImagePath::builtin("GSSTRIP.bmp"));
		inputName->setFilterFilename();
		labelMapSizes = std::make_shared<CLabel>(87, 62, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[510]);

		// TODO: Global constants?
		constexpr std::array sizes = {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE, 0};
		constexpr std::array filterIconNmes = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		constexpr std::array filterShortcuts = { EShortcut::MAPS_SIZE_S, EShortcut::MAPS_SIZE_M, EShortcut::MAPS_SIZE_L, EShortcut::MAPS_SIZE_XL, EShortcut::MAPS_SIZE_ALL };

		for(int i = 0; i < 5; i++)
			buttonsSortBy.push_back(std::make_shared<CButton>(Point(158 + 47 * i, 46), AnimationPath::builtin(filterIconNmes[i]), LIBRARY->generaltexth->zelp[54 + i], std::bind(&SelectionTab::filter, this, sizes[i], true), filterShortcuts[i]));

		constexpr std::array xpos = {23, 55, 88, 121, 306, 339};
		constexpr std::array sortIconNames = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		constexpr std::array sortShortcuts = { EShortcut::MAPS_SORT_PLAYERS, EShortcut::MAPS_SORT_SIZE, EShortcut::MAPS_SORT_FORMAT, EShortcut::MAPS_SORT_NAME, EShortcut::MAPS_SORT_VICTORY, EShortcut::MAPS_SORT_DEFEAT };
		for(int i = 0; i < 6; i++)
		{
			ESortBy criteria = (ESortBy)i;
			if(criteria == _name)
				criteria = generalSortingBy;

			buttonsSortBy.push_back(std::make_shared<CButton>(Point(xpos[i], 86), AnimationPath::builtin(sortIconNames[i]), LIBRARY->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria), sortShortcuts[i]));
		}
	}

	int positionsToShow = 18;
	std::string tabTitle;
	std::string tabTitleDelete;
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		tabTitle = "{" + LIBRARY->generaltexth->arraytxt[229] + "}";
		tabTitleDelete = "{red|" + LIBRARY->generaltexth->translate("vcmi.lobby.deleteMapTitle") + "}";
		break;
	case ESelectionScreen::loadGame:
		tabTitle = "{" + LIBRARY->generaltexth->arraytxt[230] + "}";
		tabTitleDelete = "{red|" + LIBRARY->generaltexth->translate("vcmi.lobby.deleteSaveGameTitle") + "}";
		break;
	case ESelectionScreen::saveGame:
		positionsToShow = 16;
		tabTitle = "{" + LIBRARY->generaltexth->arraytxt[231] + "}";
		break;
	case ESelectionScreen::campaignList:
		tabTitle = "{" + LIBRARY->generaltexth->allTexts[726] + "}";
		setRedrawParent(true); // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3;
		pos.y += 6;

		buttonsSortBy.push_back(std::make_shared<CButton>(Point(23, 86), AnimationPath::builtin("CamCusM.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps), EShortcut::MAPS_SORT_MAPS));
		buttonsSortBy.push_back(std::make_shared<CButton>(Point(55, 86), AnimationPath::builtin("CamCusL.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name), EShortcut::MAPS_SORT_NAME));
		break;
	default:
		assert(0);
		break;
	}

	if(enableUiEnhancements)
	{
		auto sortByDate = std::make_shared<CButton>(Point(371, 85), AnimationPath::builtin("selectionTabSortDate"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.sortDate")), std::bind(&SelectionTab::sortBy, this, ESortBy::_changeDate), EShortcut::MAPS_SORT_CHANGEDATE);
		sortByDate->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/selectionTabSortDate")));
		buttonsSortBy.push_back(sortByDate);

		if(tabType == ESelectionScreen::loadGame || tabType == ESelectionScreen::newGame)
		{
			buttonDeleteMode = std::make_shared<CButton>(Point(367, 18), AnimationPath::builtin("lobby/deleteButton"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.deleteMode")), [this, tabTitle, tabTitleDelete](){
				deleteMode = !deleteMode;
				if(deleteMode)
					labelTabTitle->setText(tabTitleDelete);
				else
					labelTabTitle->setText(tabTitle);
			});

			if(tabType == ESelectionScreen::newGame)
				buttonDeleteMode->setEnabled(false);
		}

		if(tabType == ESelectionScreen::campaignList)
		{
			buttonCampaignSet = std::make_shared<CButton>(Point(262, 53), AnimationPath::builtin("GSPBUT2.DEF"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.selectionTab.campaignSets.help")), [this]{
				std::vector<std::pair<si64, std::pair<std::string, std::string>>> namesWithIndex;
				for (auto const & set : campaignSets.Struct())
				{
					bool oneCampaignExists = false;
					for (auto const & item : set.second["items"].Vector())
						if(CResourceHandler::get()->existsResource(ResourcePath(item["file"].String(), EResType::CAMPAIGN)))
							oneCampaignExists = true;

					if(oneCampaignExists)
						namesWithIndex.push_back({set.second["index"].isNull() ? std::numeric_limits<si64>::max() : set.second["index"].Integer(), { set.first, set.second["text"].isNull() ? set.first : LIBRARY->generaltexth->translate(set.second["text"].String()) }});
				}

				std::sort(namesWithIndex.begin(), namesWithIndex.end(), [](const std::pair<si64, std::pair<std::string, std::string>>& a, const std::pair<si64, std::pair<std::string, std::string>>& b)
				{
					if (a.first != b.first) return a.first < b.first;
					return a.second.second < b.second.second;
				});

				std::vector<std::string> namesIdentifier;
				for (const auto& pair : namesWithIndex)
					namesIdentifier.push_back(pair.second.first);
				std::vector<std::string> namesTranslated;
				for (const auto& pair : namesWithIndex)
					namesTranslated.push_back(pair.second.second);

				auto window = std::make_shared<CObjectListWindow>(namesTranslated, nullptr, LIBRARY->generaltexth->translate("vcmi.selectionTab.campaignSets.hover"), LIBRARY->generaltexth->translate("vcmi.selectionTab.campaignSets.hover"), [this, namesIdentifier](int index){
					GAME->server().sendClientDisconnecting();
					(static_cast<CLobbyScreen *>(parent))->close();
					ENGINE->windows().createAndPushWindow<CCampaignScreen>(campaignSets, namesIdentifier[index]);
				}, 0, std::vector<std::shared_ptr<IImage>>(), true, true);
				ENGINE->windows().pushWindow(window);
			}, EShortcut::LOBBY_CAMPAIGN_SETS);
			buttonCampaignSet->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.selectionTab.campaignSets.hover"), FONT_SMALL, Colors::WHITE);
		}
	}

	for(int i = 0; i < positionsToShow; i++)
		listItems.push_back(std::make_shared<ListItem>(Point(30, 129 + i * 25)));

	labelTabTitle = std::make_shared<CLabel>(205, 28, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, tabTitle);
	slider = std::make_shared<CSlider>(Point(372, 86 + (enableUiEnhancements ? 30 : 0)), (tabType != ESelectionScreen::saveGame ? 480 : 430) - (enableUiEnhancements ? 30 : 0), std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, (int)curItems.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(24);

	// create scroll bounds that encompass all area in this UI element to the left of slider (including area of slider itself)
	// entire screen can't be used in here since map description might also have slider
	slider->setScrollBounds(Rect(pos.x - slider->pos.x, 0, slider->pos.x + slider->pos.w - pos.x, slider->pos.h ));
	filter(0);
}

void SelectionTab::toggleMode()
{
	allItems.clear();
	curItems.clear();
	if(GAME->server().isGuest())
	{
		if(slider)
			slider->block(true);
	}
	else
	{
		switch(tabType)
		{
		case ESelectionScreen::newGame:
			{
				inputName->disable();
				auto files = getFiles("Maps/", EResType::MAP);
				files.erase(ResourcePath("Maps/Tutorial.tut", EResType::MAP));
				files.erase(ResourcePath("Maps/BattleOnlyMode.vmap", EResType::MAP));
				parseMaps(files);
				break;
			}

		case ESelectionScreen::loadGame:
			{
				inputName->disable();
				auto unsupported = parseSaves(getFiles("Saves/", EResType::SAVEGAME));
				handleUnsupportedSavegames(unsupported);
				break;
			}

		case ESelectionScreen::saveGame:
			parseSaves(getFiles("Saves/", EResType::SAVEGAME));
			inputName->enable();
			inputName->activate();
			restoreLastSelection();
			break;

		case ESelectionScreen::campaignList:
			parseCampaigns(getFiles("Maps/", EResType::CAMPAIGN));
			break;

		default:
			assert(0);
			break;
		}
		if(slider)
		{
			slider->block(false);
			filter(0);
		}

		if(GAME->server().campaignStateToSend)
		{
			GAME->server().setCampaignState(GAME->server().campaignStateToSend);
			GAME->server().campaignStateToSend.reset();
		}
		else
		{
			restoreLastSelection();
		}
	}
	slider->setAmount((int)curItems.size());
	updateListItems();
	redraw();
}

void SelectionTab::clickReleased(const Point & cursorPosition)
{
	int line = getLine();

	if(line != -1 && curItems.size() > line)
	{
		if(!deleteMode)
			select(line);
		else
		{
			int py = line + slider->getValue();
			vstd::amax(py, 0);
			vstd::amin(py, curItems.size() - 1);

			if(curItems[py]->isFolder && boost::algorithm::starts_with(curItems[py]->folderName, ".."))
			{
				select(line);
				return;
			}

			if(!curItems[py]->isFolder)
				CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.lobby.deleteFile") + "\n\n" + curItems[py]->fullFileURI, std::vector<std::shared_ptr<CComponent>>(), [this, py](){
					LobbyDelete ld;
					ld.type = tabType == ESelectionScreen::newGame ? LobbyDelete::EType::RANDOMMAP : LobbyDelete::EType::SAVEGAME;
					ld.name = curItems[py]->fileURI;
					GAME->server().sendLobbyPack(ld);
				}, nullptr);
			else
				CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.lobby.deleteFolder") + "\n\n" + curFolder + curItems[py]->folderName, std::vector<std::shared_ptr<CComponent>>(), [this, py](){
					LobbyDelete ld;
					ld.type = LobbyDelete::EType::SAVEGAME_FOLDER;
					ld.name = curFolder + curItems[py]->folderName;
					GAME->server().sendLobbyPack(ld);
				}, nullptr);
		}
	}
#ifdef VCMI_MOBILE
	// focus input field if clicked inside it
	else if(inputName && inputName->isActive() && inputNameRect.isInside(cursorPosition))
		inputName->giveFocus();
#endif

}

void SelectionTab::keyPressed(EShortcut key)
{
	int moveBy = 0;
	switch(key)
	{
	case EShortcut::MOVE_UP:
		moveBy = -1;
		break;
	case EShortcut::MOVE_DOWN:
		moveBy = +1;
		break;
	case EShortcut::MOVE_PAGE_UP:
		moveBy = -(int)listItems.size() + 1;
		break;
	case EShortcut::MOVE_PAGE_DOWN:
		moveBy = +(int)listItems.size() - 1;
		break;
	case EShortcut::MOVE_FIRST:
		select(-slider->getValue());
		return;
	case EShortcut::MOVE_LAST:
		select((int)curItems.size() - slider->getValue());
		return;
	default:
		return;
	}
	select((int)selectionPos - slider->getValue() + moveBy);
}

void SelectionTab::clickDouble(const Point & cursorPosition)
{
	int position = getLine();
	int itemIndex = position + slider->getValue();

	if(itemIndex >= curItems.size())
		return;

	auto clickedItem = curItems[itemIndex];
	auto selectedItem = getSelectedMapInfo();

	if (clickedItem != selectedItem)
	{
		// double-click BUT player hit different item than he had selected
		// ignore - clickReleased would still trigger and update selection.
		// After which another (3rd) click if it happens would still register as double-click
		return;
	}

	if(itemIndex >= 0 && curItems[itemIndex]->isFolder)
	{
		select(position);
		return;
	}

	if(getLine() != -1) //double clicked scenarios list
	{
		(static_cast<CLobbyScreen *>(parent))->buttonStart->clickPressed(cursorPosition);
		(static_cast<CLobbyScreen *>(parent))->buttonStart->clickReleased(cursorPosition);
	}
}

void SelectionTab::showPopupWindow(const Point & cursorPosition)
{
	int position = getLine();
	int py = position + slider->getValue();

	if(py >= curItems.size())
		return;

	if(!curItems[py]->isFolder)
	{
		std::string creationDateTime;
		std::string author;
		std::string mapVersion;
		if(tabType != ESelectionScreen::campaignList)
		{
			author = curItems[py]->mapHeader->author.toString() + (!curItems[py]->mapHeader->authorContact.toString().empty() ? (" <" + curItems[py]->mapHeader->authorContact.toString() + ">") : "");
			mapVersion = curItems[py]->mapHeader->mapVersion.toString();
			creationDateTime = tabType == ESelectionScreen::newGame && curItems[py]->mapHeader->creationDateTime ? TextOperations::getFormattedDateTimeLocal(curItems[py]->mapHeader->creationDateTime) : curItems[py]->date;
		}
		else
		{
			author = curItems[py]->campaign->getAuthor() + (!curItems[py]->campaign->getAuthorContact().empty() ? (" <" + curItems[py]->campaign->getAuthorContact() + ">") : "");
			mapVersion = curItems[py]->campaign->getCampaignVersion();
			creationDateTime = curItems[py]->campaign->getCreationDateTime() ? TextOperations::getFormattedDateTimeLocal(curItems[py]->campaign->getCreationDateTime()) : curItems[py]->date;
		}

		ENGINE->windows().createAndPushWindow<CMapOverview>(
			curItems[py]->name,
			curItems[py]->fullFileURI,
			creationDateTime,
			author,
			mapVersion,
			ResourcePath(curItems[py]->fileURI),
			tabType
		);
	}
	else
		CRClickPopup::createAndPush(curItems[py]->folderName);
}

auto SelectionTab::checkSubfolder(std::string path)
{
	struct Ret
	{
		std::string folderName;
		std::string baseFolder;
		bool parentExists;
		bool fileInFolder;
	} ret;

	ret.parentExists = (curFolder != "");
	ret.fileInFolder = false;

	std::vector<std::string> filetree;
	// delete first element (e.g. 'MAPS')
	boost::split(filetree, path, boost::is_any_of("/"));
	filetree.erase(filetree.begin());
	std::string pathWithoutPrefix = boost::algorithm::join(filetree, "/");

	if(!filetree.empty())
	{
		filetree.pop_back();
		ret.baseFolder = boost::algorithm::join(filetree, "/");
	}
	else
		ret.baseFolder = "";

	if(boost::algorithm::starts_with(ret.baseFolder, curFolder))
	{
		std::string folder = ret.baseFolder.substr(curFolder.size());

		if(folder != "")
		{
			boost::split(filetree, folder, boost::is_any_of("/"));
			ret.folderName = filetree[0];
		}
	}

	if(boost::algorithm::starts_with(pathWithoutPrefix, curFolder))
		if(boost::count(pathWithoutPrefix.substr(curFolder.size()), '/') == 0)
			ret.fileInFolder = true;

	return ret;
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter(int size, bool selectFirst)
{
	if(size == -1)
		size = currentMapSizeFilter;
	currentMapSizeFilter = size;

	curItems.clear();

	if(buttonDeleteMode)
		buttonDeleteMode->setEnabled(tabType != ESelectionScreen::newGame || showRandom);

	for(auto elem : allItems)
	{
		if((elem->mapHeader && (!size || elem->mapHeader->width == size)) || tabType == ESelectionScreen::campaignList)
		{
			if(showRandom)
				curFolder = "RandomMaps/";

			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(elem->originalFileURI);

			if((showRandom && baseFolder != "RandomMaps") || (!showRandom && baseFolder == "RandomMaps"))
				continue;

			if(parentExists && !showRandom)
			{
				auto folder = std::make_shared<ElementInfo>();
				folder->isFolder = true;
				folder->folderName = "..     (" + curFolder + ")";
				auto itemIt = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return boost::starts_with(e->folderName, ".."); });
				if (itemIt == curItems.end()) {
					curItems.push_back(folder);
				}			
			}

			auto folder = std::make_shared<ElementInfo>();
			folder->isFolder = true;
			folder->folderName = folderName;
			folder->isAutoSaveFolder = boost::starts_with(baseFolder, "Autosave/") && folderName != "Autosave";
			auto itemIt = boost::range::find_if(curItems, [folder](std::shared_ptr<ElementInfo> e) { return e->folderName == folder->folderName; });
			if (itemIt == curItems.end() && folderName != "") {
				curItems.push_back(folder);
			}

			if(fileInFolder)
				curItems.push_back(elem);
		}
	}

	if(curItems.size())
	{
		slider->block(false);
		slider->setAmount((int)curItems.size());
		sort();
		if(selectFirst)
		{
			int firstPos = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
			if(firstPos < curItems.size())
			{
				slider->scrollTo(firstPos);
				callOnSelect(curItems[firstPos]);
				selectAbs(firstPos);
			}
		}
	}
	else
	{
		updateListItems();
		redraw();
		slider->block(true);
		if(callOnSelect)
			callOnSelect(nullptr);
	}
}

void SelectionTab::sortBy(int criteria)
{
	if(criteria == sortingBy)
	{
		sortModeAscending = !sortModeAscending;
	}
	else
	{
		sortingBy = (ESortBy)criteria;
		sortModeAscending = true;
	}
	sort();

	selectAbs(-1);
}

void SelectionTab::sort()
{
	if(sortingBy != generalSortingBy)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(generalSortingBy));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	int firstMapIndex = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
	if(!sortModeAscending)
	{
		if(firstMapIndex)
		{
			auto startIt = std::next(curItems.begin(), boost::starts_with(curItems[0]->folderName, "..") ? 1 : 0);
			auto endIt = std::next(curItems.begin(), firstMapIndex - 1);
			if(startIt > endIt)
				std::swap(startIt, endIt);
			std::reverse(startIt, endIt);
		}
		std::reverse(std::next(curItems.begin(), firstMapIndex), curItems.end());
	}

	updateListItems();
	redraw();
}

void SelectionTab::select(int position)
{
	if(!curItems.size())
		return;

	// New selection. py is the index in curItems.
	int py = position + slider->getValue();
	vstd::amax(py, 0);
	vstd::amin(py, curItems.size() - 1);

	selectionPos = py;

	if(position < 0)
		slider->scrollBy(position);
	else if(position >= listItems.size())
		slider->scrollBy(position - (int)listItems.size() + 1);

	if(curItems[py]->isFolder) {
		if(boost::starts_with(curItems[py]->folderName, ".."))
		{
			std::vector<std::string> filetree;
			boost::split(filetree, curFolder, boost::is_any_of("/"));
			filetree.pop_back();
			filetree.pop_back();
			curFolder = filetree.size() > 0 ? boost::algorithm::join(filetree, "/") + "/" : "";
		}
		else
			curFolder += curItems[py]->folderName + "/";
		filter(-1);
		slider->scrollTo(0);

		int firstPos = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
		if(firstPos < curItems.size())
		{
			selectAbs(firstPos);
		}

		return;
	}

	rememberCurrentSelection();

	if(inputName && inputName->isActive())
	{
		auto filename = *CResourceHandler::get()->getResourceName(ResourcePath(curItems[py]->fileURI, EResType::SAVEGAME));
		inputName->setText(filename.stem().string());
	}

	updateListItems();
	redraw();
	if(callOnSelect)
		callOnSelect(curItems[py]);
}

void SelectionTab::selectAbs(int position)
{
	if(position == -1)
		position = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
	select(position - slider->getValue());
}

void SelectionTab::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	updateListItems();
	redraw();
}

void SelectionTab::updateListItems()
{
	// elemIdx is the index of the maps or saved game to display on line 0
	// slider->capacity contains the number of available screen lines
	// slider->positionsAmnt is the number of elements after filtering
	int elemIdx = slider->getValue();
	for(auto item : listItems)
	{
		if(elemIdx < curItems.size())
		{
			item->updateItem(curItems[elemIdx], elemIdx == selectionPos);
			elemIdx++;
		}
		else
		{
			item->updateItem();
		}
	}
}

bool SelectionTab::receiveEvent(const Point & position, int eventType) const
{
	// FIXME: widget should instead have well-defined pos so events will be filtered using standard routine
	return getLine(position - pos.topLeft()) != -1;
}

int SelectionTab::getLine() const
{
	Point clickPos = ENGINE->getCursorPosition() - pos.topLeft();
	return getLine(clickPos);
}

int SelectionTab::getLine(const Point & clickPos) const
{
	int line = -1;

	// Ignore clicks on save name area
	int maxPosY;
	if(tabType == ESelectionScreen::saveGame)
		maxPosY = 516;
	else
		maxPosY = 564;

	if(clickPos.y > 115 && clickPos.y < maxPosY && clickPos.x > 22 && clickPos.x < 371)
	{
		line = (clickPos.y - 115) / 25; //which line
	}

	return line;
}

void SelectionTab::selectFileName(std::string fname)
{
	boost::to_upper(fname);

	for(int i = (int)allItems.size() - 1; i >= 0; i--)
	{
		if(boost::to_upper_copy(allItems[i]->fileURI) == fname)
		{
			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(allItems[i]->originalFileURI);
			curFolder = baseFolder != "" ? baseFolder + "/" : "";
		}
	}

	filter(-1);

	for(int i = (int)curItems.size() - 1; i >= 0; i--)
	{
		if(boost::to_upper_copy(curItems[i]->fileURI) == fname)
		{
			slider->scrollTo(i);
			selectAbs(i);
			return;
		}
	}

	selectAbs(-1);

	if(tabType == ESelectionScreen::saveGame && inputName->getText().empty())
		inputName->setText(LIBRARY->generaltexth->translate("core.genrltxt.11"));
}

void SelectionTab::selectNewestFile()
{
	time_t newestTime = 0;
	std::string newestFile = "";
	for(int i = static_cast<int>(allItems.size()) - 1; i >= 0; i--)
		if(allItems[i]->lastWrite > newestTime)
		{
			newestTime = allItems[i]->lastWrite;
			newestFile = allItems[i]->fileURI;
		}
	selectFileName(newestFile);
}

std::shared_ptr<ElementInfo> SelectionTab::getSelectedMapInfo() const
{
	return curItems.empty() || curItems[selectionPos]->isFolder ? nullptr : curItems[selectionPos];
}

void SelectionTab::rememberCurrentSelection()
{
	if(getSelectedMapInfo()->isFolder)
		return;
		
	// TODO: this can be more elegant
	if(tabType == ESelectionScreen::newGame)
	{
		Settings lastMap = settings.write["general"]["lastMap"];
		lastMap->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::loadGame)
	{
		Settings lastSave = settings.write["general"]["lastSave"];
		lastSave->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::campaignList)
	{
		Settings lastCampaign = settings.write["general"]["lastCampaign"];
		lastCampaign->String() = getSelectedMapInfo()->fileURI;
	}
}

void SelectionTab::restoreLastSelection()
{
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		selectFileName(settings["general"]["lastMap"].String());
		break;
	case ESelectionScreen::campaignList:
		selectFileName(settings["general"]["lastCampaign"].String());
		break;
	case ESelectionScreen::loadGame:
		selectNewestFile();
		break;
	case ESelectionScreen::saveGame:
		selectFileName(settings["general"]["lastSave"].String());
	}
}

bool SelectionTab::isMapSupported(const CMapInfo & info)
{
	switch (info.mapHeader->version)
	{
		case EMapFormat::ROE:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA)["supported"].Bool();
		case EMapFormat::AB:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE)["supported"].Bool();
		case EMapFormat::SOD:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH)["supported"].Bool();
		case EMapFormat::CHR:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_CHRONICLES)["supported"].Bool();
		case EMapFormat::WOG:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS)["supported"].Bool();
		case EMapFormat::HOTA:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS)["supported"].Bool();
		case EMapFormat::VCMI:
			return LIBRARY->engineSettings()->getValue(EGameSettings::MAP_FORMAT_JSON_VCMI)["supported"].Bool();
	}
	return false;
}

void SelectionTab::parseMaps(const std::unordered_set<ResourcePath> & files)
{
	logGlobal->debug("Parsing %d maps", files.size());
	allItems.clear();
	for(auto & file : files)
	{
		try
		{
			auto mapInfo = std::make_shared<ElementInfo>();
			mapInfo->mapInit(file.getOriginalName());
			mapInfo->name = mapInfo->getNameForList();

			if (isMapSupported(*mapInfo))
				allItems.push_back(mapInfo);
		}
		catch(std::exception & e)
		{
			logGlobal->error("Map %s is invalid. Message: %s", file.getName(), e.what());
		}
	}
}

std::vector<ResourcePath> SelectionTab::parseSaves(const std::unordered_set<ResourcePath> & files)
{
	std::vector<ResourcePath> unsupported;

	for(auto & file : files)
	{
		try
		{
			auto mapInfo = std::make_shared<ElementInfo>();
			mapInfo->saveInit(file);
			mapInfo->name = mapInfo->getNameForList();

			// Filter out other game modes
			bool isCampaign = mapInfo->scenarioOptionsOfSave->mode == EStartMode::CAMPAIGN;
			bool isMultiplayer = mapInfo->amountOfHumanPlayersInSave > 1;
			bool isTutorial = boost::to_upper_copy(mapInfo->scenarioOptionsOfSave->mapname) == "MAPS/TUTORIAL";
			switch(GAME->server().getLoadMode())
			{
			case ELoadMode::SINGLE:
				if(isCampaign || isTutorial)
					mapInfo->mapHeader.reset();
				break;
			case ELoadMode::CAMPAIGN:
				if(!isCampaign)
					mapInfo->mapHeader.reset();
				break;
			case ELoadMode::TUTORIAL:
				if(!isTutorial)
					mapInfo->mapHeader.reset();
				break;
			case ELoadMode::MULTI:
				if(!isMultiplayer)
					mapInfo->mapHeader.reset();
				break;
			default:
				assert(0);
				mapInfo->mapHeader.reset();
				break;
			}

			allItems.push_back(mapInfo);
		}
		catch(const IdentifierResolutionException & e)
		{
			logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
		}
		catch(const std::exception & e)
		{
			unsupported.push_back(file); // IdentifierResolutionException is not relevant -> not ask to delete, when mods are disabled
			logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
		}
	}

	return unsupported;
}

void SelectionTab::handleUnsupportedSavegames(const std::vector<ResourcePath> & files)
{
	if(GAME->server().isHost() && files.size())
	{
		MetaString text = MetaString::createFromTextID("vcmi.lobby.deleteUnsupportedSave");
		text.replaceNumber(files.size());
		CInfoWindow::showYesNoDialog(text.toString(), std::vector<std::shared_ptr<CComponent>>(), [files](){
			for(auto & file : files)
			{
				LobbyDelete ld;
				ld.type = LobbyDelete::EType::SAVEGAME;
				ld.name = file.getName();
				GAME->server().sendLobbyPack(ld);
			}
		}, nullptr);
	}
}

void SelectionTab::parseCampaigns(const std::unordered_set<ResourcePath> & files)
{
	allItems.reserve(files.size());
	for(auto & file : files)
	{
		try
		{
			auto info = std::make_shared<ElementInfo>();
			info->fileURI = file.getOriginalName();
			info->campaignInit();
			info->name = info->getNameForList();

			if(info->campaign)
			{
				// skip campaigns organized in sets
				bool foundInSet = false;
				for (auto const & set : campaignSets.Struct())
					for (auto const & item : set.second["items"].Vector())
						if(file.getName() == ResourcePath(item["file"].String()).getName())
							foundInSet = true;

				if(!foundInSet || !enableUiEnhancements)
					allItems.push_back(info);
			}
		}
		catch(const std::exception & e)
		{
			logGlobal->error("Error: Failed to process campaign %s: %s", file.getName(), e.what());
		}
	}
}

std::unordered_set<ResourcePath> SelectionTab::getFiles(std::string dirURI, EResType resType)
{
	boost::to_upper(dirURI);
	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount)
	{
		return boost::algorithm::starts_with(mount, dirURI);
	});

	std::unordered_set<ResourcePath> ret = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == resType && boost::algorithm::starts_with(ident.getName(), dirURI);
	});

	return ret;
}

SelectionTab::ListItem::ListItem(Point position)
	: CIntObject(LCLICK, position)
{
	OBJECT_CONSTRUCTION;
	pictureEmptyLine = std::make_shared<CPicture>(ImagePath::builtin("camcust"), Rect(25, 121, 349, 26), -8, -14);
	labelName = std::make_shared<CLabel>(LABEL_POS_X, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "", 185);
	labelName->setAutoRedraw(false);
	labelAmountOfPlayers = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelAmountOfPlayers->setAutoRedraw(false);
	labelNumberOfCampaignMaps = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelNumberOfCampaignMaps->setAutoRedraw(false);
	labelMapSizeLetter = std::make_shared<CLabel>(41, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelMapSizeLetter->setAutoRedraw(false);
	// FIXME: This -12 should not be needed, but for some reason CAnimImage displaced otherwise
	iconFolder = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconFolder.png"), -8, -12);
	iconFormat = std::make_shared<CAnimImage>(AnimationPath::builtin("SCSELC.DEF"), 0, 0, 59, -12);
	iconVictoryCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRVICT.DEF"), 0, 0, 277, -12);
	iconLossCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRLOSS.DEF"), 0, 0, 310, -12);
}

void SelectionTab::ListItem::updateItem(std::shared_ptr<ElementInfo> info, bool selected)
{
	if(!info)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->disable();
		return;
	}

	auto color = selected ? Colors::YELLOW : Colors::WHITE;
	if(info->isFolder)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->enable();
		pictureEmptyLine->enable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->enable();
		labelName->setMaxWidth(316);
		if(info->isAutoSaveFolder) // align autosave folder left (starting timestamps in list should be in one line)
		{
			labelName->alignment = ETextAlignment::CENTERLEFT;
			labelName->moveTo(Point(pos.x + 80, labelName->pos.y));
		}
		else
		{
			labelName->alignment = ETextAlignment::CENTER;
			labelName->moveTo(Point(pos.x + LABEL_POS_X, labelName->pos.y));
		}
		labelName->setText(info->folderName);
		labelName->setColor(color);
		return;
	}

	labelName->enable();
	if(info->campaign)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->enable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info->campaign->scenariosCount();
		labelNumberOfCampaignMaps->setText(ostr.str());
		labelNumberOfCampaignMaps->setColor(color);
		labelName->setMaxWidth(316);
		labelName->alignment = ETextAlignment::CENTER;
		labelName->moveTo(Point(pos.x + LABEL_POS_X, labelName->pos.y));
	}
	else
	{
		labelNumberOfCampaignMaps->disable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info->amountOfPlayersOnMap << "/" << info->amountOfHumanControllablePlayers;
		labelAmountOfPlayers->enable();
		labelAmountOfPlayers->setText(ostr.str());
		labelAmountOfPlayers->setColor(color);
		labelMapSizeLetter->enable();
		labelMapSizeLetter->setText(info->getMapSizeName());
		labelMapSizeLetter->setColor(color);
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->enable();
		iconFormat->setFrame(info->getMapSizeFormatIconId());
		iconVictoryCondition->enable();
		iconVictoryCondition->setFrame(info->mapHeader->victoryIconIndex, 0);
		iconLossCondition->enable();
		iconLossCondition->setFrame(info->mapHeader->defeatIconIndex, 0);
		labelName->setMaxWidth(185);
		labelName->alignment = ETextAlignment::CENTER;
		labelName->moveTo(Point(pos.x + LABEL_POS_X, labelName->pos.y));
	}
	labelName->setText(info->name);
	labelName->setColor(color);
}

