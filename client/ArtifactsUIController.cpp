/*
 * ArtifactsUIController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#include "StdInc.h"
#include "ArtifactsUIController.h"
#include "CGameInfo.h"
#include "CPlayerInterface.h"

#include "../CCallback.h"
#include "../lib/ArtifactUtils.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"

#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "widgets/CComponent.h"
#include "windows/CWindowWithArtifacts.h"

ArtifactsUIController::ArtifactsUIController()
{
	numOfMovedArts = 0;
	numOfArtsAskAssembleSession = 0;
}

bool ArtifactsUIController::askToAssemble(const ArtifactLocation & al, const bool onlyEquipped, const bool checkIgnored)
{
	if(auto hero = LOCPLINT->cb->getHero(al.artHolder))
	{
		if(hero->getArt(al.slot) == nullptr)
		{
			logGlobal->error("artifact location %d points to nothing", al.slot.num);
			return false;
		}
		return askToAssemble(hero, al.slot, onlyEquipped, checkIgnored);
	}
	return false;
}

bool ArtifactsUIController::askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot,
	const bool onlyEquipped, const bool checkIgnored)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(hero->tempOwner != LOCPLINT->playerID)
		return false;

	if(numOfArtsAskAssembleSession != 0)
		numOfArtsAskAssembleSession--;
	auto assemblyPossibilities = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId(), onlyEquipped);
	if(!assemblyPossibilities.empty())
	{
		auto askThread = new boost::thread([this, hero, art, slot, assemblyPossibilities, checkIgnored]() -> void
			{
				boost::mutex::scoped_lock askLock(askAssembleArtifactMutex);
				for(const auto combinedArt : assemblyPossibilities)
				{
					boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
					if(checkIgnored)
					{
						if(vstd::contains(ignoredArtifacts, combinedArt->getId()))
							continue;
						ignoredArtifacts.emplace(combinedArt->getId());
					}

					bool assembleConfirmed = false;
					MetaString message = MetaString::createFromTextID(art->getType()->getDescriptionTextID());
					message.appendEOL();
					message.appendEOL();
					if(combinedArt->isFused())
						message.appendRawString(CGI->generaltexth->translate("vcmi.heroWindow.fusingArtifact.fusing"));
					else
						message.appendRawString(CGI->generaltexth->allTexts[732]); // You possess all of the components needed to assemble the
					message.replaceName(ArtifactID(combinedArt->getId()));
					LOCPLINT->showYesNoDialog(message.toString(), [&assembleConfirmed, hero, slot, combinedArt]()
						{
							assembleConfirmed = true;
							LOCPLINT->cb.get()->assembleArtifacts(hero->id, slot, true, combinedArt->getId());
						}, nullptr, {std::make_shared<CComponent>(ComponentType::ARTIFACT, combinedArt->getId())});

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

bool ArtifactsUIController::askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(hero->tempOwner != LOCPLINT->playerID)
		return false;

	if(art->hasParts())
	{
		if(ArtifactUtils::isSlotBackpack(slot) && !ArtifactUtils::isBackpackFreeSlots(hero, art->getType()->getConstituents().size() - 1))
			return false;

		MetaString message = MetaString::createFromTextID(art->getType()->getDescriptionTextID());
		message.appendEOL();
		message.appendEOL();
		message.appendRawString(CGI->generaltexth->allTexts[733]); // Do you wish to disassemble this artifact?
		LOCPLINT->showYesNoDialog(message.toString(), [hero, slot]()
			{
				LOCPLINT->cb->assembleArtifacts(hero->id, slot, false, ArtifactID());
			}, nullptr);
		return true;
	}
	return false;
}

void ArtifactsUIController::artifactRemoved()
{
	for(const auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
	LOCPLINT->waitWhileDialog();
}

void ArtifactsUIController::artifactMoved()
{
	// If a bulk transfer has arrived, then redrawing only the last art movement.
	if(numOfMovedArts != 0)
		numOfMovedArts--;

	if(numOfMovedArts == 0)
		for(const auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		{
			artWin->update();
		}
	LOCPLINT->waitWhileDialog();
}

void ArtifactsUIController::bulkArtMovementStart(size_t totalNumOfArts, size_t possibleAssemblyNumOfArts)
{
	assert(totalNumOfArts >= possibleAssemblyNumOfArts);
	numOfMovedArts = totalNumOfArts;
	if(numOfArtsAskAssembleSession == 0)
	{
		// Do not start the next session until the previous one is finished
		numOfArtsAskAssembleSession = possibleAssemblyNumOfArts;
		ignoredArtifacts.clear();
	}
}

void ArtifactsUIController::artifactAssembled()
{
	for(const auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
}

void ArtifactsUIController::artifactDisassembled()
{
	for(const auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
}
