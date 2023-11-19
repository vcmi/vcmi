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
#include "../gui/Shortcut.h"

#include "CComponent.h"

#include "../windows/GUIClasses.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/IRenderHandler.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/ArtifactUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CConfigHandler.h"

void CArtPlace::setInternals(const CArtifactInstance * artInst)
{
	ourArt = artInst;
	if(!artInst)
	{
		image->disable();
		text.clear();
		hoverText = CGI->generaltexth->allTexts[507];
		return;
	}

	imageIndex = artInst->artType->getIconIndex();
	if(artInst->getTypeId() == ArtifactID::SPELL_SCROLL)
	{
		auto spellID = artInst->getScrollSpellID();
		assert(spellID.num >= 0);

		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			imageIndex = spellID.num;
			if(component.type != ComponentType::SPELL_SCROLL)
			{
				image->setScale(Point(pos.w, 34));
				image->setAnimationPath(AnimationPath::builtin("spellscr"), imageIndex);
				image->moveTo(Point(pos.x, pos.y + 4));
			}
		}
		// Add spell component info (used to provide a pic in r-click popup)
		component.type = ComponentType::SPELL_SCROLL;
		component.subType = spellID;
	}
	else
	{
		if(settings["general"]["enableUiEnhancements"].Bool() && component.type != ComponentType::ARTIFACT)
		{
			image->setScale(Point());
			image->setAnimationPath(AnimationPath::builtin("artifact"), imageIndex);
			image->moveTo(Point(pos.x, pos.y));
		}
		component.type = ComponentType::ARTIFACT;
		component.subType = artInst->getTypeId();
	}
	image->enable();
	text = artInst->getDescription();
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * art)
	: SelectableSlot(Rect(position, Point(44, 44)), Point(1, 1))
	, ourArt(art)
	, locked(false)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	imageIndex = 0;
	if(locked)
		imageIndex = ArtifactID::ART_LOCK;
	else if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), imageIndex);
	image->disable();
}

const CArtifactInstance * CArtPlace::getArt()
{
	return ourArt;
}

CCommanderArtPlace::CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * art)
	: CArtPlace(position, art),
	commanderOwner(commanderOwner),
	commanderSlotID(artSlot.num)
{
	setArtifact(art);
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
		ArtifactLocation src(commanderOwner->id, artifactPos);
		src.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		ArtifactLocation dst(commanderOwner->id, freeSlot);

		if(ourArt->canBePutAt(commanderOwner, freeSlot, true))
		{
			LOCPLINT->cb->swapArtifacts(src, dst);
			setArtifact(nullptr);
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickPressed(const Point & cursorPosition)
{
	if(ourArt && text.size())
		LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
}

void CCommanderArtPlace::showPopupWindow(const Point& cursorPosition)
{
	if(ourArt && text.size())
		CArtPlace::showPopupWindow(cursorPosition);
}

CHeroArtPlace::CHeroArtPlace(Point position, const CArtifactInstance * art)
	: CArtPlace(position, art)
{
}

void CArtPlace::lockSlot(bool on)
{
	if(locked == on)
		return;

	locked = on;

	if(on)
		image->setFrame(ArtifactID::ART_LOCK);
	else if(ourArt)
		image->setFrame(imageIndex);
	else
		image->setFrame(0);
}

bool CArtPlace::isLocked() const
{
	return locked;
}

void CArtPlace::clickPressed(const Point & cursorPosition)
{
	if(clickPressedCallback)
		clickPressedCallback(*this, cursorPosition);
}

void CArtPlace::showPopupWindow(const Point & cursorPosition)
{
	if(showPopupCallback)
		showPopupCallback(*this, cursorPosition);
}

void CArtPlace::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	if(gestureCallback)
		gestureCallback(*this, initialPosition);
}

void CArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
	if(art)
	{
		image->setFrame(locked ? static_cast<int>(ArtifactID::ART_LOCK) : imageIndex);

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

void CArtPlace::setClickPressedCallback(ClickFunctor callback)
{
	clickPressedCallback = callback;
}

void CArtPlace::setShowPopupCallback(ClickFunctor callback)
{
	showPopupCallback = callback;
}

void CArtPlace::setGestureCallback(ClickFunctor callback)
{
	gestureCallback = callback;
}

void CHeroArtPlace::addCombinedArtInfo(std::map<const CArtifact*, int> & arts)
{
	for(const auto & combinedArt : arts)
	{
		std::string artList;
		text += "\n\n";
		text += "{" + combinedArt.first->getNameTranslated() + "}";
		if(arts.size() == 1)
		{
			for(const auto part : combinedArt.first->getConstituents())
				artList += "\n" + part->getNameTranslated();
		}
		text += " (" + boost::str(boost::format("%d") % combinedArt.second) + " / " +
			boost::str(boost::format("%d") % combinedArt.first->getConstituents().size()) + ")" + artList;
	}
}

bool ArtifactUtilsClient::askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(hero->tempOwner != LOCPLINT->playerID)
		return false;

	auto assemblyPossibilities = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId());
	if(!assemblyPossibilities.empty())
	{
		auto askThread = new boost::thread([hero, art, slot, assemblyPossibilities]() -> void
			{
				boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
				for(const auto combinedArt : assemblyPossibilities)
				{
					bool assembleConfirmed = false;
					CFunctionList<void()> onYesHandlers([&assembleConfirmed]() -> void {assembleConfirmed = true; });
					onYesHandlers += std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, true, combinedArt->getId());

					LOCPLINT->showArtifactAssemblyDialog(art->artType, combinedArt, onYesHandlers);
					LOCPLINT->waitWhileDialog();
					if(assembleConfirmed)
						break;
				}
			});
		askThread->detach();
		return true;
	}
	return false;
}

bool ArtifactUtilsClient::askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(hero->tempOwner != LOCPLINT->playerID)
		return false;

	if(art->isCombined())
	{
		if(ArtifactUtils::isSlotBackpack(slot) && !ArtifactUtils::isBackpackFreeSlots(hero, art->artType->getConstituents().size() - 1))
			return false;

		LOCPLINT->showArtifactAssemblyDialog(
			art->artType,
			nullptr,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, false, ArtifactID()));
		return true;
	}
	return false;
}
