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
#include "../lib/CGeneralTextHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"

#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "widgets/CComponent.h"
#include "windows/CWindowWithArtifacts.h"

bool ArtifactsUIController::askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot, std::set<ArtifactID> * ignoredArtifacts)
{
	assert(hero);
	const auto art = hero->getArt(slot);
	assert(art);

	if(hero->tempOwner != LOCPLINT->playerID)
		return false;

	auto assemblyPossibilities = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId(), true);
	if(!assemblyPossibilities.empty())
	{
		auto askThread = new boost::thread([this, hero, art, slot, assemblyPossibilities, ignoredArtifacts]() -> void
			{
				boost::mutex::scoped_lock askLock(askAssembleArtifactsMutex);
				for(const auto combinedArt : assemblyPossibilities)
				{
					boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
					if(ignoredArtifacts && vstd::contains(*ignoredArtifacts, combinedArt->getId()))
						continue;

					bool assembleConfirmed = false;
					MetaString message = MetaString::createFromTextID(art->artType->getDescriptionTextID());
					message.appendEOL();
					message.appendEOL();
					message.appendRawString(CGI->generaltexth->allTexts[732]); // You possess all of the components needed to assemble the
					message.replaceName(ArtifactID(combinedArt->getId()));
					LOCPLINT->showYesNoDialog(message.toString(), [&assembleConfirmed, hero, slot, combinedArt]()
						{
							assembleConfirmed = true;
							LOCPLINT->cb.get()->assembleArtifacts(hero, slot, true, combinedArt->getId());
						}, nullptr, {std::make_shared<CComponent>(ComponentType::ARTIFACT, combinedArt->getId())});

					LOCPLINT->waitWhileDialog();
					if(ignoredArtifacts)
						ignoredArtifacts->emplace(combinedArt->getId());
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

	if(art->isCombined())
	{
		if(ArtifactUtils::isSlotBackpack(slot) && !ArtifactUtils::isBackpackFreeSlots(hero, art->artType->getConstituents().size() - 1))
			return false;

		MetaString message = MetaString::createFromTextID(art->artType->getDescriptionTextID());
		message.appendEOL();
		message.appendEOL();
		message.appendRawString(CGI->generaltexth->allTexts[733]); // Do you wish to disassemble this artifact?
		LOCPLINT->showYesNoDialog(message.toString(), [hero, slot]()
			{
				LOCPLINT->cb->assembleArtifacts(hero, slot, false, ArtifactID());
			}, nullptr);
		return true;
	}
	return false;
}

void ArtifactsUIController::artifactRemoved()
{
	for(auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
	LOCPLINT->waitWhileDialog();
}

void ArtifactsUIController::artifactMoved()
{
	// If a bulk transfer has arrived, then redrawing only the last art movement.
	if(numOfMovedArts != 0)
		numOfMovedArts--;

	for(auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		if(numOfMovedArts == 0)
		{
			artWin->update();
			artWin->redraw();
		}
	LOCPLINT->waitWhileDialog();
}

void ArtifactsUIController::bulkArtMovementStart(size_t numOfArts)
{
	numOfMovedArts = numOfArts;
	if(numOfArtsAskAssembleSession == 0)
	{
		// Do not start the next session until the previous one is finished
		numOfArtsAskAssembleSession = numOfArts; // TODO this is wrong
		ignoredArtifacts.clear();
	}
}

void ArtifactsUIController::artifactAssembled()
{
	for(auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
}

void ArtifactsUIController::artifactDisassembled()
{
	for(auto & artWin : GH.windows().findWindows<CWindowWithArtifacts>())
		artWin->update();
}
