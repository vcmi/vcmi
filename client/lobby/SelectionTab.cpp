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

#include <tbb/parallel_for.h>

#include "SelectionTab.h"
#include "CSelectionBase.h"
#include "CLobbyScreen.h"
#include "BattleOnlyModeTab.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../GameChatHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/InterfaceObjectConfigurable.h"
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
#include "render/CAnimation.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"
#include "../mainmenu/CCampaignScreen.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/SavegamePath.h"
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
#include "../../lib/json/JsonNode.h"

#include <widgets/GraphicalPrimitiveCanvas.h>

class ScenarioTabConfigurable : public InterfaceObjectConfigurable
{
public:
	explicit ScenarioTabConfigurable(SelectionTab & owner)
		: InterfaceObjectConfigurable()
	{
		addCallback("filterMapSize", [&owner](int filterIndex)
		{
			owner.filter(filterIndex, true);
		});

		build(JsonNode(JsonPath::builtin("config/widgets/scenarioTab.json")));
	}

	std::shared_ptr<CLabel> mapSizeFilterLabel() const
	{
		return widget<CLabel>("labelMapSizes");
	}

	std::shared_ptr<CToggleGroup> mapSizeFilterGroup() const
	{
		return widget<CToggleGroup>("groupMapSizeFilters");
	}

	void setMapSizeLabelVisible(bool visible) const
	{
		if(auto label = mapSizeFilterLabel())
		{
			if(visible)
				label->enable();
			else
				label->disable();
		}
	}
};

