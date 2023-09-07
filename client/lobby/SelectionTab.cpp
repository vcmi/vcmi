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

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"

#include "../../CCallback.h"

#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/serializer/Connection.h"

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
			return boost::ilexicographical_compare(aaa->folderName, bbb->folderName);
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
			int playerAmntB, humenPlayersB, playerAmntA, humenPlayersA;
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
			return boost::ilexicographical_compare(a->name, b->name);
		case _fileName: //by filename
			return boost::ilexicographical_compare(aaa->fileURI, bbb->fileURI);
		default:
			return boost::ilexicographical_compare(a->name, b->name);
		}
	}
	else //if we are sorting campaigns
	{
		switch(sortBy)
		{
		case _numOfMaps: //by number of maps in campaign
			return aaa->campaign->scenariosCount() < bbb->campaign->scenariosCount();
		case _name: //by name
			return boost::ilexicographical_compare(aaa->campaign->getName(), bbb->campaign->getName());
		default:
			return boost::ilexicographical_compare(aaa->campaign->getName(), bbb->campaign->getName());
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
	: CIntObject(LCLICK | SHOW_POPUP | KEYBOARD | DOUBLECLICK), callOnSelect(nullptr), tabType(Type), selectionPos(0), sortModeAscending(true), inputNameRect{32, 539, 350, 20}, curFolder(""), currentMapSizeFilter(0)
{
	OBJ_CONSTRUCTION;

	generalSortingBy = getSortBySelectionScreen(tabType);

	if(tabType != ESelectionScreen::campaignList)
	{
		sortingBy = _format;
		background = std::make_shared<CPicture>(ImagePath::builtin("SCSELBCK.bmp"), 0, 6);
		pos = background->pos;
		inputName = std::make_shared<CTextInput>(inputNameRect, Point(-32, -25), ImagePath::builtin("GSSTRIP.bmp"), 0);
		inputName->filters += CTextInput::filenameFilter;
		labelMapSizes = std::make_shared<CLabel>(87, 62, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[510]);

		int sizes[] = {36, 72, 108, 144, 0};
		const char * filterIconNmes[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		for(int i = 0; i < 5; i++)
			buttonsSortBy.push_back(std::make_shared<CButton>(Point(158 + 47 * i, 46), AnimationPath::builtin(filterIconNmes[i]), CGI->generaltexth->zelp[54 + i], std::bind(&SelectionTab::filter, this, sizes[i], true)));

		int xpos[] = {23, 55, 88, 121, 306, 339};
		const char * sortIconNames[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		for(int i = 0; i < 6; i++)
		{
			ESortBy criteria = (ESortBy)i;
			if(criteria == _name)
				criteria = generalSortingBy;

			buttonsSortBy.push_back(std::make_shared<CButton>(Point(xpos[i], 86), AnimationPath::builtin(sortIconNames[i]), CGI->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria)));
		}
	}

	int positionsToShow = 18;
	std::string tabTitle;
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		tabTitle = CGI->generaltexth->arraytxt[229];
		break;
	case ESelectionScreen::loadGame:
		tabTitle = CGI->generaltexth->arraytxt[230];
		break;
	case ESelectionScreen::saveGame:
		positionsToShow = 16;
		tabTitle = CGI->generaltexth->arraytxt[231];
		break;
	case ESelectionScreen::campaignList:
		tabTitle = CGI->generaltexth->allTexts[726];
		setRedrawParent(true); // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3;
		pos.y += 6;

		buttonsSortBy.push_back(std::make_shared<CButton>(Point(23, 86), AnimationPath::builtin("CamCusM.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps)));
		buttonsSortBy.push_back(std::make_shared<CButton>(Point(55, 86), AnimationPath::builtin("CamCusL.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name)));
		break;
	default:
		assert(0);
		break;
	}

	iconsMapFormats = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCSELC.DEF"));
	iconsVictoryCondition = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCNRVICT.DEF"));
	iconsLossCondition = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCNRLOSS.DEF"));
	for(int i = 0; i < positionsToShow; i++)
		listItems.push_back(std::make_shared<ListItem>(Point(30, 129 + i * 25), iconsMapFormats, iconsVictoryCondition, iconsLossCondition));

	labelTabTitle = std::make_shared<CLabel>(205, 28, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, tabTitle);
	slider = std::make_shared<CSlider>(Point(372, 86), tabType != ESelectionScreen::saveGame ? 480 : 430, std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, (int)curItems.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(24);

	// create scroll bounds that encompass all area in this UI element to the left of slider (including area of slider itself)
	// entire screen can't be used in here since map description might also have slider
	slider->setScrollBounds(Rect(pos.x - slider->pos.x, 0, slider->pos.x + slider->pos.w - pos.x, slider->pos.h ));
	filter(0);
}

void SelectionTab::toggleMode()
{
	if(CSH->isGuest())
	{
		allItems.clear();
		curItems.clear();
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
				parseMaps(files);
				break;
			}

		case ESelectionScreen::loadGame:
			inputName->disable();
			parseSaves(getFiles("Saves/", EResType::SAVEGAME));
			break;

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

		if(CSH->campaignStateToSend)
		{
			CSH->setCampaignState(CSH->campaignStateToSend);
			CSH->campaignStateToSend.reset();
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

	if(line != -1)
	{
		select(line);
	}
#ifdef VCMI_IOS
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

	if(itemIndex >= 0 && curItems[itemIndex]->isFolder)
		return;

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
		std::string text = boost::str(boost::format("{%1%}\r\n\r\n%2%:\r\n%3%") % curItems[py]->getName() % CGI->generaltexth->translate("vcmi.lobby.filename") % curItems[py]->fullFileURI);
		if(curItems[py]->date != "")
			text += boost::str(boost::format("\r\n\r\n%1%:\r\n%2%") % CGI->generaltexth->translate("vcmi.lobby.creationDate") % curItems[py]->date);

		GH.windows().createAndPushWindow<CMapInfoTooltipBox>(text, ResourcePath(curItems[py]->fileURI), tabType);
	}
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

	for(auto elem : allItems)
	{
		if((elem->mapHeader && (!size || elem->mapHeader->width == size)) || tabType == ESelectionScreen::campaignList)
		{
			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(elem->originalFileURI);

			if(parentExists)
			{
				auto folder = std::make_shared<ElementInfo>();
				folder->isFolder = true;
				folder->folderName = "..     (" + curFolder + ")";
				auto itemIt = boost::range::find_if(curItems, [](std::shared_ptr<ElementInfo> e) { return boost::starts_with(e->folderName, ".."); });
				if (itemIt == curItems.end()) {
					curItems.push_back(folder);
				}			
			}

			std::shared_ptr<ElementInfo> folder = std::make_shared<ElementInfo>();
			folder->isFolder = true;
			folder->folderName = folderName;
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
		std::reverse(std::next(curItems.begin(), firstMapIndex), curItems.end());

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
	Point clickPos = GH.getCursorPosition() - pos.topLeft();
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
		if(allItems[i]->fileURI == fname)
		{
			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(allItems[i]->originalFileURI);
			curFolder = baseFolder != "" ? baseFolder + "/" : "";
		}
	}

	for(int i = (int)curItems.size() - 1; i >= 0; i--)
	{
		if(curItems[i]->fileURI == fname)
		{
			slider->scrollTo(i);
			selectAbs(i);
			return;
		}
	}

	filter(-1);
	selectAbs(-1);
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
	case ESelectionScreen::saveGame:
		selectFileName(settings["general"]["lastSave"].String());
	}
}

bool SelectionTab::isMapSupported(const CMapInfo & info)
{
	switch (info.mapHeader->version)
	{
		case EMapFormat::ROE:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA)["supported"].Bool();
		case EMapFormat::AB:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE)["supported"].Bool();
		case EMapFormat::SOD:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH)["supported"].Bool();
		case EMapFormat::WOG:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS)["supported"].Bool();
		case EMapFormat::HOTA:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS)["supported"].Bool();
		case EMapFormat::VCMI:
			return CGI->settings()->getValue(EGameSettings::MAP_FORMAT_JSON_VCMI)["supported"].Bool();
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
			mapInfo->mapInit(file.getName());

			if (isMapSupported(*mapInfo))
				allItems.push_back(mapInfo);
		}
		catch(std::exception & e)
		{
			logGlobal->error("Map %s is invalid. Message: %s", file.getName(), e.what());
		}
	}
}

void SelectionTab::parseSaves(const std::unordered_set<ResourcePath> & files)
{
	for(auto & file : files)
	{
		try
		{
			auto mapInfo = std::make_shared<ElementInfo>();
			mapInfo->saveInit(file);

			// Filter out other game modes
			bool isCampaign = mapInfo->scenarioOptionsOfSave->mode == StartInfo::CAMPAIGN;
			bool isMultiplayer = mapInfo->amountOfHumanPlayersInSave > 1;
			bool isTutorial = boost::to_upper_copy(mapInfo->scenarioOptionsOfSave->mapname) == "MAPS/TUTORIAL";
			switch(CSH->getLoadMode())
			{
			case ELoadMode::SINGLE:
				if(isMultiplayer || isCampaign || isTutorial)
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
			default:
				if(!isMultiplayer)
					mapInfo->mapHeader.reset();
				break;
			}

			allItems.push_back(mapInfo);
		}
		catch(const std::exception & e)
		{
			logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
		}
	}
}

void SelectionTab::parseCampaigns(const std::unordered_set<ResourcePath> & files)
{
	allItems.reserve(files.size());
	for(auto & file : files)
	{
		auto info = std::make_shared<ElementInfo>();
		//allItems[i].date = std::asctime(std::localtime(&files[i].date));
		info->fileURI = file.getName();
		info->campaignInit();
		if(info->campaign)
			allItems.push_back(info);
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

SelectionTab::CMapInfoTooltipBox::CMapInfoTooltipBox(std::string text, ResourcePath resource, ESelectionScreen tabType)
	: CWindowObject(BORDERED | RCLICK_POPUP)
{
	drawPlayerElements = tabType == ESelectionScreen::newGame;
	renderImage = tabType == ESelectionScreen::newGame && settings["lobby"]["mapPreview"].Bool();

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	std::vector<std::shared_ptr<IImage>> mapLayerImages;
	if(renderImage)
		mapLayerImages = createMinimaps(ResourcePath(resource.getName(), EResType::MAP), IMAGE_SIZE);

	if(mapLayerImages.size() == 0)
		renderImage = false;

	pos = Rect(0, 0, 3 * BORDER + 2 * IMAGE_SIZE, 2000);

	auto drawLabel = [&]() {
		label = std::make_shared<CTextBox>(text, Rect(BORDER, BORDER, BORDER + 2 * IMAGE_SIZE, 350), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
		if(!label->slider)
			label->resize(Point(BORDER + 2 * IMAGE_SIZE, label->label->textSize.y));
	};
	drawLabel();

	int textHeight = std::min(350, label->label->textSize.y);
	pos.h = BORDER + textHeight + BORDER;
	if(renderImage)
		pos.h += IMAGE_SIZE + BORDER;
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();

	drawLabel();

	if(renderImage)
	{
		if(mapLayerImages.size() == 1)
			image1 = std::make_shared<CPicture>(mapLayerImages[0], Point(BORDER + (BORDER + IMAGE_SIZE) / 2, textHeight + 2 * BORDER));
		else
		{
			image1 = std::make_shared<CPicture>(mapLayerImages[0], Point(BORDER, textHeight + 2 * BORDER));
			image2 = std::make_shared<CPicture>(mapLayerImages[1], Point(BORDER + IMAGE_SIZE + BORDER, textHeight + 2 * BORDER));
		}
	}

	center(GH.getCursorPosition()); //center on mouse
#ifdef VCMI_MOBILE
	moveBy({0, -pos.h / 2});
#endif
	fitToScreen(10);
}

Canvas SelectionTab::CMapInfoTooltipBox::createMinimapForLayer(std::unique_ptr<CMap> & map, int layer)
{
	Canvas canvas = Canvas(Point(map->width, map->height));

	for (int y = 0; y < map->height; ++y)
		for (int x = 0; x < map->width; ++x)
		{
			TerrainTile & tile = map->getTile(int3(x, y, layer));

			ColorRGBA color = tile.terType->minimapUnblocked;
			if (tile.blocked && (!tile.visitable))
				color = tile.terType->minimapBlocked;

			if(drawPlayerElements)
				// if object at tile is owned - it will be colored as its owner
				for (const CGObjectInstance *obj : tile.blockingObjects)
				{
					PlayerColor player = obj->getOwner();
					if(player == PlayerColor::NEUTRAL)
					{
						color = graphics->neutralColor;
						break;
					}
					if (player.isValidPlayer())
					{
						color = graphics->playerColors[player.getNum()];
						break;
					}
				}

			canvas.drawPoint(Point(x, y), color);
		}
	
	return canvas;
}

std::vector<std::shared_ptr<IImage>> SelectionTab::CMapInfoTooltipBox::createMinimaps(ResourcePath resource, int size)
{
	std::vector<std::shared_ptr<IImage>> ret = std::vector<std::shared_ptr<IImage>>();

	CMapService mapService;
	std::unique_ptr<CMap> map;
	try
	{
		map = mapService.loadMap(resource);
	}
	catch (...)
	{
		return ret;
	}

	for(int i = 0; i < (map->twoLevel ? 2 : 1); i++)
	{
		Canvas canvas = createMinimapForLayer(map, i);
		Canvas canvasScaled = Canvas(Point(size, size));
		canvasScaled.drawScaled(canvas, Point(0, 0), Point(size, size));
		std::shared_ptr<IImage> img = GH.renderHandler().createImage(canvasScaled.getInternalSurface());
		
		ret.push_back(img);
	}

	return ret;
}

SelectionTab::ListItem::ListItem(Point position, std::shared_ptr<CAnimation> iconsFormats, std::shared_ptr<CAnimation> iconsVictory, std::shared_ptr<CAnimation> iconsLoss)
	: CIntObject(LCLICK, position)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pictureEmptyLine = std::make_shared<CPicture>(GH.renderHandler().loadImage(ImagePath::builtin("camcust")), Rect(25, 121, 349, 26), -8, -14);
	labelName = std::make_shared<CLabel>(184, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelName->setAutoRedraw(false);
	labelAmountOfPlayers = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelAmountOfPlayers->setAutoRedraw(false);
	labelNumberOfCampaignMaps = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelNumberOfCampaignMaps->setAutoRedraw(false);
	labelMapSizeLetter = std::make_shared<CLabel>(41, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelMapSizeLetter->setAutoRedraw(false);
	// FIXME: This -12 should not be needed, but for some reason CAnimImage displaced otherwise
	iconFolder = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconFolder.png"), -8, -12);
	iconFormat = std::make_shared<CAnimImage>(iconsFormats, 0, 0, 59, -12);
	iconVictoryCondition = std::make_shared<CAnimImage>(iconsVictory, 0, 0, 277, -12);
	iconLossCondition = std::make_shared<CAnimImage>(iconsLoss, 0, 0, 310, -12);
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
		labelName->setText(info->folderName);
		labelName->setColor(color);
		return;
	}

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
	}
	labelName->enable();
	labelName->setText(info->getNameForList());
	labelName->setColor(color);
}
