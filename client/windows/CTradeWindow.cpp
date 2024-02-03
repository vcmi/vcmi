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
#include "../render/Canvas.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGMarket.h"

CTradeWindow::CTradeWindow(const ImagePath & bgName, const IMarket *Market, const CGHeroInstance *Hero, const std::function<void()> & onWindowClosed, EMarketMode Mode):
	CTradeBase(Market, Hero),
	CWindowObject(PLAYER_COLORED, bgName),
	onWindowClosed(onWindowClosed),
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
		itemsType[1] = EType::RESOURCE;
		itemsType[0] = EType::RESOURCE;
		break;
	case EMarketMode::RESOURCE_PLAYER:
		itemsType[1] = EType::RESOURCE;
		itemsType[0] = EType::PLAYER;
		break;
	case EMarketMode::CREATURE_RESOURCE:
		itemsType[1] = EType::CREATURE;
		itemsType[0] = EType::RESOURCE;
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		itemsType[1] = EType::RESOURCE;
		itemsType[0] = EType::ARTIFACT_TYPE;
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		itemsType[1] = EType::ARTIFACT_INSTANCE;
		itemsType[0] = EType::RESOURCE;
		break;
	}
}

void CTradeWindow::initItems(bool Left)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	if(Left && (itemsType[1] == EType::ARTIFACT_TYPE || itemsType[1] == EType::ARTIFACT_INSTANCE))
	{
		if(mode == EMarketMode::ARTIFACT_RESOURCE)
		{
			auto item = std::make_shared<CTradeableItem>(Rect(Point(137, 469), Point()), itemsType[Left], -1, 1, 0);
			item->recActions &= ~(UPDATE | SHOWALL);
			items[Left].push_back(item);
		}
	}
	else
	{
		auto updRightSub = [this](EMarketMode marketMode)
		{
			if(hLeft)
				for(const auto & slot : rightTradePanel->slots)
				{
					int h1, h2; //hlp variables for getting offer
					market->getOffer(hLeft->id, slot->id, h1, h2, marketMode);

					rightTradePanel->updateOffer(*slot, h1, h2);
				}
			else
				rightTradePanel->clearSubtitles();
		};

		auto clickPressedTradePanel = [this](const std::shared_ptr<CTradeableItem> & newSlot, bool left)
		{
			CTradeBase::onSlotClickPressed(newSlot, left ? hLeft : hRight);
			selectionChanged(left);
		};

		if(Left && mode == EMarketMode::CREATURE_RESOURCE)
		{
			CreaturesPanel::slotsData slots;
			for(auto slotId = SlotID(0); slotId.num < GameConstants::ARMY_SIZE; slotId++)
			{
				if(const auto & creature = hero->getCreature(slotId))
					slots.emplace_back(std::make_tuple(creature->getId(), slotId, hero->getStackCount(slotId)));
			}
			leftTradePanel = std::make_shared<CreaturesPanel>(std::bind(clickPressedTradePanel, _1, true), slots);
			leftTradePanel->moveBy(Point(45, 123));
			leftTradePanel->deleteSlotsCheck = [this](const std::shared_ptr<CTradeableItem> & slot)
			{
				return this->hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
			};
		}
		else if(Left && (mode == EMarketMode::RESOURCE_RESOURCE || mode == EMarketMode::RESOURCE_ARTIFACT || mode == EMarketMode::RESOURCE_PLAYER))
		{
			leftTradePanel = std::make_shared<ResourcesPanel>(
				[clickPressedTradePanel](const std::shared_ptr<CTradeableItem> & newSlot)
				{
					clickPressedTradePanel(newSlot, true);
				},
				[this]()
				{
					for(const auto & slot : leftTradePanel->slots)
						slot->subtitle = std::to_string(LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(slot->serial)));
				});
			leftTradePanel->moveBy(Point(39, 182));
			leftTradePanel->updateSlots();
		}
		else if(!Left && mode == EMarketMode::RESOURCE_RESOURCE)
		{
			rightTradePanel = std::make_shared<ResourcesPanel>(
				[clickPressedTradePanel](const std::shared_ptr<CTradeableItem> & newSlot)
				{
					clickPressedTradePanel(newSlot, false);
				},
				[this, updRightSub]()
				{
					updRightSub(EMarketMode::RESOURCE_RESOURCE);
					if(hLeft)
						rightTradePanel->slots[hLeft->serial]->subtitle = CGI->generaltexth->allTexts[164]; // n/a
				});
			rightTradePanel->moveBy(Point(327, 181));
		}
		else if(!Left && (mode == EMarketMode::ARTIFACT_RESOURCE || mode == EMarketMode::CREATURE_RESOURCE))
		{
			rightTradePanel = std::make_shared<ResourcesPanel>(std::bind(clickPressedTradePanel, _1, false),
				std::bind(updRightSub, EMarketMode::ARTIFACT_RESOURCE));
			rightTradePanel->moveBy(Point(327, 181));
		}
		else if(!Left && mode == EMarketMode::RESOURCE_ARTIFACT)
		{
			rightTradePanel = std::make_shared<ArtifactsPanel>(std::bind(clickPressedTradePanel, _1, false),
				std::bind(updRightSub, EMarketMode::RESOURCE_ARTIFACT), market->availableItemsIds(mode));
			rightTradePanel->moveBy(Point(327, 181));
			rightTradePanel->deleteSlotsCheck = [this](const std::shared_ptr<CTradeableItem> & slot)
			{
				return vstd::contains(market->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), ArtifactID(slot->id)) ? false : true;
			};
		}
		else if(!Left && mode == EMarketMode::RESOURCE_PLAYER)
		{
			rightTradePanel = std::make_shared<PlayersPanel>(std::bind(clickPressedTradePanel, _1, false));
			rightTradePanel->moveBy(Point(333, 83));
		}
	}
}

