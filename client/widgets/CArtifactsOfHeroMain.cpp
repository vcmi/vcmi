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

CArtifactsOfHeroMain::CArtifactsOfHeroMain(const Point & position, bool costumesEnabled)
{
	init(
		std::bind(&CArtifactsOfHeroBase::clickPrassedArtPlace, this, _1, _2),
		std::bind(&CArtifactsOfHeroBase::showPopupArtPlace, this, _1, _2),
		position,
		std::bind(&CArtifactsOfHeroBase::scrollBackpack, this, _1));
	addGestureCallback(std::bind(&CArtifactsOfHeroBase::gestureArtPlace, this, _1, _2));

	if(costumesEnabled)
	{
		size_t costumeIndex = 0;
		for(const auto & hotkey : hotkeys)
		{
			auto keyProc = costumesSwitchers.emplace_back(std::make_shared<CKeyShortcutWrapper>(hotkey,
				[this, hotkey, costumeIndex]()
				{
					CArtifactsOfHeroMain::onCostumeSelect(costumeIndex);
				}));
			keyProc->addUsedEvents(AEventsReceiver::KEYBOARD);
			costumeIndex++;
		}
	}
}

CArtifactsOfHeroMain::~CArtifactsOfHeroMain()
{
	CArtifactsOfHeroBase::putBackPickedArtifact();
}

void CArtifactsOfHeroMain::onCostumeSelect(const size_t costumeIndex)
{
	LOCPLINT->cb->manageHeroCostume(getHero()->id, costumeIndex, GH.isKeyboardCtrlDown());
}

CArtifactsOfHeroMain::CKeyShortcutWrapper::CKeyShortcutWrapper(const EShortcut & key, const KeyPressedFunctor & onCostumeSelect)
	: CKeyShortcut(key)
	, onCostumeSelect(onCostumeSelect)
{
}

void CArtifactsOfHeroMain::CKeyShortcutWrapper::clickPressed(const Point & cursorPosition)
{
	if(onCostumeSelect)
		onCostumeSelect();
}
