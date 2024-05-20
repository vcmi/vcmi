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
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	addGestureCallback(std::bind(&CArtifactsOfHeroBase::gestureArtPlace, this, _1, _2));
}

CArtifactsOfHeroMain::~CArtifactsOfHeroMain()
{
	CArtifactsOfHeroBase::putBackPickedArtifact();
}

void CArtifactsOfHeroMain::keyPressed(EShortcut key)
{
	if(!shortcutPressed)
	{
		uint32_t costumeIdx;
		switch(key)
		{
		case EShortcut::HERO_COSTUME_0:
			costumeIdx = 0;
			break;
		case EShortcut::HERO_COSTUME_1:
			costumeIdx = 1;
			break;
		case EShortcut::HERO_COSTUME_2:
			costumeIdx = 2;
			break;
		case EShortcut::HERO_COSTUME_3:
			costumeIdx = 3;
			break;
		case EShortcut::HERO_COSTUME_4:
			costumeIdx = 4;
			break;
		case EShortcut::HERO_COSTUME_5:
			costumeIdx = 5;
			break;
		case EShortcut::HERO_COSTUME_6:
			costumeIdx = 6;
			break;
		case EShortcut::HERO_COSTUME_7:
			costumeIdx = 7;
			break;
		case EShortcut::HERO_COSTUME_8:
			costumeIdx = 8;
			break;
		case EShortcut::HERO_COSTUME_9:
			costumeIdx = 9;
			break;
		default:
			return;
		}
		shortcutPressed = true;
		LOCPLINT->cb->manageHeroCostume(getHero()->id, costumeIdx, GH.isKeyboardCtrlDown());
	}
}

void CArtifactsOfHeroMain::keyReleased(EShortcut key)
{
	if(vstd::contains(costumesSwitcherHotkeys, key))
		shortcutPressed = false;
}