void CTradeWindow::initSubs(bool Left)
{
	if(itemsType[Left] == EType::RESOURCE || itemsType[Left] == EType::ARTIFACT_TYPE)
	{ 
		if(Left)
			leftTradePanel->updateSlots();
		else
			rightTradePanel->updateSlots();
		return;
	}
}

void CTradeWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);

	if(readyToTrade)
	{
		if(hLeft)
			hLeft->showAllAt(pos.topLeft() + selectionOffset(true), updateSlotSubtitle(true), to);
		if(hRight)
			hRight->showAllAt(pos.topLeft() + selectionOffset(false), updateSlotSubtitle(false), to);
	}
}

void CTradeWindow::close()
{
	if (onWindowClosed)
		onWindowClosed();

	CWindowObject::close();
}

void CTradeWindow::setMode(EMarketMode Mode)
{
	const IMarket *m = market;
	const CGHeroInstance *h = hero;
	const auto functor = onWindowClosed;

	onWindowClosed = nullptr; // don't call on closing of this window - pass it to next window
	close();

	switch(Mode)
	{
	case EMarketMode::CREATURE_EXP:
	case EMarketMode::ARTIFACT_EXP:
		break;
	default:
		GH.windows().createAndPushWindow<CMarketplaceWindow>(m, h, functor, Mode);
		break;
	}
}

void CTradeWindow::artifactSelected(CArtPlace * slot)
{
	assert(mode == EMarketMode::ARTIFACT_RESOURCE);
	items[1][0]->setArtInstance(slot->getArt());
	if(slot->getArt())
		hLeft = items[1][0];
	else
		hLeft = nullptr;

	selectionChanged(true);
}

ImagePath CMarketplaceWindow::getBackgroundForMode(EMarketMode mode)
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		return ImagePath::builtin("TPMRKRES.bmp");
	case EMarketMode::RESOURCE_PLAYER:
		return ImagePath::builtin("TPMRKPTS.bmp");
	case EMarketMode::CREATURE_RESOURCE:
		return ImagePath::builtin("TPMRKCRS.bmp");
	case EMarketMode::RESOURCE_ARTIFACT:
		return ImagePath::builtin("TPMRKABS.bmp");
	case EMarketMode::ARTIFACT_RESOURCE:
		return ImagePath::builtin("TPMRKASS.bmp");
	}
	assert(0);
	return {};
}

