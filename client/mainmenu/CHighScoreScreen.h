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

class CButton;
class CLabel;
class CMultiLineLabel;
class CAnimImage;
class CTextInput;

class HighScoreParameter
{
public:
    int difficulty;
    int day;
    int townAmount;
    bool usedCheat;
    bool hasGrail;
    bool allDefeated;
};

class HighScoreCalculation
{
public:
    std::vector<HighScoreParameter> parameters = std::vector<HighScoreParameter>();
    auto calculate();
};

class CHighScoreScreen : public CWindowObject
{
    enum HighScorePage { SCENARIO, CAMPAIGN };

    void addButtons();
    void addHighScores();
    
    void buttonCampaginClick();
    void buttonStandardClick();
    void buttonResetClick();
    void buttonExitClick();

    HighScorePage highscorepage = HighScorePage::SCENARIO;

    std::shared_ptr<CPicture> background;
    std::vector<std::shared_ptr<CButton>> buttons;
    std::vector<std::shared_ptr<CLabel>> texts;
    std::vector<std::shared_ptr<CAnimImage>> images;
public:
	CHighScoreScreen();
};

class CHighScoreInput : public CWindowObject
{
    std::shared_ptr<CPicture> background;
    std::shared_ptr<CMultiLineLabel> text;
    std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;
    std::shared_ptr<CTextInput> textInput;

    std::function<void(std::string text)> ready;
    
    void okay();
    void abort();
public:
	CHighScoreInput(std::function<void(std::string text)> readyCB);
};

class CHighScoreInputScreen : public CWindowObject
{
    std::vector<std::shared_ptr<CMultiLineLabel>> texts;
    std::shared_ptr<CHighScoreInput> input;

    std::string video;
    bool won;
    HighScoreCalculation calc;
public:
	CHighScoreInputScreen(bool won, HighScoreCalculation calc);

    void addEntry(std::string text);

	void show(Canvas & to) override;
	void activate() override;
	void deactivate() override;
	void clickPressed(const Point & cursorPosition) override;
};