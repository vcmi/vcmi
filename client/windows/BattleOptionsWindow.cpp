/*
 * BattleOptionsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleOptionsWindow.h"
#include "CConfigHandler.h"
#include "gui/CGuiHandler.h"

#include "../../lib/filesystem/ResourceID.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "CGameInfo.h"

BattleOptionsWindow::BattleOptionsWindow(BattleInterface * owner):
		InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/battleOptionsWindow.json"));
	build(config);

	auto viewGrid = std::make_shared<CToggleButton>(Point(25, 56), "sysopchk.def", CGI->generaltexth->zelp[427], [=](bool on)
	{
		Settings cellBorders = settings.write["battle"]["cellBorders"];
		cellBorders->Bool() = on;
		if(owner)
			owner->redrawBattlefield();
	});
	viewGrid->setSelected(settings["battle"]["cellBorders"].Bool());
	toggles.push_back(viewGrid);

	auto movementShadow = std::make_shared<CToggleButton>(Point(25, 89), "sysopchk.def", CGI->generaltexth->zelp[428], [=](bool on)
	{
		Settings stackRange = settings.write["battle"]["stackRange"];
		stackRange->Bool() = on;
		if(owner)
			owner->redrawBattlefield();
	});
	movementShadow->setSelected(settings["battle"]["stackRange"].Bool());
	toggles.push_back(movementShadow);

	auto mouseShadow = std::make_shared<CToggleButton>(Point(25, 122), "sysopchk.def", CGI->generaltexth->zelp[429], [&](bool on)
	{
		Settings shadow = settings.write["battle"]["mouseShadow"];
		shadow->Bool() = on;
	});
	mouseShadow->setSelected(settings["battle"]["mouseShadow"].Bool());
	toggles.push_back(mouseShadow);

	animSpeeds = std::make_shared<CToggleGroup>([&](int value)
												{
													Settings speed = settings.write["battle"]["speedFactor"];
													speed->Float() = float(value);
												});

	std::shared_ptr<CToggleButton> toggle;
	toggle = std::make_shared<CToggleButton>(Point( 28, 225), "sysopb9.def", CGI->generaltexth->zelp[422]);
	animSpeeds->addToggle(1, toggle);

	toggle = std::make_shared<CToggleButton>(Point( 92, 225), "sysob10.def", CGI->generaltexth->zelp[423]);
	animSpeeds->addToggle(2, toggle);

	toggle = std::make_shared<CToggleButton>(Point(156, 225), "sysob11.def", CGI->generaltexth->zelp[424]);
	animSpeeds->addToggle(3, toggle);

	animSpeeds->setSelected(getAnimSpeed());

	//creating labels
	labels.push_back(std::make_shared<CLabel>(242,  32, FONT_BIG,    ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(std::make_shared<CLabel>(122, 214, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(std::make_shared<CLabel>(122, 293, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(std::make_shared<CLabel>(122, 359, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(std::make_shared<CLabel>(353,  66, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(std::make_shared<CLabel>(353, 265, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(std::make_shared<CLabel>(283,  86, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(std::make_shared<CLabel>(283, 116, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(std::make_shared<CLabel>(283, 146, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(std::make_shared<CLabel>(283, 176, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(std::make_shared<CLabel>(283, 206, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(std::make_shared<CLabel>(283, 285, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(std::make_shared<CLabel>(283, 315, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[403]));//spells only

	//general options
	labels.push_back(std::make_shared<CLabel>(61,  57, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[404]));
	labels.push_back(std::make_shared<CLabel>(61,  90, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[405]));
	labels.push_back(std::make_shared<CLabel>(61, 123, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[406]));
	labels.push_back(std::make_shared<CLabel>(61, 156, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[407]));
}

int BattleOptionsWindow::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(vstd::round(settings["session"]["spectate-battle-speed"].Float()));

	return static_cast<int>(vstd::round(settings["battle"]["speedFactor"].Float()));
}