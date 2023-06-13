/*
 * CTradeWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTradeWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../widgets/Images.h"
#include "../render/Canvas.h"
#include "../gui/TextAlignment.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGMarket.h"

CTradeWindow::CTradeableItem::CTradeableItem(Point pos, EType Type, int ID, bool Left, int Serial)
	: CIntObject(LCLICK | HOVER | SHOW_POPUP, pos),
	type(EType(-1)),// set to invalid, will be corrected in setType
	id(ID),
	serial(Serial),
	left(Left)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	downSelection = false;
	hlp = nullptr;
	setType(Type);
}

void CTradeWindow::CTradeableItem::setType(EType newType)
{
	if(type != newType)
	{
		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
		type = newType;

		if(getIndex() < 0)
		{
			image = std::make_shared<CAnimImage>(getFilename(), 0);
			image->disable();
		}
		else
		{
			image = std::make_shared<CAnimImage>(getFilename(), getIndex());
		}
	}
}

void CTradeWindow::CTradeableItem::setID(int newID)
{
	if (id != newID)
	{
		id = newID;
		if (image)
		{
			int index = getIndex();
			if (index < 0)
				image->disable();
			else
			{
				image->enable();
				image->setFrame(index);
			}
		}
	}
}

std::string CTradeWindow::CTradeableItem::getFilename()
{
	switch(type)
	{
	case RESOURCE:
		return "RESOURCE";
	case PLAYER:
		return "CREST58";
	case ARTIFACT_TYPE:
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return "artifact";
	case CREATURE:
		return "TWCRPORT";
	default:
		return "";
	}
}

int CTradeWindow::CTradeableItem::getIndex()
{
	if (id < 0)
		return -1;

	switch(type)
	{
	case RESOURCE:
	case PLAYER:
		return id;
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
	case ARTIFACT_PLACEHOLDER:
		return CGI->artifacts()->getByIndex(id)->getIconIndex();
	case CREATURE:
		return CGI->creatures()->getByIndex(id)->getIconIndex();
	default:
		return -1;
	}
}

void CTradeWindow::CTradeableItem::showAll(Canvas & to)
{
	CTradeWindow *mw = dynamic_cast<CTradeWindow *>(parent);
	assert(mw);

	Point posToBitmap;
	Point posToSubCenter;

	switch(type)
	{
	case RESOURCE:
		posToBitmap = Point(19,9);
		posToSubCenter = Point(36, 59);
		break;
	case CREATURE_PLACEHOLDER:
	case CREATURE:
		posToSubCenter = Point(29, 76);
		// Positing of unit count is different in Altar of Sacrifice and Freelancer's Guild
		if(mw->mode == EMarketMode::CREATURE_EXP && downSelection)
			posToSubCenter.y += 5;
		break;
	case PLAYER:
		posToSubCenter = Point(31, 76);
		break;
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		posToSubCenter = Point(19, 55);
		if(downSelection)
			posToSubCenter.y += 8;
		break;
	case ARTIFACT_TYPE:
		posToSubCenter = Point(19, 58);
		break;
	}

	if (image)
	{
		image->moveTo(pos.topLeft() + posToBitmap);
		CIntObject::showAll(to);
	}

	to.drawText(pos.topLeft() + posToSubCenter, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, subtitle);
}

void CTradeWindow::CTradeableItem::clickLeft(tribool down, bool previousState)
{
	CTradeWindow *mw = dynamic_cast<CTradeWindow *>(parent);
	assert(mw);
	if(down)
	{

		if(type == ARTIFACT_PLACEHOLDER)
		{
			CAltarWindow *aw = static_cast<CAltarWindow *>(mw);
			const auto pickedArtInst = aw->getPickedArtifact();

			auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(aw->arts);
			if(pickedArtInst)
			{
				artifactsOfHero->pickedArtMoveToAltar(ArtifactPosition::TRANSITION_POS);
				aw->moveArtToAltar(this->shared_from_this(), pickedArtInst);
			}
			else if(const CArtifactInstance *art = getArtInstance())
			{
				const auto hero = artifactsOfHero->getHero();
				const auto slot = hero->getSlotByInstance(art);
				assert(slot != ArtifactPosition::PRE_FIRST);
				LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero, slot),
					ArtifactLocation(hero, ArtifactPosition::TRANSITION_POS));
				artifactsOfHero->pickedArtFromSlot = slot;
				artifactsOfHero->artifactsOnAltar.erase(art);
				setID(-1);
				subtitle.clear();
				aw->deal->block(!artifactsOfHero->artifactsOnAltar.size());
			}

			aw->calcTotalExp();
			return;
		}
		if(left)
		{
			if(mw->hLeft != this->shared_from_this())
				mw->hLeft = this->shared_from_this();
			else
				return;
		}
		else
		{
			if(mw->hRight != this->shared_from_this())
				mw->hRight = this->shared_from_this();
			else
				return;
		}
		mw->selectionChanged(left);
	}
}

void CTradeWindow::CTradeableItem::showAllAt(const Point &dstPos, const std::string &customSub, Canvas & to)
{
	Rect oldPos = pos;
	std::string oldSub = subtitle;
	downSelection = true;

	moveTo(dstPos);
	subtitle = customSub;
	showAll(to);

	downSelection = false;
	moveTo(oldPos.topLeft());
	subtitle = oldSub;
}

void CTradeWindow::CTradeableItem::hover(bool on)
{
	if(!on)
	{
		GH.statusbar()->clear();
		return;
	}

	switch(type)
	{
	case CREATURE:
	case CREATURE_PLACEHOLDER:
		GH.statusbar()->write(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->objects[id]->getNamePluralTranslated()));
		break;
	case ARTIFACT_PLACEHOLDER:
		if(id < 0)
			GH.statusbar()->write(CGI->generaltexth->zelp[582].first);
		else
			GH.statusbar()->write(CGI->artifacts()->getByIndex(id)->getNameTranslated());
		break;
	}
}

void CTradeWindow::CTradeableItem::showPopupWindow()
{
	switch(type)
	{
		case CREATURE:
		case CREATURE_PLACEHOLDER:
			//GH.statusbar->print(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->objects[id]->namePl));
			break;
		case ARTIFACT_TYPE:
		case ARTIFACT_PLACEHOLDER:
			//TODO: it's would be better for market to contain actual CArtifactInstance and not just ids of certain artifact type so we can use getEffectiveDescription.
			if(id >= 0)
				CRClickPopup::createAndPush(CGI->artifacts()->getByIndex(id)->getDescriptionTranslated());
			break;
	}
}

std::string CTradeWindow::CTradeableItem::getName(int number) const
{
	switch(type)
	{
	case PLAYER:
		return CGI->generaltexth->capColors[id];
	case RESOURCE:
		return CGI->generaltexth->restypes[id];
	case CREATURE:
		if(number == 1)
			return CGI->creh->objects[id]->getNameSingularTranslated();
		else
			return CGI->creh->objects[id]->getNamePluralTranslated();
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
		return CGI->artifacts()->getByIndex(id)->getNameTranslated();
	}
	logGlobal->error("Invalid trade item type: %d", (int)type);
	return "";
}

const CArtifactInstance * CTradeWindow::CTradeableItem::getArtInstance() const
{
	switch(type)
	{
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return hlp;
	default:
		return nullptr;
	}
}

void CTradeWindow::CTradeableItem::setArtInstance(const CArtifactInstance *art)
{
	assert(type == ARTIFACT_PLACEHOLDER || type == ARTIFACT_INSTANCE);
	hlp = art;
	if(art)
		setID(art->artType->getId());
	else
		setID(-1);
}

CTradeWindow::CTradeWindow(std::string bgName, const IMarket *Market, const CGHeroInstance *Hero, EMarketMode::EMarketMode Mode):
	CWindowObject(PLAYER_COLORED, bgName),
	market(Market),
	hero(Hero),
	readyToTrade(false)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	mode = Mode;
	initTypes();
}

void CTradeWindow::initTypes()
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		itemsType[1] = RESOURCE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::RESOURCE_PLAYER:
		itemsType[1] = RESOURCE;
		itemsType[0] = PLAYER;
		break;
	case EMarketMode::CREATURE_RESOURCE:
		itemsType[1] = CREATURE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		itemsType[1] = RESOURCE;
		itemsType[0] = ARTIFACT_TYPE;
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		itemsType[1] = ARTIFACT_INSTANCE;
		itemsType[0] = RESOURCE;
		break;
	case EMarketMode::CREATURE_EXP:
		itemsType[1] = CREATURE;
		itemsType[0] = CREATURE_PLACEHOLDER;
		break;
	case EMarketMode::ARTIFACT_EXP:
		itemsType[1] = ARTIFACT_TYPE;
		itemsType[0] = ARTIFACT_PLACEHOLDER;
		break;
	}
}

void CTradeWindow::initItems(bool Left)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	if(Left && (itemsType[1] == ARTIFACT_TYPE || itemsType[1] == ARTIFACT_INSTANCE))
	{
		int xOffset = 0, yOffset = 0;
		if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			xOffset = -361;
			yOffset = +46;

			auto item = std::make_shared<CTradeableItem>(Point(137, 469), itemsType[Left], -1, 1, 0);
			item->recActions &= ~(UPDATE | SHOWALL);
			items[Left].push_back(item);
		}
		else //ARTIFACT_EXP
		{
			xOffset = -365;
			yOffset = -12;
		}

		if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			auto artifactsOfHero = std::make_shared<CArtifactsOfHeroMarket>(Point(xOffset, yOffset));
			artifactsOfHero->selectArtCallback = std::bind(&CTradeWindow::artifactSelected, this, _1);
			artifactsOfHero->setHero(hero);
			addSet(artifactsOfHero);
			arts = artifactsOfHero;
		}
		else
		{
			auto artifactsOfHero = std::make_shared<CArtifactsOfHeroAltar>(Point(xOffset, yOffset));
			artifactsOfHero->setHero(hero);
			addSet(artifactsOfHero);
			arts = artifactsOfHero;
		}
	}
	else
	{
		std::vector<int> *ids = getItemsIds(Left);
		std::vector<Rect> pos;
		int amount = -1;

		getPositionsFor(pos, Left, itemsType[Left]);

		if(Left || !ids)
			amount = 7;
		else
			amount = static_cast<int>(ids->size());

		if(ids)
			vstd::amin(amount, ids->size());

		for(int j=0; j<amount; j++)
		{
			int id = (ids && ids->size()>j) ? (*ids)[j] : j;
			if(id < 0 && mode != EMarketMode::ARTIFACT_EXP)  //when sacrificing artifacts we need to prepare empty slots
				continue;

			auto item = std::make_shared<CTradeableItem>(pos[j].topLeft(), itemsType[Left], id, Left, j);
			item->pos = pos[j] + this->pos.topLeft();
			items[Left].push_back(item);
		}
		vstd::clear_pointer(ids);
		initSubs(Left);
	}
}

std::vector<int> *CTradeWindow::getItemsIds(bool Left)
{
	std::vector<int> *ids = nullptr;

	if(mode == EMarketMode::ARTIFACT_EXP)
		return new std::vector<int>(22, -1);

	if(Left)
	{
		switch(itemsType[1])
		{
		case CREATURE:
			ids = new std::vector<int>;
			for(int i = 0; i < 7; i++)
			{
				if(const CCreature *c = hero->getCreature(SlotID(i)))
					ids->push_back(c->getId());
				else
					ids->push_back(-1);
			}
			break;
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case PLAYER:
			ids = new std::vector<int>;
			for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
				if(PlayerColor(i) != LOCPLINT->playerID && LOCPLINT->cb->getPlayerStatus(PlayerColor(i)) == EPlayerStatus::INGAME)
					ids->push_back(i);
			break;

		case ARTIFACT_TYPE:
			ids = new std::vector<int>(market->availableItemsIds(mode));
			break;
		}
	}

	return ids;
}

void CTradeWindow::getPositionsFor(std::vector<Rect> &poss, bool Left, EType type) const
{
	if(mode == EMarketMode::ARTIFACT_EXP && !Left)
	{
		//22 boxes, 5 in row, last row: two boxes centered
		int h, w, x, y, dx, dy;
		h = w = 44;
		x = 317;
		y = 53;
		dx = 54;
		dy = 70;
		for (int i = 0; i < 4 ; i++)
			for (int j = 0; j < 5 ; j++)
				poss.push_back(Rect(x + dx*j, y + dy*i, w, h));

		poss.push_back(Rect((int)(x + dx * 1.5), (y + dy * 4), w, h));
		poss.push_back(Rect((int)(x + dx * 2.5), (y + dy * 4), w, h));
	}
	else
	{
		//seven boxes:
		//  X  X  X
		//  X  X  X
		//     X
		int h, w, x, y, dx, dy;
		int leftToRightOffset;
		getBaseForPositions(type, dx, dy, x, y, h, w, !Left, leftToRightOffset);

		const std::vector<Rect> tmp =
		{
			Rect(Point(x + 0 * dx, y + 0 * dx), Point(w, h) ),
			Rect(Point(x + 1 * dx, y + 0 * dx), Point(w, h) ),
			Rect(Point(x + 2 * dx, y + 0 * dx), Point(w, h) ),
			Rect(Point(x + 0 * dx, y + 1 * dy), Point(w, h) ),
			Rect(Point(x + 1 * dx, y + 1 * dy), Point(w, h) ),
			Rect(Point(x + 2 * dx, y + 1 * dy), Point(w, h) ),
			Rect(Point(x + 1 * dx, y + 2 * dy), Point(w, h) )
		};

		vstd::concatenate(poss, tmp);

		if(!Left)
		{
			for(Rect &r : poss)
				r.x += leftToRightOffset;
		}
	}
}

void CTradeWindow::initSubs(bool Left)
{
	for(auto item : items[Left])
	{
		if(Left)
		{
			switch(itemsType[1])
			{
			case CREATURE:
				item->subtitle = std::to_string(hero->getStackCount(SlotID(item->serial)));
				break;
			case RESOURCE:
				item->subtitle = std::to_string(LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(item->serial)));
				break;
			}
		}
		else //right side
		{
			if(itemsType[0] == PLAYER)
			{
				item->subtitle = CGI->generaltexth->capColors[item->id];
			}
			else if(hLeft)//artifact, creature
			{
				int h1, h2; //hlp variables for getting offer
				market->getOffer(hLeft->id, item->id, h1, h2, mode);
				if(item->id != hLeft->id || mode != EMarketMode::RESOURCE_RESOURCE) //don't allow exchanging same resources
				{
					std::ostringstream oss;
					oss << h2;
					if(h1!=1)
						oss << "/" << h1;
					item->subtitle = oss.str();
				}
				else
					item->subtitle = CGI->generaltexth->allTexts[164]; // n/a
			}
			else
				item->subtitle = "";
		}
	}
}

void CTradeWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);

	if(hRight)
		to.drawBorder(Rect::createAround(hRight->pos, 1), Colors::BRIGHT_YELLOW, 2);
	if(hLeft && hLeft->type != ARTIFACT_INSTANCE)
		to.drawBorder(Rect::createAround(hLeft->pos, 1), Colors::BRIGHT_YELLOW, 2);

	if(readyToTrade)
	{
		if(hLeft)
			hLeft->showAllAt(pos.topLeft() + selectionOffset(true), selectionSubtitle(true), to);
		if(hRight)
			hRight->showAllAt(pos.topLeft() + selectionOffset(false), selectionSubtitle(false), to);
	}
}

void CTradeWindow::removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : toRemove)
		removeItem(item);
}

void CTradeWindow::removeItem(std::shared_ptr<CTradeableItem> item)
{
	items[item->left] -= item;

	if(hRight == item)
	{
		hRight.reset();
		selectionChanged(false);
	}
}

void CTradeWindow::getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : items[1])
		if(!hero->getStackCount(SlotID(item->serial)))
			toRemove.insert(item);
}

void CTradeWindow::setMode(EMarketMode::EMarketMode Mode)
{
	const IMarket *m = market;
	const CGHeroInstance *h = hero;

	close();

	switch(Mode)
	{
	case EMarketMode::CREATURE_EXP:
	case EMarketMode::ARTIFACT_EXP:
		GH.windows().createAndPushWindow<CAltarWindow>(m, h, Mode);
		break;
	default:
		GH.windows().createAndPushWindow<CMarketplaceWindow>(m, h, Mode);
		break;
	}
}

void CTradeWindow::artifactSelected(CHeroArtPlace *slot)
{
	assert(mode == EMarketMode::ARTIFACT_RESOURCE);
	items[1][0]->setArtInstance(slot->getArt());
	if(slot->getArt())
		hLeft = items[1][0];
	else
		hLeft = nullptr;

	selectionChanged(true);
}

std::string CMarketplaceWindow::getBackgroundForMode(EMarketMode::EMarketMode mode)
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		return "TPMRKRES.bmp";
	case EMarketMode::RESOURCE_PLAYER:
		return "TPMRKPTS.bmp";
	case EMarketMode::CREATURE_RESOURCE:
		return "TPMRKCRS.bmp";
	case EMarketMode::RESOURCE_ARTIFACT:
		return "TPMRKABS.bmp";
	case EMarketMode::ARTIFACT_RESOURCE:
		return "TPMRKASS.bmp";
	}
	assert(0);
	return "";
}

CMarketplaceWindow::CMarketplaceWindow(const IMarket * Market, const CGHeroInstance * Hero, EMarketMode::EMarketMode Mode)
	: CTradeWindow(getBackgroundForMode(Mode), Market, Hero, Mode)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	madeTransaction = false;
	bool sliderNeeded = (mode != EMarketMode::RESOURCE_ARTIFACT && mode != EMarketMode::ARTIFACT_RESOURCE);

	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	std::string title;

	if(auto * o = dynamic_cast<const CGTownInstance *>(market))
	{
		switch (mode)
		{
		case EMarketMode::CREATURE_RESOURCE:
			title = (*CGI->townh)[ETownType::STRONGHOLD]->town->buildings[BuildingID::FREELANCERS_GUILD]->getNameTranslated();
			break;
		case EMarketMode::RESOURCE_ARTIFACT:
			title = (*CGI->townh)[o->subID]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();
			break;
		case EMarketMode::ARTIFACT_RESOURCE:
			title = (*CGI->townh)[o->subID]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();

			// create image that copies part of background containing slot MISC_1 into position of slot MISC_5
			// this is workaround for bug in H3 files where this slot for ragdoll on this screen is missing
			images.push_back(std::make_shared<CPicture>(background->getSurface(), Rect(20, 187, 47, 47), 18, 339 ));
			break;
		default:
			title = CGI->generaltexth->allTexts[158];
			break;
		}
	}
	else if(auto * o = dynamic_cast<const CGMarket *>(market))
	{
		title = o->title;
	}

	titleLabel = std::make_shared<CLabel>(300, 27, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, title);

	initItems(false);
	initItems(true);

	ok = std::make_shared<CButton>(Point(516, 520), "IOK6432.DEF", CGI->generaltexth->zelp[600], [&](){ close(); }, EShortcut::GLOBAL_RETURN);
	deal = std::make_shared<CButton>(Point(307, 520), "TPMRKB.DEF", CGI->generaltexth->zelp[595], [&](){ makeDeal(); } );
	deal->block(true);

	if(sliderNeeded)
	{
		slider = std::make_shared<CSlider>(Point(231, 490),137, std::bind(&CMarketplaceWindow::sliderMoved,this,_1),0,0);
		max = std::make_shared<CButton>(Point(229, 520), "IRCBTNS.DEF", CGI->generaltexth->zelp[596], [&](){ setMax(); });
		max->block(true);
	}
	else
	{
		deal->moveBy(Point(-30, 0));
	}

	//left side
	switch(Mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
	case EMarketMode::RESOURCE_ARTIFACT:
		labels.push_back(std::make_shared<CLabel>(154, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[270]));
		break;
	case EMarketMode::CREATURE_RESOURCE:
		//%s's Creatures
		labels.push_back(std::make_shared<CLabel>(152, 102, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		//%s's Artifacts
		labels.push_back(std::make_shared<CLabel>(152, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->allTexts[271]) % hero->getNameTranslated())));
		break;
	}

	Rect traderTextRect;

	//right side
	switch(Mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
	case EMarketMode::ARTIFACT_RESOURCE:
		labels.push_back(std::make_shared<CLabel>(445, 148, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[168]));
		traderTextRect = Rect(316, 48, 260, 75);
		break;
	case EMarketMode::RESOURCE_PLAYER:
		labels.push_back(std::make_shared<CLabel>(445, 55, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[169]));
		traderTextRect = Rect(28, 48, 260, 75);
		break;
	}

	traderText = std::make_shared<CTextBox>("", traderTextRect, 0, FONT_SMALL, ETextAlignment::CENTER);
	int specialOffset = mode == EMarketMode::ARTIFACT_RESOURCE ? 35 : 0; //in selling artifacts mode we need to move res-res and art-res buttons down

	if(printButtonFor(EMarketMode::RESOURCE_PLAYER))
		buttons.push_back(std::make_shared<CButton>(Point(18, 520),"TPMRKBU1.DEF", CGI->generaltexth->zelp[612], [&](){ setMode(EMarketMode::RESOURCE_PLAYER);}));
	if(printButtonFor(EMarketMode::RESOURCE_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(516, 450 + specialOffset),"TPMRKBU5.DEF", CGI->generaltexth->zelp[605], [&](){ setMode(EMarketMode::RESOURCE_RESOURCE);}));
	if(printButtonFor(EMarketMode::CREATURE_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(516, 485),"TPMRKBU4.DEF", CGI->generaltexth->zelp[599], [&](){ setMode(EMarketMode::CREATURE_RESOURCE);}));
	if(printButtonFor(EMarketMode::RESOURCE_ARTIFACT))
		buttons.push_back(std::make_shared<CButton>(Point(18, 450 + specialOffset),"TPMRKBU2.DEF", CGI->generaltexth->zelp[598], [&](){ setMode(EMarketMode::RESOURCE_ARTIFACT);}));
	if(printButtonFor(EMarketMode::ARTIFACT_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(18, 485),"TPMRKBU3.DEF", CGI->generaltexth->zelp[613], [&](){ setMode(EMarketMode::ARTIFACT_RESOURCE);}));

	updateTraderText();
}

CMarketplaceWindow::~CMarketplaceWindow() = default;

void CMarketplaceWindow::setMax()
{
	slider->scrollToMax();
}

void CMarketplaceWindow::makeDeal()
{
	int sliderValue = 0;
	if(slider)
		sliderValue = slider->getValue();
	else
		sliderValue = !deal->isBlocked(); //should always be 1

	if(!sliderValue)
		return;

	bool allowDeal = true;
	int leftIdToSend = hLeft->id;
	switch (mode)
	{
		case EMarketMode::CREATURE_RESOURCE:
			leftIdToSend = hLeft->serial;
			break;
		case EMarketMode::ARTIFACT_RESOURCE:
			leftIdToSend = hLeft->getArtInstance()->id.getNum();
			break;
		case EMarketMode::RESOURCE_ARTIFACT:
			if(!ArtifactID(hRight->id).toArtifact()->canBePutAt(hero))
			{
				LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.326"));
				allowDeal = false;
			}
			break;
		default:
			break;
	}

	if(allowDeal)
	{
		if(slider)
		{
			LOCPLINT->cb->trade(market, mode, leftIdToSend, hRight->id, slider->getValue() * r1, hero);
			slider->scrollTo(0);
		}
		else
		{
			LOCPLINT->cb->trade(market, mode, leftIdToSend, hRight->id, r2, hero);
		}
	}

	madeTransaction = true;
	hLeft = nullptr;
	hRight = nullptr;
	selectionChanged(true);
}

void CMarketplaceWindow::sliderMoved( int to )
{
	redraw();
}

void CMarketplaceWindow::selectionChanged(bool side)
{
	readyToTrade = hLeft && hRight;
	if(mode == EMarketMode::RESOURCE_RESOURCE)
		readyToTrade = readyToTrade && (hLeft->id != hRight->id); //for resource trade, two DIFFERENT resources must be selected

	if(mode == EMarketMode::ARTIFACT_RESOURCE && !hLeft)
		arts->unmarkSlots();

	if(readyToTrade)
	{
		int soldItemId = hLeft->id;
		market->getOffer(soldItemId, hRight->id, r1, r2, mode);

		if(slider)
		{
			int newAmount = -1;
			if(itemsType[1] == RESOURCE)
				newAmount = LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(soldItemId));
			else if(itemsType[1] ==  CREATURE)
				newAmount = hero->getStackCount(SlotID(hLeft->serial)) - (hero->stacksCount() == 1  &&  hero->needsLastStack());
			else
				assert(0);

			slider->setAmount(newAmount / r1);
			slider->scrollTo(0);
			max->block(false);
			deal->block(false);
		}
		else if(itemsType[1] == RESOURCE) //buying -> check if we can afford transaction
		{
			deal->block(LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(soldItemId)) < r1);
		}
		else
			deal->block(false);
	}
	else
	{
		if(slider)
		{
			max->block(true);
			slider->setAmount(0);
			slider->scrollTo(0);
		}
		deal->block(true);
	}

	if(side && itemsType[0] != PLAYER) //items[1] selection changed, recalculate offers
		initSubs(false);

	updateTraderText();
	redraw();
}

bool CMarketplaceWindow::printButtonFor(EMarketMode::EMarketMode M) const
{
	return market->allowsTrade(M) && M != mode && (hero || ( M != EMarketMode::CREATURE_RESOURCE && M != EMarketMode::RESOURCE_ARTIFACT && M != EMarketMode::ARTIFACT_RESOURCE ));
}

void CMarketplaceWindow::garrisonChanged()
{
	if(mode != EMarketMode::CREATURE_RESOURCE)
		return;

	std::set<std::shared_ptr<CTradeableItem>> toRemove;
	getEmptySlots(toRemove);
	removeItems(toRemove);
	initSubs(true);
}

void CMarketplaceWindow::artifactsChanged(bool Left)
{
	assert(!Left);
	if(mode != EMarketMode::RESOURCE_ARTIFACT)
		return;

	std::vector<int> available = market->availableItemsIds(mode);
	std::set<std::shared_ptr<CTradeableItem>> toRemove;
	for(auto item : items[0])
		if(!vstd::contains(available, item->id))
			toRemove.insert(item);

	removeItems(toRemove);

	// clear set to erase final instance of shared_ptr - we want to redraw screen only after it has been deleted
	toRemove.clear();
	redraw();
}

std::string CMarketplaceWindow::selectionSubtitle(bool Left) const
{
	if(Left)
	{
		switch(itemsType[1])
		{
		case RESOURCE:
		case CREATURE:
			{
				int val = slider
					? slider->getValue() * r1
					: (((deal->isBlocked())) ? 0 : r1);

				return std::to_string(val);
			}
		case ARTIFACT_INSTANCE:
			return ((deal->isBlocked()) ? "0" : "1");
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case RESOURCE:
			if(slider)
				return std::to_string( slider->getValue() * r2 );
			else
				return std::to_string(r2);
		case ARTIFACT_TYPE:
			return ((deal->isBlocked()) ? "0" : "1");
		case PLAYER:
			return (hRight ? CGI->generaltexth->capColors[hRight->id] : "");
		}
	}

	return "???";
}

Point CMarketplaceWindow::selectionOffset(bool Left) const
{
	if(Left)
	{
		switch(itemsType[1])
		{
		case RESOURCE:
			return Point(122, 446);
		case CREATURE:
			return Point(128, 450);
		case ARTIFACT_INSTANCE:
			return Point(134, 466);
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case RESOURCE:
			if(mode == EMarketMode::ARTIFACT_RESOURCE)
				return Point(410, 469);
			else
				return Point(410, 446);
		case ARTIFACT_TYPE:
			return Point(425, 447);
		case PLAYER:
			return Point(417, 451);
		}
	}

	assert(0);
	return Point(0,0);
}

void CMarketplaceWindow::resourceChanged()
{
	initSubs(true);
}

void CMarketplaceWindow::getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const
{
	switch(type)
	{
	case RESOURCE:
		dx = 82;
		dy = 79;
		x = 39;
		y = 180;
		h = 68;
		w = 70;
		break;
	case PLAYER:
		dx = 83;
		dy = 118;
		h = 64;
		w = 58;
		x = 44;
		y = 83;
		assert(Right);
		break;
	case CREATURE://45,123
		x = 45;
		y = 123;
		w = 58;
		h = 64;
		dx = 83;
		dy = 98;
		assert(!Right);
		break;
	case ARTIFACT_TYPE://45,123
		x = 340-289;
		y = 180;
		w = 44;
		h = 44;
		dx = 83;
		dy = 79;
		break;
	}

	leftToRightOffset = 289;
}

void CMarketplaceWindow::updateTraderText()
{
	if(readyToTrade)
	{
		if(mode == EMarketMode::RESOURCE_PLAYER)
		{
			//I can give %s to the %s player.
			traderText->setText(boost::str(boost::format(CGI->generaltexth->allTexts[165]) % hLeft->getName() % hRight->getName()));
		}
		else if(mode == EMarketMode::RESOURCE_ARTIFACT)
		{
			//I can offer you the %s for %d %s of %s.
			traderText->setText(boost::str(boost::format(CGI->generaltexth->allTexts[267]) % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == EMarketMode::RESOURCE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s of %s.
			traderText->setText(boost::str(boost::format(CGI->generaltexth->allTexts[157]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % CGI->generaltexth->allTexts[160 + (r1==1)] % hLeft->getName()));
		}
		else if(mode == EMarketMode::CREATURE_RESOURCE)
		{
			//I can offer you %d %s of %s for %d %s.
			traderText->setText(boost::str(boost::format(CGI->generaltexth->allTexts[269]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % r1 % hLeft->getName(r1)));
		}
		else if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			//I can offer you %d %s of %s for your %s.
			traderText->setText(boost::str(boost::format(CGI->generaltexth->allTexts[268]) % r2 % CGI->generaltexth->allTexts[160 + (r2==1)] % hRight->getName() % hLeft->getName(r1)));
		}
		return;
	}

	int gnrtxtnr = -1;
	if(madeTransaction)
	{
		if(mode == EMarketMode::RESOURCE_PLAYER)
			gnrtxtnr = 166; //Are there any other resources you'd like to give away?
		else
			gnrtxtnr = 162; //You have received quite a bargain.  I expect to make no profit on the deal.  Can I interest you in any of my other wares?
	}
	else
	{
		if(mode == EMarketMode::RESOURCE_PLAYER)
			gnrtxtnr = 167; //If you'd like to give any of your resources to another player, click on the item you wish to give and to whom.
		else
			gnrtxtnr = 163; //Please inspect our fine wares.  If you feel like offering a trade, click on the items you wish to trade with and for.
	}
	traderText->setText(CGI->generaltexth->allTexts[gnrtxtnr]);
}

CAltarWindow::CAltarWindow(const IMarket * Market, const CGHeroInstance * Hero, EMarketMode::EMarketMode Mode)
	: CTradeWindow((Mode == EMarketMode::CREATURE_EXP ? "ALTARMON.bmp" : "ALTRART2.bmp"), Market, Hero, Mode)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	if(Mode == EMarketMode::CREATURE_EXP)
	{
		//%s's Creatures
		labels.push_back(std::make_shared<CLabel>(155, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
				   boost::str(boost::format(CGI->generaltexth->allTexts[272]) % hero->getNameTranslated())));

		//Altar of Sacrifice
		labels.push_back(std::make_shared<CLabel>(450, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[479]));

		 //To sacrifice creatures, move them from your army on to the Altar and click Sacrifice
		new CTextBox(CGI->generaltexth->allTexts[480], Rect(320, 56, 256, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW);

		slider = std::make_shared<CSlider>(Point(231,481),137,std::bind(&CAltarWindow::sliderMoved,this,_1),0,0);
		max = std::make_shared<CButton>(Point(147, 520), "IRCBTNS.DEF", CGI->generaltexth->zelp[578], std::bind(&CSlider::scrollToMax, slider));

		sacrificedUnits.resize(GameConstants::ARMY_SIZE, 0);
		sacrificeAll = std::make_shared<CButton>(Point(393, 520), "ALTARMY.DEF", CGI->generaltexth->zelp[579], std::bind(&CAltarWindow::SacrificeAll,this));

		initItems(true);
		mimicCres();
	}
	else
	{
		//Sacrifice artifacts for experience
		labels.push_back(std::make_shared<CLabel>(450, 34, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[477]));
		//%s's Creatures
		labels.push_back(std::make_shared<CLabel>(302, 423, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[478]));

		sacrificeAll = std::make_shared<CButton>(Point(393, 520), "ALTFILL.DEF", CGI->generaltexth->zelp[571], std::bind(&CAltarWindow::SacrificeAll,this));
		sacrificeAll->block(hero->artifactsInBackpack.empty() && hero->artifactsWorn.empty());
		sacrificeBackpack = std::make_shared<CButton>(Point(147, 520), "ALTEMBK.DEF", CGI->generaltexth->zelp[570], std::bind(&CAltarWindow::SacrificeBackpack,this));
		sacrificeBackpack->block(hero->artifactsInBackpack.empty());

		initItems(true);
		initItems(false);
		artIcon = std::make_shared<CAnimImage>("ARTIFACT", 0, 0, 281, 442);
		artIcon->disable();
	}

	//Experience needed to reach next level
	texts.push_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	//Total experience on the Altar
	texts.push_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));

	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	ok = std::make_shared<CButton>(Point(516, 520), "IOK6432.DEF", CGI->generaltexth->zelp[568], [&](){ close();}, EShortcut::GLOBAL_RETURN);

	deal = std::make_shared<CButton>(Point(269, 520), "ALTSACR.DEF", CGI->generaltexth->zelp[585], std::bind(&CAltarWindow::makeDeal,this));

	if(Mode == EMarketMode::CREATURE_EXP)
	{
		auto changeMode = std::make_shared<CButton>(Point(516, 421), "ALTART.DEF", CGI->generaltexth->zelp[580], std::bind(&CTradeWindow::setMode,this, EMarketMode::ARTIFACT_EXP));
		if(Hero->getAlignment() == ::EAlignment::EVIL)
			changeMode->block(true);
		buttons.push_back(changeMode);
	}
	else if(Mode == EMarketMode::ARTIFACT_EXP)
	{
		auto changeMode = std::make_shared<CButton>(Point(516, 421), "ALTSACC.DEF", CGI->generaltexth->zelp[572], std::bind(&CTradeWindow::setMode,this, EMarketMode::CREATURE_EXP));
		if(Hero->getAlignment() == ::EAlignment::GOOD)
			changeMode->block(true);
		buttons.push_back(changeMode);
	}

	expPerUnit.resize(GameConstants::ARMY_SIZE, 0);
	getExpValues();

	expToLevel = std::make_shared<CLabel>(73, 475, FONT_SMALL, ETextAlignment::CENTER);
	expOnAltar = std::make_shared<CLabel>(73, 543, FONT_SMALL, ETextAlignment::CENTER);

	setExpToLevel();
	calcTotalExp();
	blockTrade();
}

CAltarWindow::~CAltarWindow() = default;

void CAltarWindow::getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const
{
	leftToRightOffset = 289;
	x = 45;
	y = 110;
	w = 58;
	h = 64;
	dx = 83;
	dy = 98;
}

void CAltarWindow::sliderMoved(int to)
{
	if(hLeft)
		sacrificedUnits[hLeft->serial] = to;
	if(hRight)
		updateRight(hRight);

	deal->block(!to);
	calcTotalExp();
	redraw();
}

void CAltarWindow::makeDeal()
{
	if(mode == EMarketMode::CREATURE_EXP)
	{
		blockTrade();
		slider->scrollTo(0);

		std::vector<ui32> ids;
		std::vector<ui32> toSacrifice;

		for(int i = 0; i < sacrificedUnits.size(); i++)
		{
			if(sacrificedUnits[i])
			{
				ids.push_back(i);
				toSacrifice.push_back(sacrificedUnits[i]);
			}
		}

		LOCPLINT->cb->trade(market, mode, ids, {}, toSacrifice, hero);

		for(int& val : sacrificedUnits)
			val = 0;

		for(auto item : items[0])
		{
			item->setType(CREATURE_PLACEHOLDER);
			item->subtitle = "";
		}
	}
	else
	{
		std::vector<ui32> positions;
		auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
		for(const CArtifactInstance * art : artifactsOfHero->artifactsOnAltar)
		{
			positions.push_back(hero->getSlotByInstance(art));
		}
		std::sort(positions.begin(), positions.end(), std::greater<>());

		LOCPLINT->cb->trade(market, mode, positions, {}, {}, hero);
		artifactsOfHero->artifactsOnAltar.clear();

		for(auto item : items[0])
		{
			item->setID(-1);
			item->subtitle = "";
		}

		//arts->scrollBackpack(0);
		deal->block(true);
	}

	calcTotalExp();
}

void CAltarWindow::SacrificeAll()
{
	if(mode == EMarketMode::CREATURE_EXP)
	{
		bool movedAnything = false;
		for(auto item : items[1])
			sacrificedUnits[item->serial] = hero->getStackCount(SlotID(item->serial));

		sacrificedUnits[items[1].front()->serial]--;

		for(auto item : items[0])
		{
			updateRight(item);
			if(item->type == CREATURE)
				movedAnything = true;
		}

		deal->block(!movedAnything);
		calcTotalExp();
	}
	else
	{
		auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
		for(const auto & aw : artifactsOfHero->visibleArtSet.artifactsWorn)
		{
			if(!aw.second.locked)
				moveArtToAltar(nullptr, aw.second.artifact);
		}
		artifactsOfHero->updateWornSlots();
		SacrificeBackpack();
	}
	redraw();
}

void CAltarWindow::selectionChanged(bool side)
{
	if(mode != EMarketMode::CREATURE_EXP)
		return;

	int stackCount = 0;
	for (int i = 0; i < GameConstants::ARMY_SIZE; i++)
		if(hero->getStackCount(SlotID(i)) > sacrificedUnits[i])
			stackCount++;

	slider->setAmount(hero->getStackCount(SlotID(hLeft->serial)) - (stackCount == 1));
	slider->block(!slider->getAmount());
	slider->scrollTo(sacrificedUnits[hLeft->serial]);
	max->block(!slider->getAmount());
	selectOppositeItem(side);
	readyToTrade = true;
	redraw();
}

void CAltarWindow::selectOppositeItem(bool side)
{
	bool oppositeSide = !side;
	int pos = vstd::find_pos(items[side], side ? hLeft : hRight);
	int oppositePos = vstd::find_pos(items[oppositeSide], oppositeSide ? hLeft : hRight);

	if(pos >= 0 && pos != oppositePos)
	{
		if(oppositeSide)
			hLeft = items[oppositeSide][pos];
		else
			hRight = items[oppositeSide][pos];

		selectionChanged(oppositeSide);
	}
}

void CAltarWindow::mimicCres()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	std::vector<Rect> positions;
	getPositionsFor(positions, false, CREATURE);

	for(auto item : items[1])
	{
		auto hlp = std::make_shared<CTradeableItem>(positions[item->serial].topLeft(), CREATURE_PLACEHOLDER, item->id, false, item->serial);
		hlp->pos = positions[item->serial] + this->pos.topLeft();
		items[0].push_back(hlp);
	}
}

Point CAltarWindow::selectionOffset(bool Left) const
{
	if(Left)
		return Point(150, 421);
	else
		return Point(396, 421);
}

std::string CAltarWindow::selectionSubtitle(bool Left) const
{
	if(Left && slider && hLeft)
		return std::to_string(slider->getValue());
	else if(!Left && hRight)
		return hRight->subtitle;
	else
		return "";
}

void CAltarWindow::artifactsChanged(bool left)
{

}

void CAltarWindow::garrisonChanged()
{
	if(mode != EMarketMode::CREATURE_EXP)
		return;

	std::set<std::shared_ptr<CTradeableItem>> empty;
	getEmptySlots(empty);

	removeItems(empty);

	initSubs(true);
	getExpValues();
}

void CAltarWindow::getExpValues()
{
	int dump;
	for(auto item : items[1])
	{
		if(item->id >= 0)
			market->getOffer(item->id, 0, dump, expPerUnit[item->serial], EMarketMode::CREATURE_EXP);
	}
}

void CAltarWindow::calcTotalExp()
{
	int val = 0;
	if(mode == EMarketMode::CREATURE_EXP)
	{
		for (int i = 0; i < sacrificedUnits.size(); i++)
		{
			val += expPerUnit[i] * sacrificedUnits[i];
		}
	}
	else
	{
		auto artifactsOfHero = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
		for(const CArtifactInstance * art : artifactsOfHero->artifactsOnAltar)
		{
			int dmp, valOfArt;
			market->getOffer(art->artType->getId(), 0, dmp, valOfArt, mode);
			val += valOfArt; //WAS val += valOfArt * arts->artifactsOnAltar.count(*i);
		}
	}
	val = static_cast<int>(hero->calculateXp(val));
	expOnAltar->setText(std::to_string(val));
}

void CAltarWindow::setExpToLevel()
{
	expToLevel->setText(std::to_string(CGI->heroh->reqExp(CGI->heroh->level(hero->exp)+1) - hero->exp));
}

void CAltarWindow::blockTrade()
{
	hLeft = hRight = nullptr;
	readyToTrade = false;
	if(slider)
	{
		slider->block(true);
		max->block(true);
	}
	deal->block(true);
}

void CAltarWindow::updateRight(std::shared_ptr<CTradeableItem> toUpdate)
{
	int val = sacrificedUnits[toUpdate->serial];
	toUpdate->setType(val ? CREATURE : CREATURE_PLACEHOLDER);
	toUpdate->subtitle = val ? boost::str(boost::format(CGI->generaltexth->allTexts[122]) % std::to_string(hero->calculateXp(val * expPerUnit[toUpdate->serial]))) : ""; //%s exp
}

int CAltarWindow::firstFreeSlot()
{
	int ret = -1;
	while(items[0][++ret]->id >= 0  &&  ret + 1 < items[0].size());
	return items[0][ret]->id == -1 ? ret : -1;
}

void CAltarWindow::SacrificeBackpack()
{
	auto artsAltar = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
	while(!artsAltar->visibleArtSet.artifactsInBackpack.empty())
	{
		if(!putOnAltar(nullptr, artsAltar->visibleArtSet.artifactsInBackpack[0].artifact))
			break;
	};
	calcTotalExp();
}

void CAltarWindow::artifactPicked()
{
	redraw();
}

void CAltarWindow::showAll(Canvas & to)
{
	CTradeWindow::showAll(to);
	if(mode == EMarketMode::ARTIFACT_EXP && arts)
	{
		if(auto pickedArt = arts->getPickedArtifact())
		{
			artIcon->setFrame(pickedArt->artType->getIconIndex());
			artIcon->showAll(to);

			int dmp, val;
			market->getOffer(pickedArt->getTypeId(), 0, dmp, val, EMarketMode::ARTIFACT_EXP);
			val = static_cast<int>(hero->calculateXp(val));

			to.drawText(Point(304, 498), FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, std::to_string(val));
		}
	}
}

bool CAltarWindow::putOnAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance *art)
{
	if(!art->artType->isTradable()) //special art
	{
		logGlobal->warn("Cannot put special artifact on altar!");
		return false;
	}

	if(!altarSlot || altarSlot->id != -1)
	{
		int slotIndex = firstFreeSlot();
		if(slotIndex < 0)
		{
			logGlobal->warn("No free slots on altar!");
			return false;
		}
		altarSlot = items[0][slotIndex];
	}

	int dmp, val;
	market->getOffer(art->artType->getId(), 0, dmp, val, EMarketMode::ARTIFACT_EXP);
	val = static_cast<int>(hero->calculateXp(val));

	auto artsAltar = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(arts);
	artsAltar->artifactsOnAltar.insert(art);
	artsAltar->deleteFromVisible(art);
	altarSlot->setArtInstance(art);
	altarSlot->subtitle = std::to_string(val);

	deal->block(false);
	return true;
}

void CAltarWindow::moveArtToAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance *art)
{
	if(putOnAltar(altarSlot, art))
	{
		CCS->curh->dragAndDropCursor(nullptr);
		arts->unmarkSlots();
	}
}
