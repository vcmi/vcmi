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

#include "../windows/CHeroWindow.h"
#include "../windows/CSpellWindow.h"
#include "../windows/GUIClasses.h"
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
			artSet->clickPressedCallback = std::bind(&CWindowWithArtifacts::clickPressedArtPlaceHero, this, _1, _2, _3);
			artSet->showPopupCallback = std::bind(&CWindowWithArtifacts::showPopupArtPlaceHero, this, _1, _2, _3);
			artSet->gestureCallback = std::bind(&CWindowWithArtifacts::gestureArtPlaceHero, this, _1, _2, _3);
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

void CWindowWithArtifacts::clickPressedArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto currentArtSet = findAOHbyRef(artsInst);
	assert(currentArtSet.has_value());

	if(artPlace.isLocked())
		return;

	if(!LOCPLINT->makingTurn)
		return;

	std::visit(
		[this, &artPlace](auto artSetPtr)
		{
			// Hero(Main, Exchange) window, Kingdom window, Altar window, Backpack window left click handler
			if constexpr(
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroKingdom>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroAltar>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroBackpack>>)
			{
				const auto pickedArtInst = getPickedArtifact();
				const auto heroPickedArt = getHeroPickedArtifact();
				const auto hero = artSetPtr->getHero();
				auto isTransferAllowed = false;
				std::string msg;

				if(pickedArtInst)
				{
					auto srcLoc = ArtifactLocation(heroPickedArt->id, ArtifactPosition::TRANSITION_POS);
					auto dstLoc = ArtifactLocation(hero->id, artPlace.slot);

					if(ArtifactUtils::isSlotBackpack(artPlace.slot))
					{
						if(pickedArtInst->artType->isBig())
						{
							// War machines cannot go to backpack
							msg = boost::str(boost::format(CGI->generaltexth->allTexts[153]) % pickedArtInst->artType->getNameTranslated());
						}
						else
						{
							if(ArtifactUtils::isBackpackFreeSlots(heroPickedArt))
								isTransferAllowed = true;
							else
								msg = CGI->generaltexth->translate("core.genrltxt.152");
						}
					}
					// Check if artifact transfer is possible
					else if(pickedArtInst->canBePutAt(hero, artPlace.slot, true) && (!artPlace.getArt() || hero->tempOwner == LOCPLINT->playerID))
					{
						isTransferAllowed = true;
					}
					if constexpr(std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroKingdom>>)
					{
						if(hero != heroPickedArt)
							isTransferAllowed = false;
					}
					if(isTransferAllowed)
						LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
				}
				else if(auto art = artPlace.getArt())
				{
					if(artSetPtr->getHero()->getOwner() == LOCPLINT->playerID)
					{
						if(checkSpecialArts(*art, hero, std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroAltar>> ? true : false))
						{
							assert(artSetPtr->getHero()->getSlotByInstance(art) != ArtifactPosition::PRE_FIRST);

							auto srcLoc = ArtifactLocation(hero->id, artPlace.slot);
							auto dstLoc = ArtifactLocation(hero->id, ArtifactPosition::TRANSITION_POS);

							if(GH.isKeyboardCtrlDown())
							{
								for(auto & anotherSet : artSets)
									if(std::holds_alternative<std::shared_ptr<CArtifactsOfHeroMain>>(anotherSet))
									{
										auto anotherHeroEquipment = std::get<std::shared_ptr<CArtifactsOfHeroMain>>(anotherSet);
										if(hero->id != anotherHeroEquipment->getHero()->id)
										{
											dstLoc.slot = ArtifactPosition::FIRST_AVAILABLE;
											dstLoc.artHolder = anotherHeroEquipment->getHero()->id;
											break;
										}
									}
							}
							else if(GH.isKeyboardAltDown())
							{
								if(ArtifactUtils::isSlotEquipment(artPlace.slot))
									dstLoc.slot = ArtifactUtils::getArtBackpackPosition(artSetPtr->getHero(), art->getTypeId());
								else if(ArtifactUtils::isSlotBackpack(artPlace.slot))
									dstLoc.slot = ArtifactUtils::getArtEquippedPosition(artSetPtr->getHero(), art->getTypeId());
							}
							if(dstLoc.slot != ArtifactPosition::PRE_FIRST)
								LOCPLINT->cb->swapArtifacts(srcLoc, dstLoc);
						}
					}
					else
					{
						for(const auto & artSlot : ArtifactUtils::unmovableSlots())
							if(artPlace.slot == artSlot)
							{
								msg = CGI->generaltexth->allTexts[21];
								break;
							}
					}
				}

				if constexpr(std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroBackpack>>)
				{
					if(!isTransferAllowed && artPlace.getArt() && !GH.isKeyboardCtrlDown() && !GH.isKeyboardAltDown() && closeCallback)
						closeCallback();
				}
				else
				{
					if(!msg.empty())
						LOCPLINT->showInfoDialog(msg);
				}
			}
			// Market window left click handler
			else if constexpr(std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroMarket>>)
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
			else if constexpr(std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				const auto hero = artSetPtr->getHero();
				LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero->id, artPlace.slot), ArtifactLocation(hero->id, artSetPtr->getFilterSlot()));
				if(closeCallback)
					closeCallback();
			}
		}, currentArtSet.value());
}

