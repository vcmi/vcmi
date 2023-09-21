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
class CAnimImage;

class CHighScoreScreen : public CWindowObject
{
    enum HighScorePage { STANDARD, CAMPAIGN };

    void addButtons();
    void addHighScores();
    
    void buttonCampaginClick();
    void buttonStandardClick();
    void buttonResetClick();
    void buttonExitClick();

    HighScorePage highscorepage = HighScorePage::STANDARD;

    std::shared_ptr<CPicture> background;
    std::vector<std::shared_ptr<CButton>> buttons;
    std::vector<std::shared_ptr<CLabel>> texts;
    std::vector<std::shared_ptr<CAnimImage>> images;
public:
	CHighScoreScreen();
};
