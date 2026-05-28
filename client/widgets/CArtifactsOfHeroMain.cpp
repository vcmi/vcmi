/*
 * CArtifactsOfHeroMain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtifactsOfHeroMain.h"

#include "../GameEngine.h"
#include "../GameInstance.h"

#include "../CPlayerInterface.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroMain::CArtifactsOfHeroMain(const Point & position)
{
	init(position, std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	setClickPressedArtPlacesCallback(std::bind(&CArtifactsOfHeroBase::clickPressedArtPlace, this, _1, _2));
	setShowPopupArtPlacesCallback(std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2));
	enableGesture();
}

CArtifactsOfHeroMain::~CArtifactsOfHeroMain()
{
	if(curHero)
		CArtifactsOfHeroBase::putBackPickedArtifact();
}

void CArtifactsOfHeroMain::keyPressed(EShortcut key)
{
	if(!shortcutPressed)
	{
		int saveIdx = vstd::find_pos(costumeSaveShortcuts, key);
		int loadIdx = vstd::find_pos(costumeLoadShortcuts, key);

		if (saveIdx != -1)
		{
			shortcutPressed = true;
			GAME->interface()->cb->manageHeroCostume(getHero()->id, saveIdx, true);
			return;
		}

		if (loadIdx != -1)
		{
			shortcutPressed = true;
			GAME->interface()->cb->manageHeroCostume(getHero()->id, loadIdx, false);
			return;
		}
	}
}

void CArtifactsOfHeroMain::keyReleased(EShortcut key)
{
	if(vstd::contains(costumeSaveShortcuts, key) || vstd::contains(costumeLoadShortcuts, key))
		shortcutPressed = false;
}
