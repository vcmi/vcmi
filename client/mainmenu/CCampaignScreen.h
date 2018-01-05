/*
 * CCampaignScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "windows/CWindowObject.h"

class CLabel;
class CButton;
class SDL_Surface;
class JsonNode;
/// Campaign selection screen
class CCampaignScreen : public CIntObject
{
public:
	enum CampaignStatus {DEFAULT = 0, ENABLED, DISABLED, COMPLETED}; // the status of the campaign

private:
	/// A button which plays a video when you move the mouse cursor over it
	class CCampaignButton : public CIntObject
	{
	private:
		CLabel * hoverLabel;
		CampaignStatus status;

		std::string campFile; // the filename/resourcename of the campaign
		std::string video; // the resource name of the video
		std::string hoverText;

		void clickLeft(tribool down, bool previousState) override;
		void hover(bool on) override;

	public:
		CCampaignButton(const JsonNode & config);
		void show(SDL_Surface * to) override;
	};

	std::vector<CCampaignButton *> campButtons;
	std::vector<CPicture *> images;

	CButton * createExitButton(const JsonNode & button);

public:
	enum CampaignSet {ROE, AB, SOD, WOG};

	CCampaignScreen(const JsonNode & config);
	void showAll(SDL_Surface * to) override;
};
