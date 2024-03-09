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
#include "CCampaignScreen.h"

#include "CMainMenu.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../render/Canvas.h"
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

#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/mapping/CMapService.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

CCampaignScreen::CCampaignScreen(const JsonNode & config, std::string name)
	: CWindowObject(BORDERED), campaignSet(name)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	for(const JsonNode & node : config[name]["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	if(!images.empty())
	{
		images[0]->center(); // move background to center
		moveTo(images[0]->pos.topLeft()); // move everything else to center
		images[0]->moveTo(pos.topLeft()); // restore moved twice background
		pos = images[0]->pos; // fix height\width of this window
	}

	if(!config[name]["exitbutton"].isNull())
	{
		buttonBack = createExitButton(config[name]["exitbutton"]);
		buttonBack->setHoverable(true);
	}

	for(const JsonNode & node : config[name]["items"].Vector())
		campButtons.push_back(std::make_shared<CCampaignButton>(node, config, campaignSet));
}

void CCampaignScreen::activate()
{
	CCS->musich->playMusic(AudioPath::builtin("Music/MainMenu"), true, false);

	CWindowObject::activate();
}

std::shared_ptr<CButton> CCampaignScreen::createExitButton(const JsonNode & button)
{
	std::pair<std::string, std::string> help;
	if(!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[(size_t)button["help"].Float()];

	return std::make_shared<CButton>(Point((int)button["x"].Float(), (int)button["y"].Float()), AnimationPath::fromJson(button["name"]), help, [=](){ close();}, EShortcut::GLOBAL_CANCEL);
}

CCampaignScreen::CCampaignButton::CCampaignButton(const JsonNode & config, const JsonNode & parentConfig, std::string campaignSet)
	: campaignSet(campaignSet)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.x += static_cast<int>(config["x"].Float());
	pos.y += static_cast<int>(config["y"].Float());
	pos.w = 200;
	pos.h = 116;

	campFile = config["file"].String();
	video = VideoPath::fromJson(config["video"]);

	status = CCampaignScreen::ENABLED;

	auto header = CampaignHandler::getHeader(campFile);
	hoverText = header->getNameTranslated();

	if(persistentStorage["completedCampaigns"][header->getFilename()].Bool())
		status = CCampaignScreen::COMPLETED;

	for(const JsonNode & node : parentConfig[campaignSet]["items"].Vector())
	{
		for(const JsonNode & requirement : config["requires"].Vector())
		{
			if(node["id"].Integer() == requirement.Integer())
				if(!persistentStorage["completedCampaigns"][node["file"].String()].Bool())
					status = CCampaignScreen::DISABLED;
		}
	}

	if(persistentStorage["unlockAllCampaigns"].Bool())
		status = CCampaignScreen::ENABLED;

	if(status != CCampaignScreen::DISABLED)
	{
		addUsedEvents(LCLICK | HOVER);
		graphicsImage = std::make_shared<CPicture>(ImagePath::fromJson(config["image"]));

		hoverLabel = std::make_shared<CLabel>(pos.w / 2, pos.h + 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, "");
		parent->addChild(hoverLabel.get());
	}

	if(status == CCampaignScreen::COMPLETED)
		graphicsCompleted = std::make_shared<CPicture>(ImagePath::builtin("CAMPCHK"));
}

void CCampaignScreen::CCampaignButton::show(Canvas & to)
{
	if(status == CCampaignScreen::DISABLED)
		return;

	CIntObject::show(to);

	// Play the campaign button video when the mouse cursor is placed over the button
	if(isHovered())
		CCS->videoh->update(pos.x, pos.y, to.getInternalSurface(), true, false); // plays sequentially frame by frame, starts at the beginning when the video is over
}

void CCampaignScreen::CCampaignButton::clickReleased(const Point & cursorPosition)
{
	CCS->videoh->close();
	CMainMenu::openCampaignLobby(campFile, campaignSet);
}

void CCampaignScreen::CCampaignButton::hover(bool on)
{
	if (on)
		CCS->videoh->open(video);
	else
		CCS->videoh->close();

	if(hoverLabel)
	{
		if(on)
			hoverLabel->setText(hoverText); // Shows the name of the campaign when you get into the bounds of the button
		else
			hoverLabel->setText(" ");
	}
}
