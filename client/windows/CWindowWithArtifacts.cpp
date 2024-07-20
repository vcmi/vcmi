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

#include "CHeroWindow.h"
#include "CSpellWindow.h"
#include "CExchangeWindow.h"
#include "CHeroBackpackWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/WindowHandler.h"

#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"

#include "../widgets/CComponent.h"

#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CConfigHandler.h"

#include "../../CCallback.h"

CWindowWithArtifacts::CWindowWithArtifacts(const std::vector<CArtifactsOfHeroPtr> * artSets)
{
	if(artSets)
		this->artSets.insert(this->artSets.end(), artSets->begin(), artSets->end());
}

void CWindowWithArtifacts::addSet(const std::shared_ptr<CArtifactsOfHeroBase> & newArtSet)
{
	artSets.emplace_back(newArtSet);
}

const CGHeroInstance * CWindowWithArtifacts::getHeroPickedArtifact() const
{
	const CGHeroInstance * hero = nullptr;

	for(const auto & artSet : artSets)
		if(const auto pickedArt = artSet->getHero()->getArt(ArtifactPosition::TRANSITION_POS))
		{
			hero = artSet->getHero();
			break;
		}
	return hero;
}

const CArtifactInstance * CWindowWithArtifacts::getPickedArtifact() const
{
	const CArtifactInstance * art = nullptr;

	for(const auto & artSet : artSets)
		if(const auto pickedArt = artSet->getHero()->getArt(ArtifactPosition::TRANSITION_POS))
		{
			art = pickedArt;
			break;
		}
	return art;
}

void CWindowWithArtifacts::clickPressedOnArtPlace(const CGHeroInstance * hero, const ArtifactPosition & slot,
	bool allowExchange, bool altarTrading, bool closeWindow)
{
	if(!LOCPLINT->makingTurn)
		return;
	if(hero == nullptr)
		return;

	if(const auto heroArtOwner = getHeroPickedArtifact())
	{
		if(allowExchange || hero->id == heroArtOwner->id)
			putPickedArtifact(*hero, slot);
	}
	else if(auto art = hero->getArt(slot))
	{
		if(hero->getOwner() == LOCPLINT->playerID)
		{
			if(checkSpecialArts(*art, *hero, altarTrading))
				onClickPressedCommonArtifact(*hero, slot, closeWindow);
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
	const ArtifactLocation & dstLoc)
{
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(artsInst.getHero()->id, slot), dstLoc);
	close();
}

void CWindowWithArtifacts::showArtifactAssembling(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace,
	const Point & cursorPosition) const
{
	if(artsInst.getArt(artPlace.slot))
	{
		if(LOCPLINT->artifactController->askToDisassemble(artsInst.getHero(), artPlace.slot))
			return;
		if(LOCPLINT->artifactController->askToAssemble(artsInst.getHero(), artPlace.slot))
			return;
		if(artPlace.text.size())
			artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
	}
}

void CWindowWithArtifacts::showArifactInfo(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition) const
{
	if(artsInst.getArt(artPlace.slot) && artPlace.text.size())
		artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
}

void CWindowWithArtifacts::showQuickBackpackWindow(const CGHeroInstance * hero, const ArtifactPosition & slot,
	const Point & cursorPosition) const
{
	if(!settings["general"]["enableUiEnhancements"].Bool())
		return;

	GH.windows().createAndPushWindow<CHeroQuickBackpackWindow>(hero, slot);
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

void CWindowWithArtifacts::enableKeyboardShortcuts() const
{
	for(auto & artSet : artSets)
		artSet->enableKeyboardShortcuts();
}

void CWindowWithArtifacts::update()
{
	for(const auto & artSet : artSets)
	{
		artSet->updateWornSlots();
		artSet->updateBackpackSlots();

		if(const auto pickedArtInst = getPickedArtifact())
		{
			markPossibleSlots();
			setCursorAnimation(*pickedArtInst);
		}
		else
		{
			artSet->unmarkSlots();
			CCS->curh->dragAndDropCursor(nullptr);
		}

		// Make sure the status bar is updated so it does not display old text
		if(auto artPlace = artSet->getArtPlace(GH.getCursorPosition()))
			artPlace->hover(true);
	}
	redraw();
}

void CWindowWithArtifacts::markPossibleSlots() const
{
	if(const auto pickedArtInst = getPickedArtifact())
	{
		for(const auto & artSet : artSets)
		{
			const auto hero = artSet->getHero();
			if(hero == nullptr || !artSet->isActive())
				continue;

			if(getHeroPickedArtifact() == hero || !std::dynamic_pointer_cast<CArtifactsOfHeroKingdom>(artSet))
				artSet->markPossibleSlots(pickedArtInst, hero->tempOwner == LOCPLINT->playerID);
		}
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

void CWindowWithArtifacts::setCursorAnimation(const CArtifactInstance & artInst) const
{
	if(artInst.isScroll() && settings["general"]["enableUiEnhancements"].Bool())
	{
		assert(artInst.getScrollSpellID().num >= 0);
		auto image = GH.renderHandler().loadImage(AnimationPath::builtin("spellscr"), artInst.getScrollSpellID().num, 0, EImageBlitMode::COLORKEY);
		image->scaleFast(Point(44,34));

		CCS->curh->dragAndDropCursor(image);
	}
	else
	{
		CCS->curh->dragAndDropCursor(AnimationPath::builtin("artifact"), artInst.artType->getIconIndex());
	}
}

void CWindowWithArtifacts::putPickedArtifact(const CGHeroInstance & curHero, const ArtifactPosition & targetSlot) const
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
		for(const auto & anotherSet : artSets)
		{
			if(std::dynamic_pointer_cast<CArtifactsOfHeroMain>(anotherSet) && curHero.id != anotherSet->getHero()->id)
			{
				dstLoc.slot = ArtifactPosition::FIRST_AVAILABLE;
				dstLoc.artHolder = anotherSet->getHero()->id;
				break;
			}
			if(const auto heroSetAltar = std::dynamic_pointer_cast<CArtifactsOfHeroAltar>(anotherSet))
			{
				dstLoc.slot = ArtifactPosition::FIRST_AVAILABLE;
				dstLoc.artHolder = heroSetAltar->altarId;
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
	else if(closeWindow)
	{
		close();
	}
	if(dstLoc.slot != ArtifactPosition::PRE_FIRST)
		LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
}
