/*
 * CHighScoreScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../windows/CWindowObject.h"
#include "../../lib/gameState/HighScore.h"

class CButton;
class CLabel;
class CMultiLineLabel;
class CAnimImage;
class CTextInput;
class VideoWidgetBase;
class CFilledTexture;

class TransparentFilledRectangle;

class HighScoreCalculation
{
public:
	std::vector<HighScoreParameter> parameters;
	bool isCampaign = false;

	auto calculate();
	static CreatureID getCreatureForPoints(int points, bool campaign);
};

class CHighScoreScreen : public CWindowObject
{
public:
	enum HighScorePage { SCENARIO, CAMPAIGN };

private:
	void addButtons();
	void addHighScores();
	
	void buttonCampaignClick();
	void buttonScenarioClick();
	void buttonResetClick();
	void buttonExitClick();

	void showPopupWindow(const Point & cursorPosition) override;

	HighScorePage highscorepage;

	std::shared_ptr<CPicture> background;
	std::shared_ptr<CFilledTexture> backgroundAroundMenu;
	std::vector<std::shared_ptr<CButton>> buttons;
	std::vector<std::shared_ptr<CLabel>> texts;
	std::vector<std::shared_ptr<CAnimImage>> images;

	const int screenRows = 11;

	int highlighted;
public:
	CHighScoreScreen(HighScorePage highscorepage, int highlighted = -1);
};

class CHighScoreInput : public CWindowObject
{
	std::shared_ptr<CMultiLineLabel> text;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;
	std::shared_ptr<CTextInput> textInput;

	std::function<void(std::string text)> ready;
	
	void okay();
	void abort();
public:
	CHighScoreInput(std::string playerName, std::function<void(std::string text)> readyCB);
};

class CHighScoreInputScreen : public CWindowObject
{
	std::vector<std::shared_ptr<CMultiLineLabel>> texts;
	std::shared_ptr<CHighScoreInput> input;
	std::shared_ptr<TransparentFilledRectangle> background;
	std::shared_ptr<VideoWidgetBase> videoPlayer;
	std::shared_ptr<CFilledTexture> backgroundAroundMenu;

	bool won;
	HighScoreCalculation calc;
public:
	CHighScoreInputScreen(bool won, HighScoreCalculation calc);

	int addEntry(std::string text);

	void clickPressed(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void show(Canvas & to) override;
};
