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

#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

class CLabel;
class CPicture;
class CButton;
class VideoWidget;

class CCampaignScreen : public CWindowObject
{
public:
	enum CampaignStatus {DEFAULT = 0, ENABLED, DISABLED, COMPLETED}; // the status of the campaign

private:
	/// A button which plays a video when you move the mouse cursor over it
	class CCampaignButton : public CIntObject
	{
	private:
		std::shared_ptr<CLabel> hoverLabel;
		std::shared_ptr<CPicture> graphicsImage;
		std::shared_ptr<CPicture> graphicsCompleted;
		std::shared_ptr<VideoWidget> videoPlayer;
		CampaignStatus status;
		VideoPath videoPath;

		std::string campFile; // the filename/resourcename of the campaign
		std::string hoverText;

		std::string campaignSet;

		void clickReleased(const Point & cursorPosition) override;
		void hover(bool on) override;

	public:
		CCampaignButton(const JsonNode & config, const JsonNode & parentConfig, std::string campaignSet);
	};

	std::string campaignSet;

	std::vector<std::shared_ptr<CCampaignButton>> campButtons;
	std::vector<std::shared_ptr<CPicture>> images;
	std::shared_ptr<CButton> buttonBack;
	std::shared_ptr<CButton> buttonNext;
	std::shared_ptr<CButton> buttonPrev;
	std::shared_ptr<CLabel> page;

	std::shared_ptr<CButton> createExitButton(const JsonNode & button);

	int campaignsPerPage = 8;
	int currentPage = 0;
	int maxPages = 0;

	void switchPage(int delta);
	void updateCampaignButtons(const JsonNode& parentConfig);

public:
	CCampaignScreen(const JsonNode & config, std::string campaignSet);

	void activate() override;
};
