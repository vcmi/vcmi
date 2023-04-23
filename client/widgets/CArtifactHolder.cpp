/*
 * CArtifactHolder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactHolder.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"

#include "Buttons.h"
#include "CComponent.h"

#include "../windows/CHeroWindow.h"
#include "../windows/CSpellWindow.h"
#include "../windows/GUIClasses.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"

#include "../../lib/CArtHandler.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

void CArtPlace::setInternals(const CArtifactInstance * artInst)
{
	baseType = -1; // By default we don't store any component
	ourArt = artInst;
	if(!artInst)
	{
		image->disable();
		text.clear();
		hoverText = CGI->generaltexth->allTexts[507];
		return;
	}
	image->enable();
	image->setFrame(artInst->artType->getIconIndex());
	if(artInst->getTypeId() == ArtifactID::SPELL_SCROLL)
	{
		auto spellID = artInst->getScrollSpellID();
		if(spellID.num >= 0)
		{
			// Add spell component info (used to provide a pic in r-click popup)
			baseType = CComponent::spell;
			type = spellID;
			bonusValue = 0;
		}
	}
	else
	{
		baseType = CComponent::artifact;
		type = artInst->getTypeId();
		bonusValue = 0;
	}
	text = artInst->getDescription();
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * Art) 
	: ourArt(Art)
{
	image = nullptr;
	pos += position;
	pos.w = pos.h = 44;
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	LRClickableAreaWTextComp::clickLeft(down, previousState);
}

void CArtPlace::clickRight(tribool down, bool previousState)
{
	LRClickableAreaWTextComp::clickRight(down, previousState);
}

const CArtifactInstance * CArtPlace::getArt()
{
	return ourArt;
}

CCommanderArtPlace::CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * Art)
	: CArtPlace(position, Art),
	commanderOwner(commanderOwner),
	commanderSlotID(artSlot.num)
{
	createImage();
	setArtifact(Art);
}

void CCommanderArtPlace::createImage()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	int imageIndex = 0;
	if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>("artifact", imageIndex);
	if(!ourArt)
		image->disable();
}

void CCommanderArtPlace::returnArtToHeroCallback()
{
	ArtifactPosition artifactPos = commanderSlotID;
	ArtifactPosition freeSlot = ArtifactUtils::getArtBackpackPosition(commanderOwner, ourArt->getTypeId());
	if(freeSlot == ArtifactPosition::PRE_FIRST)
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
	}
	else
	{
		ArtifactLocation src(commanderOwner->commander.get(), artifactPos);
		ArtifactLocation dst(commanderOwner, freeSlot);

		if(ourArt->canBePutAt(dst, true))
		{
			LOCPLINT->cb->swapArtifacts(src, dst);
			setArtifact(nullptr);
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickLeft(tribool down, bool previousState)
{
	if(ourArt && text.size() && down)
		LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
}

void CCommanderArtPlace::clickRight(tribool down, bool previousState)
{
	if(ourArt && text.size() && down)
		CArtPlace::clickRight(down, previousState);
}

void CCommanderArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
}

CHeroArtPlace::CHeroArtPlace(Point position, const CArtifactInstance * Art)
	: CArtPlace(position, Art),
	locked(false),
	marked(false)
{
	createImage();
}

void CHeroArtPlace::lockSlot(bool on)
{
	if(locked == on)
		return;

	locked = on;

	if(on)
		image->setFrame(ArtifactID::ART_LOCK);
	else if(ourArt)
		image->setFrame(ourArt->artType->getIconIndex());
	else
		image->setFrame(0);
}

bool CHeroArtPlace::isLocked()
{
	return locked;
}

void CHeroArtPlace::selectSlot(bool on)
{
	if(marked == on)
		return;

	marked = on;
	if(on)
		selection->enable();
	else
		selection->disable();
}

bool CHeroArtPlace::isMarked() const
{
	return marked;
}

void CHeroArtPlace::clickLeft(tribool down, bool previousState)
{
	if(down || !previousState)
		return;

	if(leftClickCallback)
		leftClickCallback(*this);
}

void CHeroArtPlace::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(rightClickCallback)
			rightClickCallback(*this);
	}
}

void CHeroArtPlace::showAll(SDL_Surface* to)
{
	if(ourArt)
	{
		CIntObject::showAll(to);
	}

	if(marked && active)
	{
		// Draw vertical bars.
		for(int i = 0; i < pos.h; ++i)
		{
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x, pos.y + i, 240, 220, 120);
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + pos.w - 1, pos.y + i, 240, 220, 120);
		}

		// Draw horizontal bars.
		for(int i = 0; i < pos.w; ++i)
		{
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + i, pos.y, 240, 220, 120);
			CSDL_Ext::putPixelWithoutRefresh(to, pos.x + i, pos.y + pos.h - 1, 240, 220, 120);
		}
	}
}

void CHeroArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
	if(art)
	{
		image->setFrame(locked ? ArtifactID::ART_LOCK : art->artType->getIconIndex());

		if(locked) // Locks should appear as empty.
			hoverText = CGI->generaltexth->allTexts[507];
		else
			hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % ourArt->artType->getNameTranslated());
	}
	else
	{
		lockSlot(false);
	}
}

void CHeroArtPlace::addCombinedArtInfo(std::map<const CArtifact*, int> & arts)
{
	for(const auto combinedArt : arts)
	{
		std::string artList;
		text += "\n\n";
		text += "{" + combinedArt.first->getNameTranslated() + "}";
		if(arts.size() == 1)
		{
			for(const auto part : *combinedArt.first->constituents)
				artList += "\n" + part->getNameTranslated();
		}
		text += " (" + boost::str(boost::format("%d") % combinedArt.second) + " / " +
			boost::str(boost::format("%d") % combinedArt.first->constituents->size()) + ")" + artList;
	}
}

void CHeroArtPlace::createImage()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	si32 imageIndex = 0;

	if(locked)
		imageIndex = ArtifactID::ART_LOCK;
	else if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>("artifact", imageIndex);
	if(!ourArt)
		image->disable();

	selection = std::make_shared<CAnimImage>("artifact", ArtifactID::ART_SELECTION);
	selection->disable();
}

CArtifactsOfHeroBase::CArtifactsOfHeroBase()
	: backpackPos(0),
	curHero(nullptr)
{
}

CArtifactsOfHeroBase::~CArtifactsOfHeroBase()
{
	// TODO: cursor handling is CWindowWithArtifacts level. Should be moved when trading, kingdom and hero window are reworked
	// This will interfere with the implementation of a separate backpack window
	CCS->curh->dragAndDropCursor(nullptr);

	// Artifact located in artifactsTransitionPos should be returned
	if(getPickedArtifact())
	{
		auto slot = ArtifactUtils::getArtAnyPosition(curHero, curHero->artifactsTransitionPos.begin()->artifact->getTypeId());
		if(slot == ArtifactPosition::PRE_FIRST)
		{
			LOCPLINT->cb->eraseArtifactByClient(ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS));
		}
		else
		{
			LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS), ArtifactLocation(curHero, slot));
		}
	}
}

void CArtifactsOfHeroBase::init(
	CHeroArtPlace::ClickHandler lClickCallback,
	CHeroArtPlace::ClickHandler rClickCallback,
	const Point & position,
	BpackScrollHandler scrollHandler)
{
	// CArtifactsOfHeroBase::init may be transform to CArtifactsOfHeroBase::CArtifactsOfHeroBase if OBJECT_CONSTRUCTION_CAPTURING is removed
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	pos += position;
	for(int g = 0; g < GameConstants::BACKPACK_START; g++)
	{
		artWorn[ArtifactPosition(g)] = std::make_shared<CHeroArtPlace>(slotPos[g]);
	}
	backpack.clear();
	for(int s = 0; s < 5; s++)
	{
		auto artPlace = std::make_shared<CHeroArtPlace>(Point(403 + 46 * s, 365));
		backpack.push_back(artPlace);
	}
	for(auto artPlace : artWorn)
	{
		artPlace.second->slot = artPlace.first;
		artPlace.second->setArtifact(nullptr);
		artPlace.second->leftClickCallback = lClickCallback;
		artPlace.second->rightClickCallback = rClickCallback;
	}
	for(auto artPlace : backpack)
	{
		artPlace->setArtifact(nullptr);
		artPlace->leftClickCallback = lClickCallback;
		artPlace->rightClickCallback = rClickCallback;
	}
	leftBackpackRoll = std::make_shared<CButton>(Point(379, 364), "hsbtns3.def", CButton::tooltip(), [scrollHandler]() { scrollHandler(-1); }, SDLK_LEFT);
	rightBackpackRoll = std::make_shared<CButton>(Point(632, 364), "hsbtns5.def", CButton::tooltip(), [scrollHandler]() { scrollHandler(+1); }, SDLK_RIGHT);
	leftBackpackRoll->block(true);
	rightBackpackRoll->block(true);
}

void CArtifactsOfHeroBase::leftClickArtPlace(CHeroArtPlace & artPlace)
{
	if(leftClickCallback)
		leftClickCallback(*this, artPlace);
}

void CArtifactsOfHeroBase::rightClickArtPlace(CHeroArtPlace & artPlace)
{
	if(rightClickCallback)
		rightClickCallback(*this, artPlace);
}

void CArtifactsOfHeroBase::setHero(const CGHeroInstance * hero)
{
	curHero = hero;
	if(curHero->artifactsInBackpack.size() > 0)
		backpackPos %= curHero->artifactsInBackpack.size();
	else
		backpackPos = 0;

	for(auto slot : artWorn)
	{
		setSlotData(slot.second, slot.first, *curHero);
	}
	scrollBackpackForArtSet(0, *curHero);
}

const CGHeroInstance * CArtifactsOfHeroBase::getHero() const
{
	return curHero;
}

void CArtifactsOfHeroBase::scrollBackpack(int offset)
{
	scrollBackpackForArtSet(offset, *curHero);
	safeRedraw();
}

void CArtifactsOfHeroBase::scrollBackpackForArtSet(int offset, const CArtifactSet & artSet)
{
	// offset==-1 => to left; offset==1 => to right
	using slotInc = std::function<ArtifactPosition(ArtifactPosition&)>;
	auto artsInBackpack = static_cast<int>(artSet.artifactsInBackpack.size());
	auto scrollingPossible = artsInBackpack > backpack.size();

	slotInc inc_straight = [](ArtifactPosition & slot) -> ArtifactPosition
	{
		return slot + 1;
	};
	slotInc inc_ring = [artsInBackpack](ArtifactPosition & slot) -> ArtifactPosition
	{
		return ArtifactPosition(GameConstants::BACKPACK_START + (slot - GameConstants::BACKPACK_START + 1) % artsInBackpack);
	};
	slotInc inc;
	if(scrollingPossible)
		inc = inc_ring;
	else
		inc = inc_straight;

	backpackPos += offset;
	if(backpackPos < 0)
		backpackPos += artsInBackpack;

	if(artsInBackpack)
		backpackPos %= artsInBackpack;

	auto slot = ArtifactPosition(GameConstants::BACKPACK_START + backpackPos);
	for(auto artPlace : backpack)
	{
		setSlotData(artPlace, slot, artSet);
		slot = inc(slot);
	}

	// Blocking scrolling if there is not enough artifacts to scroll
	leftBackpackRoll->block(!scrollingPossible);
	rightBackpackRoll->block(!scrollingPossible);
}

void CArtifactsOfHeroBase::safeRedraw()
{
	if(active)
	{
		if(parent)
			parent->redraw();
		else
			redraw();
	}
}

void CArtifactsOfHeroBase::markPossibleSlots(const CArtifactInstance * art, bool assumeDestRemoved)
{
	for(auto artPlace : artWorn)
		artPlace.second->selectSlot(art->artType->canBePutAt(curHero, artPlace.second->slot, assumeDestRemoved));
}

void CArtifactsOfHeroBase::unmarkSlots()
{
	for(auto & artPlace : artWorn)
		artPlace.second->selectSlot(false);

	for(auto & artPlace : backpack)
		artPlace->selectSlot(false);
}

CArtifactsOfHeroBase::ArtPlacePtr CArtifactsOfHeroBase::getArtPlace(const ArtifactPosition & slot)
{
	if(ArtifactUtils::isSlotEquipment(slot))
	{
		if(artWorn.find(slot) == artWorn.end())
		{
			logGlobal->error("CArtifactsOfHero::getArtPlace: invalid slot %d", slot);
			return nullptr;
		}
		return artWorn[slot];
	}
	if(ArtifactUtils::isSlotBackpack(slot))
	{
		for(ArtPlacePtr artPlace : backpack)
			if(artPlace->slot == slot)
				return artPlace;
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}

void CArtifactsOfHeroBase::updateWornSlots()
{
	for(auto place : artWorn)
		updateSlot(place.first);
}

void CArtifactsOfHeroBase::updateBackpackSlots()
{
	for(auto artPlace : backpack)
		updateSlot(artPlace->slot);
	scrollBackpackForArtSet(0, *curHero);
}

void CArtifactsOfHeroBase::updateSlot(const ArtifactPosition & slot)
{
	setSlotData(getArtPlace(slot), slot, *curHero);
}

const CArtifactInstance * CArtifactsOfHeroBase::getPickedArtifact()
{
	// Returns only the picked up artifact. Not just highlighted like in the trading window.
	if(!curHero || curHero->artifactsTransitionPos.empty())
		return nullptr;
	else
		return curHero->getArt(ArtifactPosition::TRANSITION_POS);
}

void CArtifactsOfHeroBase::setSlotData(ArtPlacePtr artPlace, const ArtifactPosition & slot, const CArtifactSet & artSet)
{
	// Spurious call from artifactMoved in attempt to update hidden backpack slot
	if(!artPlace && ArtifactUtils::isSlotBackpack(slot))
	{
		return;
	}

	artPlace->slot = slot;
	if(auto slotInfo = artSet.getSlot(slot))
	{
		artPlace->lockSlot(slotInfo->locked);
		artPlace->setArtifact(slotInfo->artifact);
		if(!slotInfo->artifact->canBeDisassembled())
		{
			// If the artifact is part of at least one combined artifact, add additional information
			std::map<const CArtifact*, int> arts;
			for(const auto combinedArt : slotInfo->artifact->artType->constituentOf)
			{
				arts.insert(std::pair(combinedArt, 0));
				for(const auto part : *combinedArt->constituents)
					if(artSet.hasArt(part->getId(), true))
						arts.at(combinedArt)++;
			}
			artPlace->addCombinedArtInfo(arts);
		}
	}
	else
	{
		artPlace->setArtifact(nullptr);
	}
}

CArtifactsOfHeroMain::CArtifactsOfHeroMain(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1),
		std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1),
		position,
		std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
}

void CArtifactsOfHeroMain::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroMain::pickUpArtifact(CHeroArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, artPlace.slot),
		ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS));
}

CArtifactsOfHeroKingdom::CArtifactsOfHeroKingdom(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
	std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll)
{
	artWorn = ArtWorn;
	backpack = Backpack;
	leftBackpackRoll = leftScroll;
	rightBackpackRoll = rightScroll;

	for(auto artPlace : artWorn)
	{
		artPlace.second->slot = artPlace.first;
		artPlace.second->setArtifact(nullptr);
		artPlace.second->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace.second->rightClickCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
	}
	for(auto artPlace : backpack)
	{
		artPlace->setArtifact(nullptr);
		artPlace->leftClickCallback = std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1);
		artPlace->rightClickCallback = std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1);
	}
	leftBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, -1));
	rightBackpackRoll->addCallback(std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, +1));
}

void CArtifactsOfHeroKingdom::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}

void CArtifactsOfHeroKingdom::pickUpArtifact(CHeroArtPlace & artPlace)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, artPlace.slot),
		ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS));
}

CArtifactsOfHeroAltar::CArtifactsOfHeroAltar(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1), 
		std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1), 
		position,
		std::bind(&CArtifactsOfHeroAltar::scrollBackpack, this, _1));
	visibleArtSet = std::make_shared<CArtifactFittingSet>(ArtBearer::ArtBearer::HERO);
	pickedArtFromSlot = ArtifactPosition::PRE_FIRST;
};

void CArtifactsOfHeroAltar::setHero(const CGHeroInstance * hero)
{
	if(hero)
	{
		visibleArtSet->artifactsWorn = hero->artifactsWorn;
		visibleArtSet->artifactsInBackpack = hero->artifactsInBackpack;
		CArtifactsOfHeroBase::setHero(hero);
	}
}

void CArtifactsOfHeroAltar::updateWornSlots()
{
	for(auto place : artWorn)
		setSlotData(getArtPlace(place.first), place.first, *visibleArtSet);
}

void CArtifactsOfHeroAltar::updateBackpackSlots()
{
	for(auto artPlace : backpack)
		setSlotData(getArtPlace(artPlace->slot), artPlace->slot, *visibleArtSet);
}

void CArtifactsOfHeroAltar::scrollBackpack(int offset)
{
	CArtifactsOfHeroBase::scrollBackpackForArtSet(offset, *visibleArtSet);
	safeRedraw();
}

void CArtifactsOfHeroAltar::pickUpArtifact(CHeroArtPlace & artPlace)
{
	if(const auto art = artPlace.getArt())
	{
		pickedArtFromSlot = artPlace.slot;
		artPlace.setArtifact(nullptr);
		deleteFromVisible(art);
		if(ArtifactUtils::isSlotBackpack(pickedArtFromSlot))
			pickedArtFromSlot = curHero->getSlotByInstance(art);
		assert(pickedArtFromSlot != ArtifactPosition::PRE_FIRST);
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, pickedArtFromSlot), ArtifactLocation(curHero, ArtifactPosition::TRANSITION_POS));
	}
}

void CArtifactsOfHeroAltar::swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
	const auto pickedArtInst = curHero->getArt(ArtifactPosition::TRANSITION_POS);
	assert(pickedArtInst);
	visibleArtSet->putArtifact(dstLoc.slot, const_cast<CArtifactInstance*>(pickedArtInst));
}

void CArtifactsOfHeroAltar::pickedArtMoveToAltar(const ArtifactPosition & slot)
{
	if(ArtifactUtils::isSlotBackpack(slot) || ArtifactUtils::isSlotEquipment(slot) || slot == ArtifactPosition::TRANSITION_POS)
	{
		assert(!curHero->getSlot(pickedArtFromSlot)->getArt());
		LOCPLINT->cb->swapArtifacts(ArtifactLocation(curHero, slot), ArtifactLocation(curHero, pickedArtFromSlot));
		pickedArtFromSlot = ArtifactPosition::PRE_FIRST;
	}
}

void CArtifactsOfHeroAltar::deleteFromVisible(const CArtifactInstance * artInst)
{
	const auto slot = visibleArtSet->getSlotByInstance(artInst);
	visibleArtSet->removeArtifact(slot);
	if(ArtifactUtils::isSlotBackpack(slot))
	{
		scrollBackpackForArtSet(0, *visibleArtSet);
	}
	else
	{
		if(artInst->canBeDisassembled())
		{
			for(const auto part : dynamic_cast<const CCombinedArtifactInstance*>(artInst)->constituentsInfo)
			{
				if(part.slot != ArtifactPosition::PRE_FIRST)
					getArtPlace(part.slot)->setArtifact(nullptr);
			}
		}
	}
}

CArtifactsOfHeroMarket::CArtifactsOfHeroMarket(const Point & position)
{
	init(
		std::bind(&CArtifactsOfHeroBase::leftClickArtPlace, this, _1), 
		std::bind(&CArtifactsOfHeroBase::rightClickArtPlace, this, _1), 
		position,
		std::bind(&CArtifactsOfHeroMarket::scrollBackpack, this, _1));
};

void CArtifactsOfHeroMarket::scrollBackpack(int offset)
{
	CArtifactsOfHeroBase::scrollBackpackForArtSet(offset, *curHero);

	// We may have highlight on one of backpack artifacts
	if(selectArtCallback)
	{
		for(auto & artPlace : backpack)
		{
			if(artPlace->isMarked())
			{
				selectArtCallback(artPlace.get());
				break;
			}
		}
	}
	safeRedraw();
}

void CWindowWithArtifacts::addSet(CArtifactsOfHeroPtr artSet)
{
	artSets.emplace_back(artSet);
	std::visit([this](auto artSetWeak)
		{
			auto artSet = artSetWeak.lock();
			artSet->leftClickCallback = std::bind(&CWindowWithArtifacts::leftClickArtPlaceHero, this, _1, _2);
			artSet->rightClickCallback = std::bind(&CWindowWithArtifacts::rightClickArtPlaceHero, this, _1, _2);
		}, artSet);
}

const CGHeroInstance * CWindowWithArtifacts::getHeroPickedArtifact()
{
	auto res = getState();
	if(res.has_value())
		return std::get<const CGHeroInstance*>(res.value());
	else
		return nullptr;
}

const CArtifactInstance * CWindowWithArtifacts::getPickedArtifact()
{
	auto res = getState();
	if(res.has_value())
		return std::get<const CArtifactInstance*>(res.value());
	else
		return nullptr;
}

void CWindowWithArtifacts::leftClickArtPlaceHero(CArtifactsOfHeroBase & artsInst, CHeroArtPlace & artPlace)
{
	const auto artSetWeak = findAOHbyRef(artsInst);
	assert(artSetWeak.has_value());

	if(artPlace.isLocked())
		return;

	const auto checkSpecialArts = [](const CGHeroInstance * hero, CHeroArtPlace & artPlace) -> bool
	{
		if(artPlace.getArt()->getTypeId() == ArtifactID::SPELLBOOK)
		{
			GH.pushIntT<CSpellWindow>(hero, LOCPLINT, LOCPLINT->battleInt.get());
			return false;
		}
		if(artPlace.getArt()->getTypeId() == ArtifactID::CATAPULT)
		{
			// The Catapult must be equipped
			std::vector<std::shared_ptr<CComponent>> catapult(1, std::make_shared<CComponent>(CComponent::artifact, 3, 0));
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[312], catapult);
			return false;
		}
		return true;
	};

	std::visit(
		[checkSpecialArts, this, &artPlace](auto artSetWeak) -> void
		{
			const auto artSetPtr = artSetWeak.lock();
			constexpr auto isMainWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMain>>;
			constexpr auto isKingdomWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>;
			constexpr auto isAltarWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroAltar>>;
			constexpr auto isMarketWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMarket>>;

			// Hero(Main, Exchange) window, Kingdom window, Altar window left click handler
			if constexpr(isMainWindow || isKingdomWindow || isAltarWindow)
			{
				const auto pickedArtInst = getPickedArtifact();
				const auto heroPickedArt = getHeroPickedArtifact();
				const auto hero = artSetPtr->getHero();

				if(pickedArtInst)
				{
					auto srcLoc = ArtifactLocation(heroPickedArt, ArtifactPosition::TRANSITION_POS);
					auto dstLoc = ArtifactLocation(hero, artPlace.slot);
					auto isTransferAllowed = false;

					if(ArtifactUtils::isSlotBackpack(artPlace.slot))
					{
						if(pickedArtInst->artType->isBig())
						{
							// War machines cannot go to backpack
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[153]) % pickedArtInst->artType->getNameTranslated()));
						}
						else
						{
							if(ArtifactUtils::isBackpackFreeSlots(heroPickedArt))
								isTransferAllowed = true;
							else
								LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
						}
					}
					// Check if artifact transfer is possible
					else if(pickedArtInst->canBePutAt(dstLoc, true) && (!artPlace.getArt() || hero->tempOwner == LOCPLINT->playerID))
					{
						isTransferAllowed = true;
					}
					if constexpr(isKingdomWindow)
					{
						if(hero != heroPickedArt)
							isTransferAllowed = false;
					}
					if(isTransferAllowed)
						artSetPtr->swapArtifacts(srcLoc, dstLoc);
				}
				else
				{
					if(artPlace.getArt())
					{			
						if(artSetPtr->getHero()->tempOwner == LOCPLINT->playerID)
						{
							if(checkSpecialArts(hero, artPlace))
								artSetPtr->pickUpArtifact(artPlace);
						}
						else
						{
							for(const auto artSlot : ArtifactUtils::unmovableSlots())
								if(artPlace.slot == artSlot)
								{
									LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]);
									break;
								}
						}
					}
				}
			}
			// Market window left click handler
			else if constexpr(isMarketWindow)
			{
				if(artSetPtr->selectArtCallback && artPlace.getArt())
				{
					if(artPlace.getArt()->artType->isTradable())
					{
						artSetPtr->unmarkSlots();
						artPlace.selectSlot(true);
						artSetPtr->selectArtCallback(&artPlace);
					}
					else
					{
						// This item can't be traded
						LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]);
					}
				}
			}
		}, artSetWeak.value());
}

void CWindowWithArtifacts::rightClickArtPlaceHero(CArtifactsOfHeroBase & artsInst, CHeroArtPlace & artPlace)
{
	const auto artSetWeak = findAOHbyRef(artsInst);
	assert(artSetWeak.has_value());

	if(artPlace.isLocked())
		return;

	std::visit(
		[&artPlace](auto artSetWeak) -> void
		{
			const auto artSetPtr = artSetWeak.lock();
			constexpr auto isMainWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMain>>;
			constexpr auto isKingdomWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>;
			constexpr auto isAltarWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroAltar>>;
			constexpr auto isMarketWindow = std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMarket>>;

			// Hero(Main, Exchange) window, Kingdom window right click handler
			if constexpr(isMainWindow || isKingdomWindow)
			{
				if(artPlace.getArt())
				{
					if(ArtifactUtils::askToDisassemble(artSetPtr->getHero(), artPlace.slot))
					{
						return;
					}
					if(ArtifactUtils::askToAssemble(artSetPtr->getHero(), artPlace.slot))
					{
						return;
					}
					if(artPlace.text.size())
						artPlace.LRClickableAreaWTextComp::clickRight(boost::logic::tribool::true_value, false);
				}
			}
			// Altar window, Market window right click handler
			else if constexpr(isAltarWindow || isMarketWindow)
			{
				if(artPlace.getArt() && artPlace.text.size())
					artPlace.LRClickableAreaWTextComp::clickRight(boost::logic::tribool::true_value, false);
			}
		}, artSetWeak.value());
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation & artLoc)
{
	updateSlots(artLoc.slot);
}

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw)
{
	auto curState = getState();
	if(!curState.has_value())
		// Transition state. Nothing to do here. Just skip. Need to wait for final state.
		return;

	// When moving one artifact onto another it leads to two art movements: dst->TRANSITION_POS; src->dst
	// However after first movement we pick the art from TRANSITION_POS and the second movement coming when
	// we have a different artifact may look surprising... but it's valid.

	auto pickedArtInst = std::get<const CArtifactInstance*>(curState.value());
	assert(srcLoc.isHolder(std::get<const CGHeroInstance*>(curState.value())));
	assert(srcLoc.getArt() == pickedArtInst);

	auto artifactMovedBody = [this, withRedraw, &srcLoc, &destLoc, &pickedArtInst](auto artSetWeak) -> void
	{
		auto artSetPtr = artSetWeak.lock();
		if(artSetPtr)
		{
			const auto hero = artSetPtr->getHero();
			if(artSetPtr->active)
			{
				if(pickedArtInst)
				{
					CCS->curh->dragAndDropCursor("artifact", pickedArtInst->artType->getIconIndex());
					if(srcLoc.isHolder(hero) || !std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>)
						artSetPtr->markPossibleSlots(pickedArtInst, hero->tempOwner == LOCPLINT->playerID);
				}
				else
				{
					artSetPtr->unmarkSlots();
					CCS->curh->dragAndDropCursor(nullptr);
				}
			}
			if(withRedraw)
			{
				artSetPtr->updateWornSlots();
				artSetPtr->updateBackpackSlots();

				// Update arts bonuses on window.
				// TODO rework this part when CHeroWindow and CExchangeWindow are reworked
				if(auto * chw = dynamic_cast<CHeroWindow*>(this))
				{
					chw->update(hero, true);
				}
				else if(auto * cew = dynamic_cast<CExchangeWindow*>(this))
				{
					cew->updateWidgets();
				}
				artSetPtr->safeRedraw();
			}

			// Make sure the status bar is updated so it does not display old text
			if(destLoc.isHolder(hero))
			{
				if(auto artPlace = artSetPtr->getArtPlace(destLoc.slot))
					artPlace->hover(true);
			}
		}
	};

	for(auto artSetWeak : artSets)
		std::visit(artifactMovedBody, artSetWeak);
}

void CWindowWithArtifacts::artifactDisassembled(const ArtifactLocation & artLoc)
{
	updateSlots(artLoc.slot);
}

void CWindowWithArtifacts::artifactAssembled(const ArtifactLocation & artLoc)
{
	updateSlots(artLoc.slot);
}

void CWindowWithArtifacts::updateSlots(const ArtifactPosition & slot)
{
	auto updateSlotBody = [slot](auto artSetWeak) -> void
	{
		if(const auto artSetPtr = artSetWeak.lock())
		{
			if(ArtifactUtils::isSlotEquipment(slot))
				artSetPtr->updateWornSlots();
			else if(ArtifactUtils::isSlotBackpack(slot))
				artSetPtr->updateBackpackSlots();

			artSetPtr->safeRedraw();
		}
	};

	for(auto artSetWeak : artSets)
		std::visit(updateSlotBody, artSetWeak);
}

std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> CWindowWithArtifacts::getState()
{
	const CArtifactInstance * artInst = nullptr;
	const CGHeroInstance * hero = nullptr;
	size_t pickedCnt = 0;

	auto getHeroArtBody = [&hero, &artInst, &pickedCnt](auto artSetWeak) -> void
	{
		auto artSetPtr = artSetWeak.lock();
		if(artSetPtr)
		{
			if(const auto art = artSetPtr->getPickedArtifact())
			{
				artInst = art;
				hero = artSetPtr->getHero();
				pickedCnt += hero->artifactsTransitionPos.size();
			}
		}
	};
	for(auto artSetWeak : artSets)
		std::visit(getHeroArtBody, artSetWeak);

	// The state is possible when the hero has placed an artifact in the ArtifactPosition::TRANSITION_POS,
	// and the previous artifact has not yet removed from the ArtifactPosition::TRANSITION_POS.
	// This is a transitional state. Then return nullopt.
	if(pickedCnt > 1)
		return std::nullopt;
	else
		return std::make_tuple(hero, artInst);
}

std::optional<CWindowWithArtifacts::CArtifactsOfHeroPtr> CWindowWithArtifacts::findAOHbyRef(CArtifactsOfHeroBase & artsInst)
{
	std::optional<CArtifactsOfHeroPtr> res;

	auto findAOHBody = [&res, &artsInst](auto & artSetWeak) -> void
	{
		if(&artsInst == artSetWeak.lock().get())
			res = artSetWeak;
	};

	for(auto artSetWeak : artSets)
	{
		std::visit(findAOHBody, artSetWeak);
		if(res.has_value())
			return res;
	}
	return res;
}

bool ArtifactUtils::askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);
	auto assemblyPossibilities = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId(), ArtifactUtils::isSlotEquipment(slot));

	for(const auto combinedArt : assemblyPossibilities)
	{
		LOCPLINT->showArtifactAssemblyDialog(
			art->artType,
			combinedArt,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, true, combinedArt->getId()));

		if(assemblyPossibilities.size() > 2)
			logGlobal->warn("More than one possibility of assembling on %s... taking only first", art->artType->getNameTranslated());
		return true;
	}
	return false;
}

bool ArtifactUtils::askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(art->canBeDisassembled())
	{
		if(ArtifactUtils::isSlotBackpack(slot) && !ArtifactUtils::isBackpackFreeSlots(hero, art->artType->constituents->size() - 1))
			return false;

		LOCPLINT->showArtifactAssemblyDialog(
			art->artType,
			nullptr,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, false, ArtifactID()));
		return true;
	}
	return false;
}
