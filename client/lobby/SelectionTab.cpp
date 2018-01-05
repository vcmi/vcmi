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
#include "CLobbyScreen.h"


#include "../CGameInfo.h"

#include "../../CCallback.h"

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

#include "../../lib/NetPacks.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/serializer/Connection.h"
#include "../../lib/CModHandler.h"

// CGCreature
#include "../lib/CCreatureHandler.h"


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

SelectionTab::SelectionTab(ESelectionScreen Type)
	: background(nullptr), onSelect(nullptr)
{
	OBJ_CONSTRUCTION;
	selectionPos = 0;
	addUsedEvents(LCLICK | WHEEL | KEYBOARD | DOUBLECLICK);
	slider = nullptr;
	txt = nullptr;
	tabType = Type;

	if(Type == ESelectionScreen::campaignList)
	{
		background = nullptr; //use background from parent
		type |= REDRAW_PARENT; // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3;
		pos.y += 6;

		//sort by buttons
		new CButton(Point(23, 86), "CamCusM.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps)); //by num of maps
		new CButton(Point(55, 86), "CamCusL.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name)); //by name
	}
	else
	{
		background = new CPicture("SCSELBCK.bmp", 0, 6);
		pos = background->pos;
		//size filter buttons
		{
			int sizes[] = {36, 72, 108, 144, 0};
			const char * names[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
			for(int i = 0; i < 5; i++)
				new CButton(Point(158 + 47 * i, 46), names[i], CGI->generaltexth->zelp[54 + i], std::bind(&SelectionTab::filter, this, sizes[i], true));
		}

		//sort buttons buttons
		{
			int xpos[] = {23, 55, 88, 121, 306, 339};
			const char * names[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
			for(int i = 0; i < 6; i++)
			{
				ESortBy criteria = (ESortBy)i;

				if(criteria == _name)
					criteria = generalSortingBy;

				new CButton(Point(xpos[i], 86), names[i], CGI->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria));
			}
		}
	}

	generalSortingBy = (tabType == ESelectionScreen::loadGame || tabType == ESelectionScreen::saveGame) ? _fileName : _name;

	slider = new CSlider(Point(372, 86), tabType != ESelectionScreen::saveGame ? 480 : 430, std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, curItems.size(), 0, false, CSlider::BLUE);
	slider->addUsedEvents(WHEEL);

	formatIcons = std::make_shared<CAnimation>("SCSELC.DEF");
	formatIcons->load();

	sortingBy = _format;
	ascending = true;
	filter(0);
	//select(0);
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
			parseMaps(getFiles("Maps/", EResType::MAP));
			positionsToShow = 18;
			break;

		case ESelectionScreen::loadGame:
			parseSaves(getFiles("Saves/", EResType::CLIENT_SAVEGAME));
			positionsToShow = 18;
			break;

		case ESelectionScreen::saveGame:
			parseSaves(getFiles("Saves/", EResType::CLIENT_SAVEGAME));
			positionsToShow = 16;
			txt = new CTextInput(Rect(32, 539, 350, 20), Point(-32, -25), "GSSTRIP.bmp", 0);
			txt->filters += CTextInput::filenameFilter;
			break;

		case ESelectionScreen::campaignList:
			parseCampaigns(getFiles("Maps/", EResType::CAMPAIGN));
			positionsToShow = 18;
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
	}
	slider->setAmount(curItems.size());

	redraw();
}

SelectionTab::~SelectionTab()
{
	formatIcons->unload();
}

void SelectionTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printMaps(to);

	std::string title;
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		title = CGI->generaltexth->arraytxt[229];
		break;
	case ESelectionScreen::loadGame:
		title = CGI->generaltexth->arraytxt[230];
		break;
	case ESelectionScreen::saveGame:
		title = CGI->generaltexth->arraytxt[231];
		break;
	case ESelectionScreen::campaignList:
		title = CGI->generaltexth->allTexts[726];
		break;
	}

	printAtMiddleLoc(title, 205, 28, FONT_MEDIUM, Colors::YELLOW, to); //Select a Scenario to Play
	if(tabType != ESelectionScreen::campaignList)
	{
		printAtMiddleLoc(CGI->generaltexth->allTexts[510], 87, 62, FONT_SMALL, Colors::YELLOW, to); //Map sizes
	}
}