CMarketplaceWindow::CMarketplaceWindow(const IMarket * Market, const CGHeroInstance * Hero, const std::function<void()> & onWindowClosed, EMarketMode Mode)
	: CTradeWindow(getBackgroundForMode(Mode), Market, Hero, onWindowClosed, Mode)
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
			title = (*CGI->townh)[o->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();
			break;
		case EMarketMode::ARTIFACT_RESOURCE:
			title = (*CGI->townh)[o->getFaction()]->town->buildings[BuildingID::ARTIFACT_MERCHANT]->getNameTranslated();

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
	if(mode == EMarketMode::ARTIFACT_RESOURCE)
	{
		arts = std::make_shared<CArtifactsOfHeroMarket>(Point(-361, 46));
		arts->selectArtCallback = std::bind(&CTradeWindow::artifactSelected, this, _1);
		arts->setHero(hero);
		addSetAndCallbacks(arts);
	}
	initItems(false);
	initItems(true);

	ok = std::make_shared<CButton>(Point(516, 520), AnimationPath::builtin("IOK6432.DEF"), CGI->generaltexth->zelp[600], [&](){ close(); }, EShortcut::GLOBAL_RETURN);
	deal = std::make_shared<CButton>(Point(307, 520), AnimationPath::builtin("TPMRKB.DEF"), CGI->generaltexth->zelp[595], [&](){ makeDeal(); } );
	deal->block(true);

	if(sliderNeeded)
	{
		slider = std::make_shared<CSlider>(Point(231, 490), 137, std::bind(&CMarketplaceWindow::sliderMoved, this, _1), 0, 0, 0, Orientation::HORIZONTAL);
		max = std::make_shared<CButton>(Point(229, 520), AnimationPath::builtin("IRCBTNS.DEF"), CGI->generaltexth->zelp[596], [&](){ setMax(); });
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
		buttons.push_back(std::make_shared<CButton>(Point(18, 520),AnimationPath::builtin("TPMRKBU1.DEF"), CGI->generaltexth->zelp[612], [&](){ setMode(EMarketMode::RESOURCE_PLAYER);}));
	if(printButtonFor(EMarketMode::RESOURCE_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(516, 450 + specialOffset),AnimationPath::builtin("TPMRKBU5.DEF"), CGI->generaltexth->zelp[605], [&](){ setMode(EMarketMode::RESOURCE_RESOURCE);}));
	if(printButtonFor(EMarketMode::CREATURE_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(516, 485),AnimationPath::builtin("TPMRKBU4.DEF"), CGI->generaltexth->zelp[599], [&](){ setMode(EMarketMode::CREATURE_RESOURCE);}));
	if(printButtonFor(EMarketMode::RESOURCE_ARTIFACT))
		buttons.push_back(std::make_shared<CButton>(Point(18, 450 + specialOffset),AnimationPath::builtin("TPMRKBU2.DEF"), CGI->generaltexth->zelp[598], [&](){ setMode(EMarketMode::RESOURCE_ARTIFACT);}));
	if(printButtonFor(EMarketMode::ARTIFACT_RESOURCE))
		buttons.push_back(std::make_shared<CButton>(Point(18, 485),AnimationPath::builtin("TPMRKBU3.DEF"), CGI->generaltexth->zelp[613], [&](){ setMode(EMarketMode::ARTIFACT_RESOURCE);}));

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
			leftIdToSend = hLeft->getArtInstance()->getId().getNum();
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
		switch(mode)
		{
		case EMarketMode::RESOURCE_RESOURCE:
			LOCPLINT->cb->trade(market, mode, GameResID(leftIdToSend), GameResID(hRight->id), slider->getValue() * r1, hero);
			slider->scrollTo(0);
			break;
		case EMarketMode::CREATURE_RESOURCE:
			LOCPLINT->cb->trade(market, mode, SlotID(leftIdToSend), GameResID(hRight->id), slider->getValue() * r1, hero);
			slider->scrollTo(0);
			break;
		case EMarketMode::RESOURCE_PLAYER:
			LOCPLINT->cb->trade(market, mode, GameResID(leftIdToSend), PlayerColor(hRight->id), slider->getValue() * r1, hero);
			slider->scrollTo(0);
			break;

		case EMarketMode::RESOURCE_ARTIFACT:
			LOCPLINT->cb->trade(market, mode, GameResID(leftIdToSend), ArtifactID(hRight->id), r2, hero);
			break;
		case EMarketMode::ARTIFACT_RESOURCE:
			LOCPLINT->cb->trade(market, mode, ArtifactInstanceID(leftIdToSend), GameResID(hRight->id), r2, hero);
			break;
		}
	}

	madeTransaction = true;
	hLeft = nullptr;
	hRight = nullptr;
	if(leftTradePanel)
		leftTradePanel->deselect();
	assert(rightTradePanel);
	rightTradePanel->deselect();
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
			if(itemsType[1] == EType::RESOURCE)
				newAmount = LOCPLINT->cb->getResourceAmount(static_cast<EGameResID>(soldItemId));
			else if(itemsType[1] == EType::CREATURE)
				newAmount = hero->getStackCount(SlotID(hLeft->serial)) - (hero->stacksCount() == 1  &&  hero->needsLastStack());
			else
				assert(0);

			slider->setAmount(newAmount / r1);
			slider->scrollTo(0);
			max->block(false);
			deal->block(false);
		}
		else if(itemsType[1] == EType::RESOURCE) //buying -> check if we can afford transaction
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

	if(side && itemsType[0] != EType::PLAYER) //items[1] selection changed, recalculate offers
		initSubs(false);

	updateTraderText();
	redraw();
}

