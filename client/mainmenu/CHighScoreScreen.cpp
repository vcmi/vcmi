/*
 * CHighScoreScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CHighScoreScreen.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../windows/InfoWindows.h"
#include "../render/Canvas.h"

#include "../CGameInfo.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/EntityIdentifiers.h"

CHighScoreScreen::CHighScoreScreen()
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

    addHighScores();
    addButtons();

    // TODO: remove; only for testing
    for (int i = 0; i < 11; i++)
    {
        Settings entry = persistentStorage.write["highscore"]["campaign"][std::to_string(i)]["player"];
        entry->String() = "test";
        Settings entry1 = persistentStorage.write["highscore"]["campaign"][std::to_string(i)]["campaign"];
        entry1->String() = "test";
        Settings entry2 = persistentStorage.write["highscore"]["campaign"][std::to_string(i)]["points"];
        entry2->Integer() = 100;

        Settings entry3 = persistentStorage.write["highscore"]["standard"][std::to_string(i)]["player"];
        entry3->String() = "test";
        Settings entry4 = persistentStorage.write["highscore"]["standard"][std::to_string(i)]["land"];
        entry4->String() = "test";
        Settings entry5 = persistentStorage.write["highscore"]["standard"][std::to_string(i)]["days"];
        entry5->Integer() = 123;
        Settings entry6 = persistentStorage.write["highscore"]["standard"][std::to_string(i)]["points"];
        entry6->Integer() = 100;
    }
}

void CHighScoreScreen::addButtons()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
    
    buttons.clear();

    buttons.push_back(std::make_shared<CButton>(Point(31, 113), AnimationPath::builtin("HISCCAM.DEF"), CButton::tooltip(), [&](){ buttonCampaginClick(); }));
    buttons.push_back(std::make_shared<CButton>(Point(31, 345), AnimationPath::builtin("HISCSTA.DEF"), CButton::tooltip(), [&](){ buttonStandardClick(); }));
    buttons.push_back(std::make_shared<CButton>(Point(726, 113), AnimationPath::builtin("HISCRES.DEF"), CButton::tooltip(), [&](){ buttonResetClick(); }));
    buttons.push_back(std::make_shared<CButton>(Point(726, 345), AnimationPath::builtin("HISCEXT.DEF"), CButton::tooltip(), [&](){ buttonExitClick(); }));
}

void CHighScoreScreen::addHighScores()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

    background = std::make_shared<CPicture>(ImagePath::builtin(highscorepage == HighScorePage::STANDARD ? "HISCORE" : "HISCORE2"));

    texts.clear();

    static const JsonNode configCreatures(JsonPath::builtin("CONFIG/highscoreCreatures.json"));
    auto creatures = configCreatures["creatures"].Vector();
    for(auto & creature : creatures) {
        images.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[CreatureID::decode(creature["creature"].String())]->getIconIndex(), 0, 10, 10));
    }

    // Header
    texts.push_back(std::make_shared<CLabel>(115, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.433")));
    texts.push_back(std::make_shared<CLabel>(220, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.434")));

    if(highscorepage == HighScorePage::STANDARD)
    {
        texts.push_back(std::make_shared<CLabel>(400, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.435")));
        texts.push_back(std::make_shared<CLabel>(555, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.436")));
        texts.push_back(std::make_shared<CLabel>(625, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.75")));
    }
    else
    {
        texts.push_back(std::make_shared<CLabel>(410, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.672")));
        texts.push_back(std::make_shared<CLabel>(590, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.75")));
    }

    // Content
    int y = 65;
    auto & data = persistentStorage["highscore"][highscorepage == HighScorePage::STANDARD ? "standard" : "campaign"];
    for (int i = 0; i < 11; i++)
    {
        auto & curData = data[std::to_string(i)];

        texts.push_back(std::make_shared<CLabel>(115, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, std::to_string(i)));
        texts.push_back(std::make_shared<CLabel>(220, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, curData["player"].String()));
    
        if(highscorepage == HighScorePage::STANDARD)
        {
            texts.push_back(std::make_shared<CLabel>(400, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, curData["land"].String()));
            texts.push_back(std::make_shared<CLabel>(555, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, std::to_string(curData["days"].Integer())));
            texts.push_back(std::make_shared<CLabel>(625, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, std::to_string(curData["points"].Integer())));
        }
        else
        {
            texts.push_back(std::make_shared<CLabel>(410, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, curData["campaign"].String()));
            texts.push_back(std::make_shared<CLabel>(590, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, std::to_string(curData["points"].Integer())));
        }
    }
}

void CHighScoreScreen::buttonCampaginClick()
{
    highscorepage = HighScorePage::CAMPAIGN;
    addHighScores();
    addButtons();
    redraw();
}

void CHighScoreScreen::buttonStandardClick()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
    highscorepage = HighScorePage::STANDARD;
    addHighScores();
    addButtons();
    redraw();
}

void CHighScoreScreen::buttonResetClick()
{
    CInfoWindow::showYesNoDialog(
        CGI->generaltexth->allTexts[666],
        {},
        [this]()
        {
            Settings entry = persistentStorage.write["highscore"];
            entry->clear();
            addHighScores();
            addButtons();
            redraw();
        },
        0
    );
}

void CHighScoreScreen::buttonExitClick()
{
    close();
}
