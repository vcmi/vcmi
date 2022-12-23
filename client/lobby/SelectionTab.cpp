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
#include "../CMessage.h"
#include "../CBitmapHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"

#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/serializer/Connection.h"


bool mapSorter::operator()(const std::shared_ptr<CMapInfo> aaa, const std::shared_ptr<CMapInfo> bbb)
{
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
			return (a->defeatMessage < b->defeatMessage);
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
			return (a->victoryMessage < b->victoryMessage);
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
			return CGI->generaltexth->campaignRegionNames[aaa->campaignHeader->mapVersion].size() <
				   CGI->generaltexth->campaignRegionNames[bbb->campaignHeader->mapVersion].size();
			break;
		case _name: //by name
			return boost::ilexicographical_compare(aaa->campaignHeader->name, bbb->campaignHeader->name);
		default:
			return boost::ilexicographical_compare(aaa->campaignHeader->name, bbb->campaignHeader->name);
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
	: CIntObject(LCLICK | WHEEL | KEYBOARD | DOUBLECLICK), callOnSelect(nullptr), tabType(Type), selectionPos(0), sortModeAscending(true), inputNameRect{32, 539, 350, 20}
{
	OBJ_CONSTRUCTION;

	generalSortingBy = getSortBySelectionScreen(tabType);

	if(tabType != ESelectionScreen::campaignList)
	{
		sortingBy = _format;
		background = std::make_shared<CPicture>("SCSELBCK.bmp", 0, 6);
		pos = background->pos;
		inputName = std::make_shared<CTextInput>(inputNameRect, Point(-32, -25), "GSSTRIP.bmp", 0);
		inputName->filters += CTextInput::filenameFilter;
		labelMapSizes = std::make_shared<CLabel>(87, 62, FONT_SMALL, EAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[510]);

		int sizes[] = {36, 72, 108, 144, 0};
		const char * filterIconNmes[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		for(int i = 0; i < 5; i++)
			buttonsSortBy.push_back(std::make_shared<CButton>(Point(158 + 47 * i, 46), filterIconNmes[i], CGI->generaltexth->zelp[54 + i], std::bind(&SelectionTab::filter, this, sizes[i], true)));

		int xpos[] = {23, 55, 88, 121, 306, 339};
		const char * sortIconNames[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		for(int i = 0; i < 6; i++)
		{
			ESortBy criteria = (ESortBy)i;
			if(criteria == _name)
				criteria = generalSortingBy;

			buttonsSortBy.push_back(std::make_shared<CButton>(Point(xpos[i], 86), sortIconNames[i], CGI->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria)));
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
		type |= REDRAW_PARENT; // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3;
		pos.y += 6;

		buttonsSortBy.push_back(std::make_shared<CButton>(Point(23, 86), "CamCusM.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps)));
		buttonsSortBy.push_back(std::make_shared<CButton>(Point(55, 86), "CamCusL.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name)));
		break;
	default:
		assert(0);
		break;
	}

	iconsMapFormats = std::make_shared<CAnimation>("SCSELC.DEF");
	iconsMapFormats->preload();
	iconsVictoryCondition = std::make_shared<CAnimation>("SCNRVICT.DEF");
	iconsVictoryCondition->preload();
	iconsLossCondition = std::make_shared<CAnimation>("SCNRLOSS.DEF");
	iconsLossCondition->preload();
	for(int i = 0; i < positionsToShow; i++)
		listItems.push_back(std::make_shared<ListItem>(Point(30, 129 + i * 25), iconsMapFormats, iconsVictoryCondition, iconsLossCondition));

	labelTabTitle = std::make_shared<CLabel>(205, 28, FONT_MEDIUM, EAlignment::CENTER, Colors::YELLOW, tabTitle);
	slider = std::make_shared<CSlider>(Point(372, 86), tabType != ESelectionScreen::saveGame ? 480 : 430, std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, (int)curItems.size(), 0, false, CSlider::BLUE);
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
			inputName->disable();
			parseMaps(getFiles("Maps/", EResType::MAP));
			break;

		case ESelectionScreen::loadGame:
			inputName->disable();
			parseSaves(getFiles("Saves/", EResType::CLIENT_SAVEGAME));
			break;

		case ESelectionScreen::saveGame:
			parseSaves(getFiles("Saves/", EResType::CLIENT_SAVEGAME));
			inputName->enable();
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

void SelectionTab::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		int line = getLine();

		if(line != -1)
		{
			select(line);
		}
#ifdef VCMI_IOS
        // focus input field if clicked inside it
        else if(inputName && inputName->active && inputNameRect.isIn(GH.current->button.x, GH.current->button.y))
            inputName->giveFocus();
#endif
	}
}

void SelectionTab::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED)
		return;

	int moveBy = 0;
	switch(key.keysym.sym)
	{
	case SDLK_UP:
		moveBy = -1;
		break;
	case SDLK_DOWN:
		moveBy = +1;
		break;
	case SDLK_PAGEUP:
		moveBy = -(int)listItems.size() + 1;
		break;
	case SDLK_PAGEDOWN:
		moveBy = +(int)listItems.size() - 1;
		break;
	case SDLK_HOME:
		select(-slider->getValue());
		return;
	case SDLK_END:
		select((int)curItems.size() - slider->getValue());
		return;
	default:
		return;
	}
	select((int)selectionPos - slider->getValue() + moveBy);
}

void SelectionTab::onDoubleClick()
{
	if(getLine() != -1) //double clicked scenarios list
	{
		(static_cast<CLobbyScreen *>(parent))->buttonStart->clickLeft(false, true);
	}
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter(int size, bool selectFirst)
{
	curItems.clear();

	if(tabType == ESelectionScreen::campaignList)
	{
		for(auto elem : allItems)
			curItems.push_back(elem);
	}
	else
	{
		for(auto elem : allItems)
		{
			if(elem->mapHeader && elem->mapHeader->version && (!size || elem->mapHeader->width == size))
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
			slider->moveTo(0);
			callOnSelect(curItems[0]);
			selectAbs(0);
		}
	}
	else
	{
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

	selectAbs(0);
}

void SelectionTab::sort()
{
	if(sortingBy != generalSortingBy)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(generalSortingBy));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	if(!sortModeAscending)
		std::reverse(curItems.begin(), curItems.end());

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
		slider->moveBy(position);
	else if(position >= listItems.size())
		slider->moveBy(position - (int)listItems.size() + 1);

	rememberCurrentSelection();

	if(inputName && inputName->active)
	{
		auto filename = *CResourceHandler::get("local")->getResourceName(ResourceID(curItems[py]->fileURI, EResType::CLIENT_SAVEGAME));
		inputName->setText(filename.stem().string());
	}
	updateListItems();
	if(callOnSelect)
		callOnSelect(curItems[py]);
}

void SelectionTab::selectAbs(int position)
{
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

int SelectionTab::getLine()
{
	int line = -1;
	Point clickPos(GH.current->button.x, GH.current->button.y);
	clickPos = clickPos - pos.topLeft();

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
	for(int i = (int)curItems.size() - 1; i >= 0; i--)
	{
		if(curItems[i]->fileURI == fname)
		{
			slider->moveTo(i);
			selectAbs(i);
			return;
		}
	}

	selectAbs(0);
}

std::shared_ptr<CMapInfo> SelectionTab::getSelectedMapInfo() const
{
	return curItems.empty() ? nullptr : curItems[selectionPos];
}

void SelectionTab::rememberCurrentSelection()
{
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

void SelectionTab::parseMaps(const std::unordered_set<ResourceID> & files)
{
	logGlobal->debug("Parsing %d maps", files.size());
	allItems.clear();
	for(auto & file : files)
	{
		try
		{
			auto mapInfo = std::make_shared<CMapInfo>();
			mapInfo->mapInit(file.getName());

			// ignore unsupported map versions (e.g. WoG maps without WoG)
			// but accept VCMI maps
			if((mapInfo->mapHeader->version >= EMapFormat::VCMI) || (mapInfo->mapHeader->version <= CGI->modh->settings.data["textData"]["mapVersion"].Float()))
				allItems.push_back(mapInfo);
		}
		catch(std::exception & e)
		{
			logGlobal->error("Map %s is invalid. Message: %s", file.getName(), e.what());
		}
	}
}

void SelectionTab::parseSaves(const std::unordered_set<ResourceID> & files)
{
	for(auto & file : files)
	{
		try
		{
			auto mapInfo = std::make_shared<CMapInfo>();
			mapInfo->saveInit(file);

			// Filter out other game modes
			bool isCampaign = mapInfo->scenarioOptionsOfSave->mode == StartInfo::CAMPAIGN;
			bool isMultiplayer = mapInfo->amountOfHumanPlayersInSave > 1;
			switch(CSH->getLoadMode())
			{
			case ELoadMode::SINGLE:
				if(isMultiplayer || isCampaign)
					mapInfo->mapHeader.reset();
				break;
			case ELoadMode::CAMPAIGN:
				if(!isCampaign)
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

void SelectionTab::parseCampaigns(const std::unordered_set<ResourceID> & files)
{
	allItems.reserve(files.size());
	for(auto & file : files)
	{
		auto info = std::make_shared<CMapInfo>();
		//allItems[i].date = std::asctime(std::localtime(&files[i].date));
		info->fileURI = file.getName();
		info->campaignInit();
		allItems.push_back(info);
	}
}

std::unordered_set<ResourceID> SelectionTab::getFiles(std::string dirURI, int resType)
{
	boost::to_upper(dirURI);
	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount)
	{
		return boost::algorithm::starts_with(mount, dirURI);
	});

	std::unordered_set<ResourceID> ret = CResourceHandler::get()->getFilteredFiles([&](const ResourceID & ident)
	{
		return ident.getType() == resType && boost::algorithm::starts_with(ident.getName(), dirURI);
	});

	return ret;
}

SelectionTab::ListItem::ListItem(Point position, std::shared_ptr<CAnimation> iconsFormats, std::shared_ptr<CAnimation> iconsVictory, std::shared_ptr<CAnimation> iconsLoss)
	: CIntObject(LCLICK, position)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	labelName = std::make_shared<CLabel>(184, 0, FONT_SMALL, EAlignment::CENTER, Colors::WHITE);
	labelName->setAutoRedraw(false);
	labelAmountOfPlayers = std::make_shared<CLabel>(8, 0, FONT_SMALL, EAlignment::CENTER, Colors::WHITE);
	labelAmountOfPlayers->setAutoRedraw(false);
	labelNumberOfCampaignMaps = std::make_shared<CLabel>(8, 0, FONT_SMALL, EAlignment::CENTER, Colors::WHITE);
	labelNumberOfCampaignMaps->setAutoRedraw(false);
	labelMapSizeLetter = std::make_shared<CLabel>(41, 0, FONT_SMALL, EAlignment::CENTER, Colors::WHITE);
	labelMapSizeLetter->setAutoRedraw(false);
	// FIXME: This -12 should not be needed, but for some reason CAnimImage displaced otherwise
	iconFormat = std::make_shared<CAnimImage>(iconsFormats, 0, 0, 59, -12);
	iconVictoryCondition = std::make_shared<CAnimImage>(iconsVictory, 0, 0, 277, -12);
	iconLossCondition = std::make_shared<CAnimImage>(iconsLoss, 0, 0, 310, -12);
}

void SelectionTab::ListItem::updateItem(std::shared_ptr<CMapInfo> info, bool selected)
{
	if(!info)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->disable();
		return;
	}

	auto color = selected ? Colors::YELLOW : Colors::WHITE;
	if(info->campaignHeader)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->enable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << CGI->generaltexth->campaignRegionNames[info->campaignHeader->mapVersion].size();
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
		iconFormat->enable();
		iconFormat->setFrame(info->getMapSizeFormatIconId().first, info->getMapSizeFormatIconId().second);
		iconVictoryCondition->enable();
		iconVictoryCondition->setFrame(info->mapHeader->victoryIconIndex, 0);
		iconLossCondition->enable();
		iconLossCondition->setFrame(info->mapHeader->defeatIconIndex, 0);
	}
	labelName->enable();
	labelName->setText(info->getNameForList());
	labelName->setColor(color);
}
