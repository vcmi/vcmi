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

#include "../gui/CGuiHandler.h"

#include "../CPlayerInterface.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

#include "../../CCallback.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CArtifactsOfHeroMain::CArtifactsOfHeroMain(const Point & position)
{
	init(position, std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	enableGesture();
}

CArtifactsOfHeroMain::~CArtifactsOfHeroMain()
{
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
			LOCPLINT->cb->manageHeroCostume(getHero()->id, saveIdx, true);
			return;
		}

		if (loadIdx != -1)
		{
			shortcutPressed = true;
			LOCPLINT->cb->manageHeroCostume(getHero()->id, loadIdx, false);
			return;
		}
	}
}

void CArtifactsOfHeroMain::keyReleased(EShortcut key)
{
	if(vstd::contains(costumeSaveShortcuts, key) || vstd::contains(costumeLoadShortcuts, key))
		shortcutPressed = false;
}
