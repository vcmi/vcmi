/*
 * CHeroBackpackWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroBackpackWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"

#include "../widgets/Buttons.h"

CHeroBackpackWindow::CHeroBackpackWindow(const CGHeroInstance * hero)
	: CWindowObject(PLAYER_COLORED)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	
	arts = std::make_shared<CArtifactsOfHeroBackpack>(Point(-100, -170));
	arts->setHero(hero);
	addSet(arts);

	addCloseCallback(std::bind(&CHeroBackpackWindow::close, this));

	quitButton = std::make_shared<CButton>(Point(242, 200), "hsbtns.def", CButton::tooltip(""), [this]() { close(); }, EShortcut::GLOBAL_RETURN);
}
