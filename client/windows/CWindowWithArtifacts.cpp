/*
 * CWindowWithArtifacts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CWindowWithArtifacts.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/WindowHandler.h"

#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"

#include "../widgets/CComponent.h"

#include "../windows/CSpellWindow.h"
#include "../windows/CHeroBackpackWindow.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CConfigHandler.h"

#include "../../CCallback.h"

CWindowWithArtifacts::CWindowWithArtifacts(const std::vector<ArtifactsOfHeroVar> * artSets)
{
	if(artSets)
		this->artSets.insert(this->artSets.end(), artSets->begin(), artSets->end());
}

void CWindowWithArtifacts::addSet(ArtifactsOfHeroVar newArtSet)
{
	artSets.emplace_back(newArtSet);
}

void CWindowWithArtifacts::addSetAndCallbacks(ArtifactsOfHeroVar newArtSet)
{
	addSet(newArtSet);
	std::visit([this](auto artSet)
		{
			if constexpr(std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroMarket>>)
			{
				artSet->clickPressedCallback = [artSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					artSet->onClickPressedArtPlace(artPlace);
				};
			}
			if constexpr(std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				artSet->clickPressedCallback = [this, artSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					if(const auto curHero = artSet->getHero())
						swapArtifactAndClose(*artSet, artPlace.slot, ArtifactLocation(curHero->id, artSet->getFilterSlot()));
				};
			}
			if constexpr(
					std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroMain>> ||
					std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroKingdom>> ||
					std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroAltar>> ||
					std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroBackpack>>)
			{
				artSet->clickPressedCallback = [this, artSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					if(const auto curHero = artSet->getHero())
						clickPressedOnArtPlace(*curHero, artPlace.slot,
							!std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroKingdom>>,
							std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroAltar>>,
							std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroBackpack>>);
				};
			}
			if constexpr(
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroMarket>> ||
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				artSet->showPopupCallback = [this](CArtPlace & artPlace, const Point & cursorPosition)
				{
					showArifactInfo(artPlace, cursorPosition);
				};
			}
			if constexpr(
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroKingdom>> ||
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroAltar>> ||
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroBackpack>>)
			{
				artSet->showPopupCallback = [this, artSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					showArtifactAssembling(*artSet, artPlace, cursorPosition);
				};
			}
			if constexpr(
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSet), std::shared_ptr<CArtifactsOfHeroKingdom>>)
			{
				artSet->gestureCallback = [this, artSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					if(const auto curHero = artSet->getHero())
						showQuickBackpackWindow(*curHero, artPlace.slot, cursorPosition);
				};
			}
		}, newArtSet);
}

void CWindowWithArtifacts::addCloseCallback(const CloseCallback & callback)
{
	closeCallback = callback;
}

const CGHeroInstance * CWindowWithArtifacts::getHeroPickedArtifact()
{
	const CGHeroInstance * hero = nullptr;

	for(auto & artSet : artSets)
		std::visit([&hero](auto artSetPtr)
			{
				if(const auto pickedArt = artSetPtr->getHero()->getArt(ArtifactPosition::TRANSITION_POS))
				{
					hero = artSetPtr->getHero();
					return;
				}
			}, artSet);
	return hero;
}

const CArtifactInstance * CWindowWithArtifacts::getPickedArtifact()
{
	const CArtifactInstance * art = nullptr;

	for(auto & artSet : artSets)
		std::visit([&art](auto artSetPtr)
			{
				if(const auto pickedArt = artSetPtr->getHero()->getArt(ArtifactPosition::TRANSITION_POS))
				{
					art = pickedArt;
					return;
				}
		}, artSet);
	return art;
}

void CWindowWithArtifacts::clickPressedOnArtPlace(const CGHeroInstance & hero, const ArtifactPosition & slot,
	bool allowExchange, bool altarTrading, bool closeWindow)
{
	if(!LOCPLINT->makingTurn)
		return;

	if(const auto heroArtOwner = getHeroPickedArtifact())
	{
		if(allowExchange || hero.id == heroArtOwner->id)
			putPickedArtifact(hero, slot);
	}
	else if(auto art = hero.getArt(slot))
	{
		if(hero.getOwner() == LOCPLINT->playerID)
		{
			if(checkSpecialArts(*art, hero, altarTrading))
				onClickPressedCommonArtifact(hero, slot, closeWindow);
		}
		else
		{
			for(const auto & artSlot : ArtifactUtils::unmovableSlots())
				if(slot == artSlot)
				{
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]);
					break;
				}
		}
	}
}

void CWindowWithArtifacts::swapArtifactAndClose(const CArtifactsOfHeroBase & artsInst, const ArtifactPosition & slot,
	const ArtifactLocation & dstLoc) const
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(artsInst.getHero()->id, slot), dstLoc);
	if(closeCallback)
		closeCallback();
}

void CWindowWithArtifacts::showArtifactAssembling(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace,
	const Point & cursorPosition) const
{
	if(artsInst.getArt(artPlace.slot))
	{
		if(ArtifactUtilsClient::askToDisassemble(artsInst.getHero(), artPlace.slot))
			return;
		if(ArtifactUtilsClient::askToAssemble(artsInst.getHero(), artPlace.slot))
			return;
		if(artPlace.text.size())
			artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
	}
}

void CWindowWithArtifacts::showArifactInfo(CArtPlace & artPlace, const Point & cursorPosition) const
{
	if(artPlace.getArt() && artPlace.text.size())
		artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
}

void CWindowWithArtifacts::showQuickBackpackWindow(const CGHeroInstance & hero, const ArtifactPosition & slot,
	const Point & cursorPosition) const
{
	if(!settings["general"]["enableUiEnhancements"].Bool())
		return;

	GH.windows().createAndPushWindow<CHeroQuickBackpackWindow>(&hero, slot);
	auto backpackWindow = GH.windows().topWindow<CHeroQuickBackpackWindow>();
	backpackWindow->moveTo(cursorPosition - Point(1, 1));
	backpackWindow->fitToScreen(15);
}

void CWindowWithArtifacts::activate()
{
	if(const auto art = getPickedArtifact())
	{
		markPossibleSlots();
		setCursorAnimation(*art);
	}
	CWindowObject::activate();
}

void CWindowWithArtifacts::deactivate()
{
	CCS->curh->dragAndDropCursor(nullptr);
	CWindowObject::deactivate();
}

void CWindowWithArtifacts::enableArtifactsCostumeSwitcher() const
{
	for(auto & artSet : artSets)
		std::visit(
			[](auto artSetPtr)
			{
				if constexpr(std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroMain>>)
					artSetPtr->enableArtifactsCostumeSwitcher();
			}, artSet);
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation & artLoc)
{
	update();
}

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc)
{
	for(auto & artSet : artSets)
		std::visit([this](auto artSetPtr)
			{
				if(const auto pickedArtInst = getPickedArtifact())
				{
					markPossibleSlots();
					setCursorAnimation(*pickedArtInst);
				}
				else
				{
					artSetPtr->unmarkSlots();
					CCS->curh->dragAndDropCursor(nullptr);
				}
			}, artSet);
}

void CWindowWithArtifacts::artifactDisassembled(const ArtifactLocation & artLoc)
{
	update();
}

void CWindowWithArtifacts::artifactAssembled(const ArtifactLocation & artLoc)
{
	markPossibleSlots();
	update();
}

void CWindowWithArtifacts::update()
{
	for(auto & artSet : artSets)
		std::visit([](auto artSetPtr)
			{
				artSetPtr->updateWornSlots();
				artSetPtr->updateBackpackSlots();

				// Make sure the status bar is updated so it does not display old text
				if(auto artPlace = artSetPtr->getArtPlace(GH.getCursorPosition()))
					artPlace->hover(true);
			}, artSet);
}

void CWindowWithArtifacts::markPossibleSlots()
{
	if(const auto pickedArtInst = getPickedArtifact())
	{
		const auto heroArtOwner = getHeroPickedArtifact();
		auto artifactAssembledBody = [&pickedArtInst, &heroArtOwner](auto artSetPtr)
		{
			if(artSetPtr->isActive())
			{
				const auto hero = artSetPtr->getHero();
				if(heroArtOwner == hero || !std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroKingdom>>)
					artSetPtr->markPossibleSlots(pickedArtInst, hero->tempOwner == LOCPLINT->playerID);
			}
		};

		for(auto & artSet : artSets)
			std::visit(artifactAssembledBody, artSet);
	}
}

bool CWindowWithArtifacts::checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance & hero, bool isTrade) const
{
	const auto artId = artInst.getTypeId();
	
	if(artId == ArtifactID::SPELLBOOK)
	{
		GH.windows().createAndPushWindow<CSpellWindow>(&hero, LOCPLINT, LOCPLINT->battleInt.get());
		return false;
	}
	if(artId == ArtifactID::CATAPULT)
	{
		// The Catapult must be equipped
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[312],
			std::vector<std::shared_ptr<CComponent>>(1, std::make_shared<CComponent>(ComponentType::ARTIFACT, ArtifactID(ArtifactID::CATAPULT))));
		return false;
	}
	if(isTrade && !artInst.artType->isTradable())
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21],
			std::vector<std::shared_ptr<CComponent>>(1, std::make_shared<CComponent>(ComponentType::ARTIFACT, artId)));
		return false;
	}
	return true;
}

void CWindowWithArtifacts::setCursorAnimation(const CArtifactInstance & artInst)
{
	if(artInst.isScroll() && settings["general"]["enableUiEnhancements"].Bool())
	{
		assert(artInst.getScrollSpellID().num >= 0);
		const auto animation = GH.renderHandler().loadAnimation(AnimationPath::builtin("spellscr"));
		animation->load(artInst.getScrollSpellID().num);
		CCS->curh->dragAndDropCursor(animation->getImage(artInst.getScrollSpellID().num)->scaleFast(Point(44, 34)));
	}
	else
	{
		CCS->curh->dragAndDropCursor(AnimationPath::builtin("artifact"), artInst.artType->getIconIndex());
	}
}

void CWindowWithArtifacts::putPickedArtifact(const CGHeroInstance & curHero, const ArtifactPosition & targetSlot)
{
	const auto heroArtOwner = getHeroPickedArtifact();
	const auto pickedArt = getPickedArtifact();
	auto srcLoc = ArtifactLocation(heroArtOwner->id, ArtifactPosition::TRANSITION_POS);
	auto dstLoc = ArtifactLocation(curHero.id, targetSlot);

	if(ArtifactUtils::isSlotBackpack(dstLoc.slot))
	{
		if(pickedArt->artType->isBig())
		{
			// War machines cannot go to backpack
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[153]) % pickedArt->artType->getNameTranslated()));
		}
		else
		{
			if(ArtifactUtils::isBackpackFreeSlots(heroArtOwner))
				LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
			else
				LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
		}
	}
	// Check if artifact transfer is possible
	else if(pickedArt->canBePutAt(&curHero, dstLoc.slot, true) && (!curHero.getArt(targetSlot) || curHero.tempOwner == LOCPLINT->playerID))
	{
		LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
	}
}

void CWindowWithArtifacts::onClickPressedCommonArtifact(const CGHeroInstance & curHero, const ArtifactPosition & slot, bool closeWindow)
{
	assert(curHero.getArt(slot));
	auto srcLoc = ArtifactLocation(curHero.id, slot);
	auto dstLoc = ArtifactLocation(curHero.id, ArtifactPosition::TRANSITION_POS);

	if(GH.isKeyboardCmdDown())
	{
		for(auto & anotherSet : artSets)
		{
			if(std::holds_alternative<std::shared_ptr<CArtifactsOfHeroMain>>(anotherSet))
			{
				const auto anotherHeroEquipment = std::get<std::shared_ptr<CArtifactsOfHeroMain>>(anotherSet);
				if(curHero.id != anotherHeroEquipment->getHero()->id)
				{
					dstLoc.slot = ArtifactPosition::FIRST_AVAILABLE;
					dstLoc.artHolder = anotherHeroEquipment->getHero()->id;
					break;
				}
			}
			if(std::holds_alternative<std::shared_ptr<CArtifactsOfHeroAltar>>(anotherSet))
			{
				const auto heroEquipment = std::get<std::shared_ptr<CArtifactsOfHeroAltar>>(anotherSet);
				dstLoc.slot = ArtifactPosition::FIRST_AVAILABLE;
				dstLoc.artHolder = heroEquipment->altarId;
				break;
			}
		}
	}
	else if(GH.isKeyboardAltDown())
	{
		const auto artId = curHero.getArt(slot)->getTypeId();
		if(ArtifactUtils::isSlotEquipment(slot))
			dstLoc.slot = ArtifactUtils::getArtBackpackPosition(&curHero, artId);
		else if(ArtifactUtils::isSlotBackpack(slot))
			dstLoc.slot = ArtifactUtils::getArtEquippedPosition(&curHero, artId);
	}
	else if(closeWindow && closeCallback)
	{
		closeCallback();
	}
	if(dstLoc.slot != ArtifactPosition::PRE_FIRST)
		LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);

}