void SelectionTab::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		int line = getLine();
		if(line != -1)
			select(line);
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
		moveBy = -positionsToShow + 1;
		break;
	case SDLK_PAGEDOWN:
		moveBy = +positionsToShow - 1;
		break;
	case SDLK_HOME:
		select(-slider->getValue());
		return;
	case SDLK_END:
		select(curItems.size() - slider->getValue());
		return;
	default:
		return;
	}
	select(selectionPos - slider->getValue() + moveBy);
}

void SelectionTab::onDoubleClick()
{
	if(getLine() != -1) //double clicked scenarios list
	{
		//act as if start button was pressed
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
		slider->setAmount(curItems.size());
		sort();
		if(selectFirst)
		{
			slider->moveTo(0);
			onSelect(curItems[0]);
			selectAbs(0);
		}
	}
	else
	{
		slider->block(true);
		if(onSelect) // MPTODO: Find why host doesn't crash on it
			onSelect(nullptr);
	}
}

void SelectionTab::sortBy(int criteria)
{
	if(criteria == sortingBy)
	{
		ascending = !ascending;
	}
	else
	{
		sortingBy = (ESortBy)criteria;
		ascending = true;
	}
	sort();

	selectAbs(0);
}

void SelectionTab::sort()
{
	if(sortingBy != generalSortingBy)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(generalSortingBy));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	if(!ascending)
		std::reverse(curItems.begin(), curItems.end());

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
	else if(position >= positionsToShow)
		slider->moveBy(position - positionsToShow + 1);

	rememberCurrentSelection();

	if(txt)
	{
		auto filename = *CResourceHandler::get("local")->getResourceName(ResourceID(curItems[py]->fileURI, EResType::CLIENT_SAVEGAME));
		txt->setText(filename.stem().string());
	}

	if(onSelect)
		onSelect(curItems[py]);
}

void SelectionTab::selectAbs(int position)
{
	select(position - slider->getValue());
}

void SelectionTab::sliderMove(int slidPos)
{
	if(!slider)
		return; //ignore spurious call when slider is being created
	redraw();
}


