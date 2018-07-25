/*
 * CCampaignScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../mainmenu/CMainMenu.h"
#include "CCampaignScreen.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CBitmapHandler.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../windows/CWindowObject.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/CArtHandler.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/spells/CSpellHandler.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CCreatureHandler.h"

#include "../../lib/mapping/CCampaignHandler.h"
#include "../../lib/mapping/CMapService.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

CCampaignScreen::CCampaignScreen(const JsonNode & config)
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	if(!images.empty())
	{
		images[0]->center(); // move background to center
		moveTo(images[0]->pos.topLeft()); // move everything else to center
		images[0]->moveTo(pos.topLeft()); // restore moved twice background
		pos = images[0]->pos; // fix height\width of this window
	}

	if(!config["exitbutton"].isNull())
	{
		buttonBack = createExitButton(config["exitbutton"]);
		buttonBack->hoverable = true;
	}

	for(const JsonNode & node : config["items"].Vector())
		campButtons.push_back(std::make_shared<CCampaignButton>(node));
}

std::shared_ptr<CButton> CCampaignScreen::createExitButton(const JsonNode & button)
{
	std::pair<std::string, std::string> help;
	if(!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[button["help"].Float()];

	return std::make_shared<CButton>(Point(button["x"].Float(), button["y"].Float()), button["name"].String(), help, [=](){ close();}, button["hotkey"].Float());
}

CCampaignScreen::CCampaignButton::CCampaignButton(const JsonNode & config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.x += config["x"].Float();
	pos.y += config["y"].Float();
	pos.w = 200;
	pos.h = 116;

	campFile = config["file"].String();
	video = config["video"].String();

	status = config["open"].Bool() ? CCampaignScreen::ENABLED : CCampaignScreen::DISABLED;

	CCampaignHeader header = CCampaignHandler::getHeader(campFile);
	hoverText = header.name;

	if(status != CCampaignScreen::DISABLED)
	{
		addUsedEvents(LCLICK | HOVER);
		graphicsImage = std::make_shared<CPicture>(config["image"].String());

		hoverLabel = std::make_shared<CLabel>(pos.w / 2, pos.h + 20, FONT_MEDIUM, CENTER, Colors::YELLOW, "");
		parent->addChild(hoverLabel.get());
	}

	if(status == CCampaignScreen::COMPLETED)
		graphicsCompleted = std::make_shared<CPicture>("CAMPCHK");
}

void CCampaignScreen::CCampaignButton::show(SDL_Surface * to)
{
	if(status == CCampaignScreen::DISABLED)
		return;

	CIntObject::show(to);

	// Play the campaign button video when the mouse cursor is placed over the button
	if(hovered)
	{
		if(CCS->videoh->fname != video)
			CCS->videoh->open(video);

		CCS->videoh->update(pos.x, pos.y, to, true, false); // plays sequentially frame by frame, starts at the beginning when the video is over
	}
	else if(CCS->videoh->fname == video) // When you got out of the bounds of the button then close the video
	{
		CCS->videoh->close();
		redraw();
	}
}

void CCampaignScreen::CCampaignButton::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		CCS->videoh->close();
		CMainMenu::openCampaignLobby(campFile);
	}
}

void CCampaignScreen::CCampaignButton::hover(bool on)
{
	if(hoverLabel)
	{
		if(on)
			hoverLabel->setText(hoverText); // Shows the name of the campaign when you get into the bounds of the button
		else
			hoverLabel->setText(" ");
	}
}