bool mapSorter::operator()(const std::shared_ptr<ElementInfo> aaa, const std::shared_ptr<ElementInfo> bbb)
{
	if(aaa->isFolder || bbb->isFolder)
	{
		if(aaa->isFolder != bbb->isFolder)
			return (aaa->isFolder > bbb->isFolder);
		else
		{
			const bool aParent = boost::algorithm::starts_with(aaa->folderName, "..");
			const bool bParent = boost::algorithm::starts_with(bbb->folderName, "..");
			if(aParent != bParent)
				return aParent;
			return TextOperations::compareLocalizedStrings(aaa->folderName, bbb->folderName);
		}
	}

	if(aaa->entry && bbb->entry) //if we are sorting scenarios
	{
		const ScenarioListEntry & a = *aaa->entry;
		const ScenarioListEntry & b = *bbb->entry;
		switch(sortBy)
		{
		case _format: //by map format (RoE, WoG, etc)
			return (a.getMapFormat() < b.getMapFormat());
		case _loscon: //by loss conditions
			return (a.getDefeatIcon() < b.getDefeatIcon());
		case _playerAm: //by player amount
			if(a.getPlayers() != b.getPlayers())
				return (a.getPlayers() < b.getPlayers());
			else
				return (a.getHumanPlayers() < b.getHumanPlayers());
		case _size: //by size of map
			return (a.getMapWidth() < b.getMapWidth());
		case _viccon: //by victory conditions
			return (a.getVictoryIcon() < b.getVictoryIcon());
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
			return aaa->campaignInfo->campaign->scenariosCount() < bbb->campaignInfo->campaign->scenariosCount();
		case _name: //by name
			return TextOperations::compareLocalizedStrings(aaa->campaignInfo->campaign->getNameTranslated(&GAME->translator()), bbb->campaignInfo->campaign->getNameTranslated(&GAME->translator()));
		default:
			return TextOperations::compareLocalizedStrings(aaa->campaignInfo->campaign->getNameTranslated(&GAME->translator()), bbb->campaignInfo->campaign->getNameTranslated(&GAME->translator()));
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
	, positionsToShow(0)
	, enableUiEnhancements(settings["general"]["enableUiEnhancements"].Bool())
	, campaignSets(JsonUtils::assembleFromFiles("config/campaignSets.json"))
{
	OBJECT_CONSTRUCTION;
		
	generalSortingBy = getSortBySelectionScreen(tabType);
	sortingBy = _format;

	if(tabType != ESelectionScreen::campaignList)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("SCSELBCK.bmp"), ENGINE->isRoeData() ? 36 : 0, 6);
		pos = background->pos;
		inputName = std::make_shared<CTextInput>(inputNameRect, Point(-32, -25), ImagePath::builtin("GSSTRIP.bmp"));
		inputName->setFilterFilename();

		scenarioTabConfigurable = std::make_shared<ScenarioTabConfigurable>(*this);
		addChild(scenarioTabConfigurable.get(), false);
		if(ENGINE->isRoeData())
		{
			if(auto group = scenarioTabConfigurable->mapSizeFilterGroup())
				group->moveBy(Point(-36, 0));
			if(auto label = scenarioTabConfigurable->mapSizeFilterLabel())
				label->moveBy(Point(-18, 0));
		}
		scenarioTabConfigurable->setMapSizeLabelVisible(!CResourceHandler::get()->existsResource(AnimationPath::builtin("SCGTBUT.DEF")));

		std::array xpos = {23, 55, 88, 121, 306, 339};
		if(ENGINE->isRoeData())
			xpos = {20, 52, 0, 85, 270, 303};
		constexpr std::array sortIconNames = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		constexpr std::array sortShortcuts = { EShortcut::MAPS_SORT_PLAYERS, EShortcut::MAPS_SORT_SIZE, EShortcut::MAPS_SORT_FORMAT, EShortcut::MAPS_SORT_NAME, EShortcut::MAPS_SORT_VICTORY, EShortcut::MAPS_SORT_DEFEAT };
		for(int i = 0; i < 6; i++)
		{
			if(ENGINE->isRoeData() && i == 2)
				continue;

			ESortBy criteria = (ESortBy)i;
			if(criteria == _name)
				criteria = generalSortingBy;

			buttonsSortBy.push_back(std::make_shared<CButton>(Point(xpos[i], 86), AnimationPath::builtin(sortIconNames[i]), LIBRARY->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria), sortShortcuts[i]));
		}
	}

	positionsToShow = enableUiEnhancements ? 17 : 18;
	std::string tabTitle;
	std::string tabTitleDelete;
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		tabTitle = "{" + LIBRARY->generaltexth->translate("core.arraytxt.229") + "}";
		tabTitleDelete = "{red|" + LIBRARY->generaltexth->translate("vcmi.lobby.deleteMapTitle") + "}";
		break;
	case ESelectionScreen::loadGame:
		tabTitle = "{" + LIBRARY->generaltexth->translate("core.arraytxt.230") + "}";
		tabTitleDelete = "{red|" + LIBRARY->generaltexth->translate("vcmi.lobby.deleteSaveGameTitle") + "}";
		break;
	case ESelectionScreen::saveGame:
		positionsToShow = enableUiEnhancements ? 15 : 16;
		tabTitle = "{" + LIBRARY->generaltexth->translate("core.arraytxt.231") + "}";
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
		auto sortByDate = std::make_shared<CButton>(Point(371 - (ENGINE->isRoeData() ? 36 : 0), 85), AnimationPath::builtin("selectionTabSortDate"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.sortDate")), std::bind(&SelectionTab::sortBy, this, ESortBy::_changeDate), EShortcut::MAPS_SORT_CHANGEDATE);
		sortByDate->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/selectionTabSortDate")));
		buttonsSortBy.push_back(sortByDate);

		Rect searchWidgetArea(22 - (ENGINE->isRoeData() ? 3 : 0), 115, 366 - (ENGINE->isRoeData() ? 33 : 0), 26);
		Rect searchTextInputArea(87, 115, 301 - (ENGINE->isRoeData() ? 36 : 0), 26);

		searchWidgetBackground = std::make_shared<FilledTexturePlayerColored>(searchWidgetArea);
		searchWidgetBackground->setPlayerColor(PlayerColor(1));

		searchBoxLabel = std::make_shared<CLabel>(55, 128, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, "Filter:");
		searchInputRectangle = std::make_shared<TransparentFilledRectangle>(searchTextInputArea, ColorRGBA(0, 0, 0, 128));

		searchInput = std::make_shared<CTextInput>(searchTextInputArea, FONT_SMALL, ETextAlignment::CENTER, true);
		searchInput->setCallback([this](const std::string & text) {
									 std::shared_ptr<ElementInfo> selectedMap = getSelectedElement();
									 bool hideSelectedElement = !selectedMap || !checkNameFilter(selectedMap->name);
									 filter(-1, hideSelectedElement);
									 if (!hideSelectedElement)
									 {
										 auto it = find(curItems.begin(), curItems.end(), selectedMap);
										 if (it != curItems.end())
											 selectAbs(it - curItems.begin());
									 }
								 });


		if(tabType == ESelectionScreen::loadGame || tabType == ESelectionScreen::newGame)
		{
			buttonDeleteMode = std::make_shared<CButton>(Point(367 - (ENGINE->isRoeData() ? 36 : 0), 18), AnimationPath::builtin("lobby/deleteButton"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.deleteMode")), [this, tabTitle, tabTitleDelete](){
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
		listItems.push_back(std::make_shared<ListItem>(Point(30, (enableUiEnhancements ? 154 : 129) + i * 25)));

	labelTabTitle = std::make_shared<CLabel>(205 - (ENGINE->isRoeData() ? 18 : 0), 28, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, tabTitle);
	slider = std::make_shared<CSlider>(Point(372 - (ENGINE->isRoeData() ? 36 : 0), 86 + (enableUiEnhancements ? 54 : 0)), (tabType != ESelectionScreen::saveGame ? 480 : 430) - (enableUiEnhancements ? 54 : 0), std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, (int)curItems.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(24);
	slider->setInertiaEnabled(true);

	// create scroll bounds that encompass all area in this UI element to the left of slider (including area of slider itself)
	// entire screen can't be used in here since map description might also have slider
	slider->setScrollBounds(Rect(pos.x - slider->pos.x, 0, slider->pos.x + slider->pos.w - pos.x, slider->pos.h ));
	filter(0);
}

void SelectionTab::toggleMode()
{
	allItems.clear();
	curItems.clear();
	resourceFiles.clear();
	listLoaded = false;
	const bool restoreSelection = curFolder.empty();
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
		case ESelectionScreen::loadGame:
			inputName->disable();
			break;

		case ESelectionScreen::saveGame:
			inputName->enable();
			inputName->activate();
			if(!restoreSelection)
				inputName->setText(LIBRARY->generaltexth->translate("core.genrltxt.11"));
			break;

		case ESelectionScreen::campaignList:
			resourceFiles = getFiles("Maps/", EResType::CAMPAIGN);
			break;

		default:
			assert(0);
			break;
		}

		if(tabType == ESelectionScreen::loadGame && restoreSelection)
			requestLastSave();
		else
			openFolder(curFolder, [this, restoreSelection](){ finishToggleMode(restoreSelection); });
	}
	slider->setAmount((int)curItems.size());
	updateListItems();
	redraw();
}

void SelectionTab::finishToggleMode(bool restoreSelection)
{
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
	else if(restoreSelection)
	{
		restoreLastSelection();
	}

	slider->setAmount((int)curItems.size());
	updateListItems();
	redraw();
}

void SelectionTab::openFolder(const std::string & folder, const std::function<void()> & onOpened)
{
	if(tabType == ESelectionScreen::campaignList)
	{
		curFolder = folder;
		parseCurrentFolder();
		listLoaded = true;
		onOpened();
		return;
	}

	LobbyRequestScenarioList request;
	request.folder = folder;
	sendListRequest(request, [onOpened](const LobbyScenarioList &){ onOpened(); });
}

void SelectionTab::requestLastSave()
{
	LobbyRequestScenarioList request;
	request.selectLastSave = true;
	sendListRequest(request, [this](const LobbyScenarioList & pack)
	{
		filter(0);
		if(pack.selectedFile.empty() || !selectFileInCurrentFolder(pack.selectedFile))
		{
			selectionPos = curItems.size();
			notifySelection(nullptr);
		}
	});
}

ScenarioListEntry::EType SelectionTab::getListType() const
{
	return tabType == ESelectionScreen::newGame ? ScenarioListEntry::EType::MAP : ScenarioListEntry::EType::SAVE;
}

void SelectionTab::sendAndAwaitList(const CPackForLobby & pack, std::function<void(const LobbyScenarioList &)> onReceived)
{
	assert(!onListReceived);
	onListReceived = std::move(onReceived);
	parent->setInputEnabled(false);
	GAME->server().sendLobbyPack(pack);
}

void SelectionTab::sendListRequest(LobbyRequestScenarioList & request, std::function<void(const LobbyScenarioList &)> onReceived)
{
	request.type = getListType();
	request.loadMode = GAME->server().getLoadMode();
	sendAndAwaitList(request, std::move(onReceived));
}

void SelectionTab::sendDelete(const LobbyDelete & pack)
{
	const auto selected = getSelectedElement();
	const std::string selectedFile = selected ? selected->fileURI : "";
	sendAndAwaitList(pack, [this, selectedFile](const LobbyScenarioList &)
	{
		filter(-1);
		if(selectedFile.empty() || !selectFileInCurrentFolder(selectedFile))
			selectFirstFile();
	});
}

void SelectionTab::onScenarioListReceived(const LobbyScenarioList & pack)
{
	// reply to a screen that was closed before the reply came
	if(!onListReceived || pack.list.type != getListType())
		return;

	auto callback = std::move(onListReceived);
	onListReceived = nullptr;
	parent->setInputEnabled(true);

	setScenarioList(pack.list);
	listLoaded = true;
	callback(pack);
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
				CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.lobby.deleteFile") + "\n\n" + getFullFileURI(*curItems[py]), std::vector<std::shared_ptr<CComponent>>(), [this, py](){
					LobbyDelete ld;
					ld.type = tabType == ESelectionScreen::newGame ? LobbyDelete::EType::RANDOMMAP : LobbyDelete::EType::SAVEGAME;
					ld.names.push_back(curItems[py]->fileURI);
					sendDelete(ld);
				}, nullptr);
			else
				CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.lobby.deleteFolder") + "\n\n" + curFolder + curItems[py]->folderName, std::vector<std::shared_ptr<CComponent>>(), [this, py](){
					LobbyDelete ld;
					ld.type = LobbyDelete::EType::SAVEGAME_FOLDER;
					ld.names.push_back(curFolder + curItems[py]->folderName);
					sendDelete(ld);
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
	auto selectedItem = getSelectedElement();

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
		const std::string fileDateTime = TextOperations::getFormattedDateTimeLocal(curItems[py]->lastWrite);
		std::string authorContact;
		if(tabType != ESelectionScreen::campaignList)
		{
			const ScenarioListEntry & entry = *curItems[py]->entry;
			author = entry.getMapAuthor(&GAME->translator());
			authorContact = entry.getMapAuthorContact(&GAME->translator());
			mapVersion = entry.getMapVersion(&GAME->translator());
			creationDateTime = tabType == ESelectionScreen::newGame && entry.getMapCreationTime() ? TextOperations::getFormattedDateTimeLocal(entry.getMapCreationTime()) : fileDateTime;
		}
		else
		{
			const auto & campaign = curItems[py]->campaignInfo->campaign;
			author = campaign->getAuthor(&GAME->translator());
			authorContact = campaign->getAuthorContact(&GAME->translator());
			mapVersion = campaign->getCampaignVersion(&GAME->translator());
			creationDateTime = campaign->getCreationDateTime() ? TextOperations::getFormattedDateTimeLocal(campaign->getCreationDateTime()) : fileDateTime;
		}
		if(!authorContact.empty())
			author += " <" + authorContact + ">";

		ENGINE->windows().createAndPushWindow<CMapOverview>(
			curItems[py]->name,
			getFullFileURI(*curItems[py]),
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
		if(std::ranges::count(pathWithoutPrefix.substr(curFolder.size()), '/') == 0)
			ret.fileInFolder = true;

	return ret;
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter(int size, size_t requiredHumanPlayersCount, bool selectFirst)
{
	setRequiredHumanPlayers(requiredHumanPlayersCount);
	filter(size, selectFirst);
}

void SelectionTab::filter(int size, bool selectFirst)
{
	if(size == -1)
		size = currentMapSizeFilter;
	currentMapSizeFilter = size;

	curItems.clear();

	if(buttonDeleteMode)
		buttonDeleteMode->setEnabled(tabType != ESelectionScreen::newGame || showRandom);

	hiddenIncompatibleMapsCount = 0;

	for(auto elem : allItems)
	{
		if(elem->isFolder)
		{
			curItems.push_back(elem);
			continue;
		}

		if((elem->entry && (!size || elem->entry->getMapWidth() == size)) || tabType == ESelectionScreen::campaignList)
		{
			if(!isMapCompatibleWithLobbyPlayerCount(*elem))
			{
				++hiddenIncompatibleMapsCount;
				continue;
			}

			if(!enableUiEnhancements || checkNameFilter(elem->name))
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
			int firstPos = std::ranges::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
			if(firstPos < curItems.size())
			{
				slider->scrollTo(firstPos);
				selectAbs(firstPos);
			}
		}
	}
	else
	{
		updateListItems();
		redraw();
		slider->block(true);
		notifySelection(nullptr);
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

	int firstMapIndex = std::ranges::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
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

	if(curItems[py]->isFolder)
	{
		std::string folder;
		if(boost::starts_with(curItems[py]->folderName, ".."))
		{
			std::vector<std::string> filetree;
			boost::split(filetree, curFolder, boost::is_any_of("/"));
			filetree.pop_back();
			filetree.pop_back();
			folder = filetree.size() > 0 ? boost::algorithm::join(filetree, "/") + "/" : "";
		}
		else
			folder = curFolder + curItems[py]->folderName + "/";

		openFolder(folder, [this]()
		{
			filter(-1);
			slider->scrollTo(0);
			selectNewestFile(tabType == ESelectionScreen::saveGame);
		});
		return;
	}

	rememberCurrentSelection();

	if(inputName && inputName->isActive())
		inputName->setText(curItems[py]->name);

	if (curItems.size() <= positionsToShow)
		slider->scrollToMin();

	updateListItems();
	redraw();
	notifySelection(curItems[py]);
}

void SelectionTab::selectAbs(int position)
{
	if(position == -1)
		position = std::ranges::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return !e->isFolder; }) - curItems.begin();
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

	int startingPos = enableUiEnhancements ? 140 : 115;
	if(clickPos.y > startingPos && clickPos.y < maxPosY && clickPos.x > 22 && clickPos.x < 371)
	{
		line = (clickPos.y - startingPos) / 25; //which line
	}

	return line;
}

void SelectionTab::selectFileName(const std::string & fname)
{
	const std::string folder = ScenarioList::getFolderOf(fname);

	// Keep scenario selection on the root list: random maps are accessed via dedicated UI path.
	if(tabType == ESelectionScreen::newGame && boost::algorithm::iequals(folder, "RandomMaps/"))
		folder.clear();

	if(boost::algorithm::iequals(folder, curFolder))
	{
		if(!selectFileInCurrentFolder(fname))
			selectFirstFile();
		return;
	}

	openFolder(folder, [this, fname]()
	{
		if(selectFileInCurrentFolder(fname))
			return;

		openFolder("", [this]()
		{
			filter(-1);
			selectFirstFile();
		});
	});
}

bool SelectionTab::selectFileInCurrentFolder(const std::string & fname)
{
	filter(-1);

	for(int i = (int)curItems.size() - 1; i >= 0; i--)
	{
		if(!curItems[i]->isFolder && boost::algorithm::iequals(curItems[i]->fileURI, fname))
		{
			slider->scrollTo(i);
			selectAbs(i);
			return true;
		}
	}
	return false;
}

void SelectionTab::selectFirstFile()
{
	int firstPos = std::ranges::find_if(curItems, [this](const std::shared_ptr<ElementInfo> & element)
	{
		return !element->isFolder
			&& (tabType != ESelectionScreen::saveGame || !SavegamePath::isAutosaveName(element->fileURI));
	}) - curItems.begin();
	if(firstPos < curItems.size())
	{
		slider->scrollTo(firstPos);
		selectAbs(firstPos);
	}
	else
	{
		selectionPos = curItems.size();
		notifySelection(nullptr);
	}

	if(tabType == ESelectionScreen::saveGame && inputName->getText().empty())
		inputName->setText(LIBRARY->generaltexth->translate("core.genrltxt.11"));
}

void SelectionTab::selectNewestFile(bool skipAutosaves)
{
	time_t newestTime = 0;
	std::string newestFile = "";
	for(const auto & item : curItems)
	{
		if(item->isFolder)
			continue;
		if(skipAutosaves && SavegamePath::isAutosaveName(item->fileURI))
			continue;

		if(item->lastWrite > newestTime || (item->lastWrite == newestTime && item->fileURI > newestFile))
		{
			newestTime = item->lastWrite;
			newestFile = item->fileURI;
		}
	}

	if(newestFile.empty() || !selectFileInCurrentFolder(newestFile))
	{
		selectionPos = curItems.size();
		notifySelection(nullptr);
	}
}

std::shared_ptr<ElementInfo> SelectionTab::getSelectedElement() const
{
	return selectionPos >= curItems.size() || curItems[selectionPos]->isFolder ? nullptr : curItems[selectionPos];
}

ScenarioSelection SelectionTab::getSelection() const
{
	ScenarioSelection selection;
	const auto element = getSelectedElement();
	if(!element)
		return selection;

	selection.fileURI = element->fileURI;
	if(element->campaignInfo)
		selection.type = ScenarioSelection::EType::CAMPAIGN;
	else if(element->entry->type == ScenarioListEntry::EType::MAP)
		selection.type = ScenarioSelection::EType::MAP;
	else
		selection.type = ScenarioSelection::EType::SAVE;
	return selection;
}

void SelectionTab::notifySelection(const std::shared_ptr<ElementInfo> & element)
{
	if(callOnSelect)
		callOnSelect(element);
}

void SelectionTab::rememberCurrentSelection()
{
	const auto selectedMapInfo = getSelectedElement();
	if(!selectedMapInfo)
		return;
		
	// TODO: this can be more elegant
	if(tabType == ESelectionScreen::newGame)
	{
		Settings lastMap = settings.write["general"]["lastMap"];
		lastMap->String() = selectedMapInfo->fileURI;
	}
	else if(tabType == ESelectionScreen::campaignList)
	{
		Settings lastCampaign = settings.write["general"]["lastCampaign"];
		lastCampaign->String() = selectedMapInfo->fileURI;
	}
}

void SelectionTab::restoreLastSelection()
{
	// lobby state can arrive before the list or while a list is requested
	if(!listLoaded || onListReceived)
		return;

	switch(tabType)
	{
	case ESelectionScreen::newGame:
		selectFileName(settings["general"]["lastMap"].String());
		break;
	case ESelectionScreen::campaignList:
		selectFileName(settings["general"]["lastCampaign"].String());
		break;
	case ESelectionScreen::loadGame:
		requestLastSave();
		break;
	case ESelectionScreen::saveGame:
		selectNewestFile(true);
	}
}

bool SelectionTab::checkNameFilter(const std::string & fullstring) const
{
	std::string filter = searchInput->getText();
	return std::search(fullstring.begin(), fullstring.end(), filter.begin(), filter.end(),
					   [](char a, char b) -> bool {return std::tolower(a) == std::tolower(b);}) != fullstring.end();
}

static std::string getMapSizeName(int width)
{
	switch(width)
	{
	case CMapHeader::MAP_SIZE_SMALL:
		return "S";
	case CMapHeader::MAP_SIZE_MIDDLE:
		return "M";
	case CMapHeader::MAP_SIZE_LARGE:
		return "L";
	case CMapHeader::MAP_SIZE_XLARGE:
		return "XL";
	case CMapHeader::MAP_SIZE_HUGE:
		return "H";
	case CMapHeader::MAP_SIZE_XHUGE:
		return "XH";
	case CMapHeader::MAP_SIZE_GIANT:
		return "G";
	default:
		return "C";
	}
}

bool SelectionTab::isMapCompatibleWithLobbyPlayerCount(const ElementInfo & info) const
{
	if(tabType != ESelectionScreen::newGame || GAME->server().loadMode != ELoadMode::MULTI || !info.entry)
		return true;

	return info.entry->getHumanPlayers() >= getRequiredHumanPlayers();
}

size_t SelectionTab::getRequiredHumanPlayers() const
{
	return requiredHumanPlayers;
}

void SelectionTab::setRequiredHumanPlayers(size_t players)
{
	requiredHumanPlayers = players;
}

size_t SelectionTab::getHiddenIncompatibleMapsCount() const
{
	return hiddenIncompatibleMapsCount;
}

void SelectionTab::parseCurrentFolder()
{
	allItems.clear();
	std::unordered_set<ResourcePath> currentFiles;
	std::map<std::string, std::time_t> folderLastWrite;

	if(!curFolder.empty())
	{
		auto parentFolder = std::make_shared<ElementInfo>();
		parentFolder->isFolder = true;
		parentFolder->folderName = "..     (" + curFolder + ")";
		allItems.push_back(parentFolder);
	}

	for(const auto & file : resourceFiles)
	{
		auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(file.getOriginalName());
		if(!folderName.empty())
			folderLastWrite[folderName] = std::max(folderLastWrite[folderName], CResourceHandler::get()->getLastWriteTime(file));
		if(fileInFolder)
			currentFiles.insert(file);
	}

	parseCampaigns(currentFiles);

	for(const auto & [folderName, lastWrite] : folderLastWrite)
	{
		auto folder = std::make_shared<ElementInfo>();
		folder->isFolder = true;
		folder->folderName = folderName;
		folder->lastWrite = lastWrite;
		allItems.push_back(folder);
	}
}

void SelectionTab::setScenarioList(const ScenarioList & list)
{
	allItems.clear();
	curFolder = list.folder;

	if(!curFolder.empty() && !showRandom)
	{
		auto parentFolder = std::make_shared<ElementInfo>();
		parentFolder->isFolder = true;
		parentFolder->folderName = "..     (" + curFolder + ")";
		allItems.push_back(parentFolder);
	}

	const size_t offset = allItems.size();
	for(const auto & entry : list.entries)
	{
		auto element = std::make_shared<ElementInfo>();
		element->entry = entry;
		element->fileURI = entry.fileURI;
		element->lastWrite = entry.lastWrite;
		allItems.push_back(element);
	}
	installTexts(offset);

	for(const auto & subfolder : list.subfolders)
	{
		if(tabType == ESelectionScreen::newGame && subfolder.name == "RandomMaps")
			continue;
		if(subfolder.compatible == false)
			continue;

		auto folder = std::make_shared<ElementInfo>();
		folder->isFolder = true;
		folder->folderName = subfolder.name;
		folder->isAutoSaveFolder = boost::starts_with(curFolder, "Autosave/");
		folder->lastWrite = subfolder.lastWrite;
		allItems.push_back(folder);
	}

	if(tabType == ESelectionScreen::loadGame)
		handleUnsupportedSavegames(list.unsupportedSaves);
}

void SelectionTab::handleUnsupportedSavegames(const std::vector<std::string> & files)
{
	if(GAME->server().isHost() && files.size())
	{
		MetaString text = MetaString::createFromTextID("vcmi.lobby.deleteUnsupportedSave");
		text.replaceNumber(files.size());
		CInfoWindow::showYesNoDialog(text.toString(&GAME->translator()), std::vector<std::shared_ptr<CComponent>>(), [this, files](){
			LobbyDelete ld;
			ld.type = LobbyDelete::EType::SAVEGAME;
			ld.names = files;
			sendDelete(ld);
		}, nullptr);
	}
}

void SelectionTab::parseCampaigns(const std::unordered_set<ResourcePath> & files)
{
	auto timeStart = std::chrono::steady_clock::now();

	std::vector<ResourcePath> filesVector(files.begin(), files.end());

	// every entry is written by a single thread only, entries of skipped campaigns stay empty
	size_t offset = allItems.size();
	allItems.resize(offset + filesVector.size());

	tbb::parallel_for(tbb::blocked_range<size_t>(0, filesVector.size()), [this, &filesVector, offset](const tbb::blocked_range<size_t> & r)
	{
		for(auto i = r.begin(); i != r.end(); i++)
		{
			const auto & file = filesVector[i];
			try
			{
				auto info = std::make_shared<ElementInfo>();
				info->campaignInfo = std::make_shared<CMapInfo>();
				info->campaignInfo->fileURI = file.getOriginalName();
				info->campaignInfo->campaignInit();
				info->fileURI = info->campaignInfo->fileURI;
				info->lastWrite = info->campaignInfo->lastWrite;

				if(info->campaignInfo->campaign)
				{
					// skip campaigns organized in sets
					bool foundInSet = false;
					for (auto const & set : campaignSets.Struct())
						for (auto const & item : set.second["items"].Vector())
							if(file.getName() == ResourcePath(item["file"].String()).getName())
								foundInSet = true;

					if(!foundInSet || !enableUiEnhancements)
						allItems[offset + i] = info;
				}
			}
			catch(const std::exception & e)
			{
				logGlobal->error("Error: Failed to process campaign %s: %s", file.getName(), e.what());
			}
		}
	});

	allItems.erase(std::remove(allItems.begin() + offset, allItems.end(), nullptr), allItems.end());
	installTexts(offset);

	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStart);
	logGlobal->debug("Parsing %d campaigns took %d ms", filesVector.size(), timeElapsed.count());
}

std::string SelectionTab::getFullFileURI(const ElementInfo & item) const
{
	EResType resType = EResType::SAVEGAME;
	if(tabType == ESelectionScreen::newGame)
		resType = EResType::MAP;
	if(tabType == ESelectionScreen::campaignList)
		resType = EResType::CAMPAIGN;

	const ResourcePath resource(item.fileURI, resType);

	return CResourceHandler::get()->getFullFileURI(resource);
}

void SelectionTab::installTexts(size_t offset)
{
	// campaign texts are inert data, so every listed campaign has to be installed before its
	// name or description can be rendered. Keys are namespaced per campaign, so the whole
	// set can stay installed while the picker is open. Maps and saves use texts of their metadata
	for(size_t i = offset; i < allItems.size(); ++i)
	{
		const auto & item = allItems[i];
		if(item->campaignInfo)
		{
			item->textOverlays.emplace_back(item->campaignInfo->campaign->getTexts());
			item->name = item->campaignInfo->getNameTranslated(&GAME->translator());
		}
		else if(item->entry->type == ScenarioListEntry::EType::SAVE)
		{
			std::vector<std::string> path;
			boost::split(path, item->entry->originalFileURI, boost::is_any_of("\\/"));
			item->name = path.back();
		}
		else
		{
			item->name = item->entry->getMapName(&GAME->translator());
			if(item->name.empty())
				item->name = LIBRARY->generaltexth->allTexts[508];
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
	if(!ENGINE->isDemoData())
		pictureEmptyLine = std::make_shared<CPicture>(ImagePath::builtin("camcust"), Rect(25, 121, 349, 26), -8, -14);
	labelName = std::make_shared<CLabel>(LABEL_POS_X - (ENGINE->isRoeData() ? 36 : 0), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "", 185);
	labelName->setAutoRedraw(false);
	labelAmountOfPlayers = std::make_shared<CLabel>(8 - (ENGINE->isRoeData() ? 3 : 0), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelAmountOfPlayers->setAutoRedraw(false);
	labelNumberOfCampaignMaps = std::make_shared<CLabel>(8 - (ENGINE->isRoeData() ? 3 : 0), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelNumberOfCampaignMaps->setAutoRedraw(false);
	labelMapSizeLetter = std::make_shared<CLabel>(41 - (ENGINE->isRoeData() ? 3 : 0), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelMapSizeLetter->setAutoRedraw(false);
	// FIXME: This -12 should not be needed, but for some reason CAnimImage displaced otherwise
	iconFolder = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconFolder.png"), -8, -12);
	if(!ENGINE->isRoeData())
		iconFormat = std::make_shared<CAnimImage>(AnimationPath::builtin("SCSELC.DEF"), 0, 0, 59, -12);
	iconVictoryCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRVICT.DEF"), 0, 0, 277 - (ENGINE->isRoeData() ? 36 : 0), -12);
	iconLossCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRLOSS.DEF"), 0, 0, 310 - (ENGINE->isRoeData() ? 36 : 0), -12);
}

void SelectionTab::ListItem::updateItem(std::shared_ptr<ElementInfo> info, bool selected)
{
	if(!info)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		if(pictureEmptyLine)
			pictureEmptyLine->disable();
		if(iconFormat)
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
		if(pictureEmptyLine)
			pictureEmptyLine->enable();
		if(iconFormat)
			iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->enable();
		labelName->setMaxWidth(316);
		if(info->isAutoSaveFolder) // align autosave folder left (starting timestamps in list should be in one line)
		{
			labelName->alignment = ETextAlignment::CENTERLEFT;
			labelName->moveTo(Point(pos.x + 80 - (ENGINE->isRoeData() ? 22 : 0), labelName->pos.y));
		}
		else
		{
			labelName->alignment = ETextAlignment::CENTER;
			labelName->moveTo(Point(pos.x + LABEL_POS_X - (ENGINE->isRoeData() ? 22 : 0), labelName->pos.y));
		}
		labelName->setText(info->folderName);
		labelName->setColor(color);
		return;
	}

	labelName->enable();
	if(info->campaignInfo)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		if(pictureEmptyLine)
			pictureEmptyLine->disable();
		if(iconFormat)
			iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->enable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info->campaignInfo->campaign->scenariosCount();
		labelNumberOfCampaignMaps->setText(ostr.str());
		labelNumberOfCampaignMaps->setColor(color);
		labelName->setMaxWidth(316);
		labelName->alignment = ETextAlignment::CENTER;
		labelName->moveTo(Point(pos.x + LABEL_POS_X - (ENGINE->isRoeData() ? 36 : 0), labelName->pos.y));
	}
	else
	{
		labelNumberOfCampaignMaps->disable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info->entry->getPlayers() << "/" << info->entry->getHumanPlayers();
		labelAmountOfPlayers->enable();
		labelAmountOfPlayers->setText(ostr.str());
		labelAmountOfPlayers->setColor(color);
		labelMapSizeLetter->enable();
		labelMapSizeLetter->setText(getMapSizeName(info->entry->getMapWidth()));
		labelMapSizeLetter->setColor(color);
		iconFolder->disable();
		if(pictureEmptyLine)
			pictureEmptyLine->disable();
		if(iconFormat)
		{
			iconFormat->enable();
			iconFormat->setFrame(info->entry->getMapFormatSettings()["iconIndex"].Integer());
		}
		iconVictoryCondition->enable();
		iconVictoryCondition->setFrame(info->entry->getVictoryIcon(), 0);
		iconLossCondition->enable();
		iconLossCondition->setFrame(info->entry->getDefeatIcon(), 0);
		labelName->setMaxWidth(185);
		labelName->alignment = ETextAlignment::CENTER;
		labelName->moveTo(Point(pos.x + LABEL_POS_X - (ENGINE->isRoeData() ? 36 : 0), labelName->pos.y));
	}
	labelName->setText(info->name);
	labelName->setColor(color);
}