bool CMarketplaceWindow::printButtonFor(EMarketMode M) const
{
	if (!market->allowsTrade(M))
		return false;

	if (M == mode)
		return false;

	if ( M == EMarketMode::RESOURCE_RESOURCE || M == EMarketMode::RESOURCE_PLAYER)
	{
		auto * town = dynamic_cast<const CGTownInstance *>(market);

		if (town)
			return town->getOwner() == LOCPLINT->playerID;
		else
			return true;
	}
	else
	{
		return hero != nullptr;
	}
}

void CMarketplaceWindow::updateGarrison()
{
	if(mode != EMarketMode::CREATURE_RESOURCE)
		return;

	leftTradePanel->deleteSlots();
	leftTradePanel->updateSlots();
}

void CMarketplaceWindow::artifactsChanged(bool Left)
{
	assert(!Left);
	if(mode != EMarketMode::RESOURCE_ARTIFACT)
		return;
	
	rightTradePanel->deleteSlots();
	redraw();
}

std::string CMarketplaceWindow::updateSlotSubtitle(bool Left) const
{
	if(Left)
	{
		switch(itemsType[1])
		{
		case EType::RESOURCE:
		case EType::CREATURE:
			{
				int val = slider
					? slider->getValue() * r1
					: (((deal->isBlocked())) ? 0 : r1);

				return std::to_string(val);
			}
		case EType::ARTIFACT_INSTANCE:
			return ((deal->isBlocked()) ? "0" : "1");
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case EType::RESOURCE:
			if(slider)
				return std::to_string( slider->getValue() * r2 );
			else
				return std::to_string(r2);
		case EType::ARTIFACT_TYPE:
			return ((deal->isBlocked()) ? "0" : "1");
		case EType::PLAYER:
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
		case EType::RESOURCE:
			return Point(122, 448);
		case EType::CREATURE:
			return Point(128, 450);
		case EType::ARTIFACT_INSTANCE:
			return Point(134, 469);
		}
	}
	else
	{
		switch(itemsType[0])
		{
		case EType::RESOURCE:
			if(mode == EMarketMode::ARTIFACT_RESOURCE)
				return Point(410, 471);
			else
				return Point(410, 448);
		case EType::ARTIFACT_TYPE:
			return Point(411, 449);
		case EType::PLAYER:
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