void CWindowWithArtifacts::showPopupArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto currentArtSet = findAOHbyRef(artsInst);
	assert(currentArtSet.has_value());

	if(artPlace.isLocked())
		return;

	std::visit(
		[&artPlace, &cursorPosition](auto artSetPtr)
		{
			// Hero (Main, Exchange) window, Kingdom window, Backpack window right click handler
			if constexpr(
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroAltar>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroKingdom>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroBackpack>>)
			{
				if(artPlace.getArt())
				{
					if(ArtifactUtilsClient::askToDisassemble(artSetPtr->getHero(), artPlace.slot))
					{
						return;
					}
					if(ArtifactUtilsClient::askToAssemble(artSetPtr->getHero(), artPlace.slot))
					{
						return;
					}
					if(artPlace.text.size())
						artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
				}
			}
			// Altar window, Market window right click handler
			else if constexpr(
				std::is_same_v<decltype(artSetPtr), std::weak_ptr<CArtifactsOfHeroMarket>> ||
				std::is_same_v<decltype(artSetPtr), std::weak_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				if(artPlace.getArt() && artPlace.text.size())
					artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
			}
		}, currentArtSet.value());
}

void CWindowWithArtifacts::gestureArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto currentArtSet = findAOHbyRef(artsInst);
	assert(currentArtSet.has_value());
	if(artPlace.isLocked())
		return;

	std::visit(
		[&artPlace, cursorPosition](auto artSetPtr)
		{
			if constexpr(
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSetPtr), std::shared_ptr<CArtifactsOfHeroKingdom>>)
			{
				if(!settings["general"]["enableUiEnhancements"].Bool())
					return;

				GH.windows().createAndPushWindow<CHeroQuickBackpackWindow>(artSetPtr->getHero(), artPlace.slot);
				auto backpackWindow = GH.windows().topWindow<CHeroQuickBackpackWindow>();
				backpackWindow->moveTo(cursorPosition - Point(1, 1));
				backpackWindow->fitToScreen(15);
			}
		}, currentArtSet.value());
}

void CWindowWithArtifacts::activate()
{
	if(const auto art = getPickedArtifact())
		setCursorAnimation(*art);
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

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw)
{
	auto artifactMovedBody = [this](auto artSetPtr)
	{
		if(const auto pickedArtInst = getPickedArtifact())
		{
			setCursorAnimation(*pickedArtInst);
		}
		else
		{
			artSetPtr->unmarkSlots();
			CCS->curh->dragAndDropCursor(nullptr);
		}
	};

	for(auto & artSet : artSets)
		std::visit(artifactMovedBody, artSet);

	if(withRedraw)
		update();
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
		std::visit([this](auto artSetPtr)
			{
				artSetPtr->updateWornSlots();
				artSetPtr->updateBackpackSlots();

				// Update arts bonuses on window.
				// TODO rework this part when CHeroWindow and CExchangeWindow are reworked
				if(auto * chw = dynamic_cast<CHeroWindow*>(this))
				{
					chw->update(artSetPtr->getHero(), true);
				}
				else if(auto * cew = dynamic_cast<CExchangeWindow*>(this))
				{
					cew->updateWidgets();
				}

				// Make sure the status bar is updated so it does not display old text
				if(auto artPlace = artSetPtr->getArtPlace(GH.getCursorPosition()))
					artPlace->hover(true);

				artSetPtr->redraw();
			}, artSet);
}

std::optional<CWindowWithArtifacts::ArtifactsOfHeroVar> CWindowWithArtifacts::findAOHbyRef(const CArtifactsOfHeroBase & artsInst)
{
	std::optional<ArtifactsOfHeroVar> res;
	for(auto & artSet : artSets)
	{
		std::visit([&res, &artsInst](auto & artSetPtr)
			{
				if(&artsInst == artSetPtr.get())
					res = artSetPtr;
			}, artSet);
		if(res.has_value())
			return res;
	}
	return res;
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
				if(heroArtOwner == hero || !std::is_same_v<decltype(artSetPtr), std::weak_ptr<CArtifactsOfHeroKingdom>>)
					artSetPtr->markPossibleSlots(pickedArtInst, hero->tempOwner == LOCPLINT->playerID);
			}
		};

		for(auto & artSetWeak : artSets)
			std::visit(artifactAssembledBody, artSetWeak);
	}
}

bool CWindowWithArtifacts::checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance * hero, bool isTrade) const
{
	const auto artId = artInst.getTypeId();
	
	if(artId == ArtifactID::SPELLBOOK)
	{
		GH.windows().createAndPushWindow<CSpellWindow>(hero, LOCPLINT, LOCPLINT->battleInt.get());
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
	markPossibleSlots();
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