// Display the tab with the scenario names
//
// elemIdx is the index of the maps or saved game to display on line 0
// slider->capacity contains the number of available screen lines
// slider->positionsAmnt is the number of elements after filtering
void SelectionTab::printMaps(SDL_Surface * to)
{

	int elemIdx = slider->getValue();

	// Display all elements if there's enough space
	//if(slider->amount < slider->capacity)
	//	elemIdx = 0;


	SDL_Color itemColor;
	for(int line = 0; line < positionsToShow && elemIdx < curItems.size(); elemIdx++, line++)
	{
		auto currentItem = curItems[elemIdx];

		if(elemIdx == selectionPos)
			itemColor = Colors::YELLOW;
		else
			itemColor = Colors::WHITE;

		if(tabType != ESelectionScreen::campaignList)
		{
			//amount of players
			std::ostringstream ostr(std::ostringstream::out);
			ostr << currentItem->playerAmnt << "/" << currentItem->humanPlayers;
			printAtLoc(ostr.str(), 29, 120 + line * 25, FONT_SMALL, itemColor, to);

			//map size
			std::string temp2 = "C";
			switch(currentItem->mapHeader->width)
			{
			case 36:
				temp2 = "S";
				break;
			case 72:
				temp2 = "M";
				break;
			case 108:
				temp2 = "L";
				break;
			case 144:
				temp2 = "XL";
				break;
			}
			printAtMiddleLoc(temp2, 70, 128 + line * 25, FONT_SMALL, itemColor, to);

			int frame = -1, group = 0;
			switch(currentItem->mapHeader->version)
			{
			case EMapFormat::ROE:
				frame = 0;
				break;
			case EMapFormat::AB:
				frame = 1;
				break;
			case EMapFormat::SOD:
				frame = 2;
				break;
			case EMapFormat::WOG:
				frame = 3;
				break;
			case EMapFormat::VCMI:
				frame = 0;
				group = 1;
				break;
			default:
				// Unknown version. Be safe and ignore that map
				logGlobal->warn("Warning: %s has wrong version!", currentItem->fileURI);
				continue;
			}
			IImage * icon = formatIcons->getImage(frame, group);
			if(icon)
			{
				icon->draw(to, pos.x + 88, pos.y + 117 + line * 25);
				icon->decreaseRef();
			}

			//victory conditions
			icon = CMM->victoryIcons->getImage(currentItem->mapHeader->victoryIconIndex, 0);
			if(icon)
			{
				icon->draw(to, pos.x + 306, pos.y + 117 + line * 25);
				icon->decreaseRef();
			}
			//loss conditions
			icon = CMM->lossIcons->getImage(currentItem->mapHeader->defeatIconIndex, 0);
			if(icon)
			{
				icon->draw(to, pos.x + 339, pos.y + 117 + line * 25);
				icon->decreaseRef();
			}
		}
		else //if campaign
		{
			//number of maps in campaign
			std::ostringstream ostr(std::ostringstream::out);
			ostr << CGI->generaltexth->campaignRegionNames[currentItem->campaignHeader->mapVersion].size();
			printAtLoc(ostr.str(), 29, 120 + line * 25, FONT_SMALL, itemColor, to);
		}

		//print name
		printAtMiddleLoc(currentItem->getNameForList(), 213, 128 + line * 25, FONT_SMALL, itemColor, to);
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
	for(int i = curItems.size() - 1; i >= 0; i--)
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
		Settings lastMap = settings.write["session"]["lastMap"];
		lastMap->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::loadGame)
	{
		Settings lastSave = settings.write["session"]["lastSave"];
		lastSave->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::campaignList)
	{
		Settings lastCampaign = settings.write["session"]["lastCampaign"];
		lastCampaign->String() = getSelectedMapInfo()->fileURI;
	}
}

void SelectionTab::restoreLastSelection()
{
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		selectFileName(settings["session"]["lastMap"].String());
		break;
	case ESelectionScreen::campaignList:
		selectFileName(settings["session"]["lastCampaign"].String());
		break;
	case ESelectionScreen::loadGame:
	case ESelectionScreen::saveGame:
		selectFileName(settings["session"]["lastSave"].String());
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
			CLoadFile lf(*CResourceHandler::get()->getResourceName(file), MINIMAL_SERIALIZATION_VERSION);
			lf.checkMagicBytes(SAVEGAME_MAGIC);

			// Create the map info object
			auto mapInfo = std::make_shared<CMapInfo>();
			mapInfo->mapHeader = make_unique<CMapHeader>();
			mapInfo->scenarioOpts = nullptr; //to be created by serialiser
			lf >> *(mapInfo->mapHeader.get()) >> mapInfo->scenarioOpts;
			mapInfo->fileURI = file.getName();
			mapInfo->countPlayers();
			std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(file));
			mapInfo->date = std::asctime(std::localtime(&time));
			mapInfo->isSaveGame = true;
			// We absolutely not need this data for lobby and server will read it from save
			// FIXME: actually we don't want them in CMapHeader!
			mapInfo->mapHeader->triggeredEvents.clear();

			// Filter out other game modes
			bool isCampaign = mapInfo->scenarioOpts->mode == StartInfo::CAMPAIGN;
			bool isMultiplayer = mapInfo->actualHumanPlayers > 1;
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
