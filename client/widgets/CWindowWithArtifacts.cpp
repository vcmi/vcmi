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

#include "CComponent.h"

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

void CWindowWithArtifacts::addSet(CArtifactsOfHeroPtr artSet)
{
	artSets.emplace_back(artSet);
}

void CWindowWithArtifacts::addSetAndCallbacks(CArtifactsOfHeroPtr artSet)
{
	CArtifactsOfHeroBase::PutBackPickedArtCallback artPutBackFunctor = []() -> void
	{
		CCS->curh->dragAndDropCursor(nullptr);
	};

	addSet(artSet);
	std::visit([this, artPutBackFunctor](auto artSetWeak)
		{
			auto artSet = artSetWeak.lock();
			artSet->clickPressedCallback = std::bind(&CWindowWithArtifacts::clickPressedArtPlaceHero, this, _1, _2, _3);
			artSet->showPopupCallback = std::bind(&CWindowWithArtifacts::showPopupArtPlaceHero, this, _1, _2, _3);
			artSet->gestureCallback = std::bind(&CWindowWithArtifacts::gestureArtPlaceHero, this, _1, _2, _3);
			artSet->setPutBackPickedArtifactCallback(artPutBackFunctor);
		}, artSet);
}

void CWindowWithArtifacts::addCloseCallback(CloseCallback callback)
{
	closeCallback = callback;
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

void CWindowWithArtifacts::clickPressedArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto artSet = findAOHbyRef(artsInst);
	assert(artSet.has_value());

	if(artPlace.isLocked())
		return;

	std::visit(
		[this, &artPlace](auto artSetWeak) -> void
		{
			const auto artSetPtr = artSetWeak.lock();

			// Hero(Main, Exchange) window, Kingdom window, Altar window, Backpack window left click handler
			if constexpr(
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMain>> || 
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroAltar>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroBackpack>>)
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
					if constexpr(std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>)
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
						if(checkSpecialArts(*art, hero, std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroAltar>> ? true : false))
						{
							assert(artSetPtr->getHero()->getSlotByInstance(art));
							LOCPLINT->cb->swapArtifacts(ArtifactLocation(artSetPtr->getHero()->id, artSetPtr->getHero()->getSlotByInstance(art)),
								ArtifactLocation(artSetPtr->getHero()->id, ArtifactPosition::TRANSITION_POS));
						}
					}
					else
					{
						for(const auto artSlot : ArtifactUtils::unmovableSlots())
							if(artPlace.slot == artSlot)
							{
								msg = CGI->generaltexth->allTexts[21];
								break;
							}
					}
				}

				if constexpr(std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroBackpack>>)
				{
					if(!isTransferAllowed && artPlace.getArt())
					{
						if(closeCallback)
							closeCallback();
					}
				}
				else
				{
					if(!msg.empty())
						LOCPLINT->showInfoDialog(msg);
				}
			}
			// Market window left click handler
			else if constexpr(std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMarket>>)
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
			else if constexpr(std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				const auto hero = artSetPtr->getHero();
				LOCPLINT->cb->swapArtifacts(ArtifactLocation(hero->id, artPlace.slot), ArtifactLocation(hero->id, artSetPtr->getFilterSlot()));
				if(closeCallback)
					closeCallback();
			}
		}, artSet.value());
}

void CWindowWithArtifacts::showPopupArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto artSetWeak = findAOHbyRef(artsInst);
	assert(artSetWeak.has_value());

	if(artPlace.isLocked())
		return;

	std::visit(
		[&artPlace, &cursorPosition](auto artSetWeak) -> void
		{
			const auto artSetPtr = artSetWeak.lock();

			// Hero (Main, Exchange) window, Kingdom window, Backpack window right click handler
			if constexpr(
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroAltar>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroBackpack>>)
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
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMarket>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroQuickBackpack>>)
			{
				if(artPlace.getArt() && artPlace.text.size())
					artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
			}
		}, artSetWeak.value());
}

