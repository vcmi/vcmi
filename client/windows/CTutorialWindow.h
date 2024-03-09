/*
 * CTutorialWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CFilledTexture;
class CButton;
class CLabel;
class CMultiLineLabel;

enum TutorialMode
{
	TOUCH_ADVENTUREMAP,
	TOUCH_BATTLE
};

class CTutorialWindow : public CWindowObject
{
	TutorialMode mode;
	std::shared_ptr<CFilledTexture> background;

	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonLeft;
	std::shared_ptr<CButton> buttonRight;

	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<CMultiLineLabel> labelInformation;

	std::string video;
	std::vector<std::string> videos;

	int page;

	void exit();
	void next();
	void previous();
	void setContent();

public:
	CTutorialWindow(const TutorialMode & m);
	static void openWindowFirstTime(const TutorialMode & m);	

	void show(Canvas & to) override;
	void activate() override;
	void deactivate() override;
};