void CWindowWithArtifacts::gestureArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition)
{
	const auto artSetWeak = findAOHbyRef(artsInst);
	assert(artSetWeak.has_value());
	if(artPlace.isLocked())
		return;

	std::visit(
		[&artPlace, cursorPosition](auto artSetWeak) -> void
		{
			const auto artSetPtr = artSetWeak.lock();
			if constexpr(
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroMain>> ||
				std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>)
			{
				if(!settings["general"]["enableUiEnhancements"].Bool())
					return;

				GH.windows().createAndPushWindow<CHeroQuickBackpackWindow>(artSetPtr->getHero(), artPlace.slot);
				auto backpackWindow = GH.windows().topWindow<CHeroQuickBackpackWindow>();
				backpackWindow->moveTo(cursorPosition - Point(1, 1));
				backpackWindow->fitToScreen(15);
			}
		}, artSetWeak.value());
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation & artLoc)
{
	updateSlots();
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
	assert(!pickedArtInst || destLoc.artHolder == std::get<const CGHeroInstance*>(curState.value())->id);

	auto artifactMovedBody = [this, withRedraw, &destLoc, &pickedArtInst](auto artSetWeak) -> void
	{
		auto artSetPtr = artSetWeak.lock();
		if(artSetPtr)
		{
			const auto hero = artSetPtr->getHero();
			if(pickedArtInst)
			{
				markPossibleSlots();
				
				if(pickedArtInst->getTypeId() == ArtifactID::SPELL_SCROLL && pickedArtInst->getScrollSpellID().num >= 0 && settings["general"]["enableUiEnhancements"].Bool())
				{
					auto anim = GH.renderHandler().loadAnimation(AnimationPath::builtin("spellscr"));
					anim->load(pickedArtInst->getScrollSpellID().num);
					std::shared_ptr<IImage> img = anim->getImage(pickedArtInst->getScrollSpellID().num);
					CCS->curh->dragAndDropCursor(img->scaleFast(Point(44, 34)));
				}
				else
					CCS->curh->dragAndDropCursor(AnimationPath::builtin("artifact"), pickedArtInst->artType->getIconIndex());
			}
			else
			{
				artSetPtr->unmarkSlots();
				CCS->curh->dragAndDropCursor(nullptr);
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
				artSetPtr->redraw();
			}

			// Make sure the status bar is updated so it does not display old text
			if(destLoc.artHolder == hero->id)
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
	updateSlots();
}

void CWindowWithArtifacts::artifactAssembled(const ArtifactLocation & artLoc)
{
	markPossibleSlots();
	updateSlots();
}

void CWindowWithArtifacts::updateSlots()
{
	auto updateSlotBody = [](auto artSetWeak) -> void
	{
		if(const auto artSetPtr = artSetWeak.lock())
		{
			artSetPtr->updateWornSlots();
			artSetPtr->updateBackpackSlots();
			artSetPtr->redraw();
		}
	};

	for(auto artSetWeak : artSets)
		std::visit(updateSlotBody, artSetWeak);
}

std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> CWindowWithArtifacts::getState()
{
	const CArtifactInstance * artInst = nullptr;
	std::map<const CGHeroInstance*, size_t> pickedCnt;

	auto getHeroArtBody = [&artInst, &pickedCnt](auto artSetWeak) -> void
	{
		auto artSetPtr = artSetWeak.lock();
		if(artSetPtr)
		{
			if(const auto art = artSetPtr->getPickedArtifact())
			{
				const auto hero = artSetPtr->getHero();
				if(pickedCnt.count(hero) == 0)
				{
					pickedCnt.insert({ hero, hero->artifactsTransitionPos.size() });
					artInst = art;
				}
			}
		}
	};
	for(auto artSetWeak : artSets)
		std::visit(getHeroArtBody, artSetWeak);

	// The state is possible when the hero has placed an artifact in the ArtifactPosition::TRANSITION_POS,
	// and the previous artifact has not yet removed from the ArtifactPosition::TRANSITION_POS.
	// This is a transitional state. Then return nullopt.
	if(std::accumulate(std::begin(pickedCnt), std::end(pickedCnt), 0, [](size_t accum, const auto & value)
		{
			return accum + value.second;
		}) > 1)
		return std::nullopt;
	else
		return std::make_tuple(pickedCnt.begin()->first, artInst);
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

void CWindowWithArtifacts::markPossibleSlots()
{
	if(const auto pickedArtInst = getPickedArtifact())
	{
		const auto heroArtOwner = getHeroPickedArtifact();
		auto artifactAssembledBody = [&pickedArtInst, &heroArtOwner](auto artSetWeak) -> void
		{
			if(auto artSetPtr = artSetWeak.lock())
			{
				if(artSetPtr->isActive())
				{
					const auto hero = artSetPtr->getHero();
					if(heroArtOwner == hero || !std::is_same_v<decltype(artSetWeak), std::weak_ptr<CArtifactsOfHeroKingdom>>)
						artSetPtr->markPossibleSlots(pickedArtInst, hero->tempOwner == LOCPLINT->playerID);
				}
			}
		};

		for(auto artSetWeak : artSets)
			std::visit(artifactAssembledBody, artSetWeak);
	}
}

bool CWindowWithArtifacts::checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance * hero, bool isTrade)
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
	if(isTrade)
	{
		if(!artInst.artType->isTradable())
		{
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21],
				std::vector<std::shared_ptr<CComponent>>(1, std::make_shared<CComponent>(ComponentType::ARTIFACT, artId)));
			return false;
		}
	}
	return true;
}
