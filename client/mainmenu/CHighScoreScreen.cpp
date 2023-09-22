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
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../windows/InfoWindows.h"
#include "../render/Canvas.h"

#include "../CGameInfo.h"
#include "../CVideoHandler.h"
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
        entry2->Integer() = std::rand() % 400 * 5;

        Settings entry3 = persistentStorage.write["highscore"]["scenario"][std::to_string(i)]["player"];
        entry3->String() = "test";
        Settings entry4 = persistentStorage.write["highscore"]["scenario"][std::to_string(i)]["land"];
        entry4->String() = "test";
        Settings entry5 = persistentStorage.write["highscore"]["scenario"][std::to_string(i)]["days"];
        entry5->Integer() = 123;
        Settings entry6 = persistentStorage.write["highscore"]["scenario"][std::to_string(i)]["points"];
        entry6->Integer() = std::rand() % 400;
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

    background = std::make_shared<CPicture>(ImagePath::builtin(highscorepage == HighScorePage::SCENARIO ? "HISCORE" : "HISCORE2"));

    texts.clear();
    images.clear();

    static const JsonNode configCreatures(JsonPath::builtin("CONFIG/highscoreCreatures.json"));
    auto creatures = configCreatures["creatures"].Vector();

    // Header
    texts.push_back(std::make_shared<CLabel>(115, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.433")));
    texts.push_back(std::make_shared<CLabel>(220, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.434")));

    if(highscorepage == HighScorePage::SCENARIO)
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
    auto & data = persistentStorage["highscore"][highscorepage == HighScorePage::SCENARIO ? "scenario" : "campaign"];
    for (int i = 0; i < 11; i++)
    {
        auto & curData = data[std::to_string(i)];

        texts.push_back(std::make_shared<CLabel>(115, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, std::to_string(i+1)));
        texts.push_back(std::make_shared<CLabel>(220, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, curData["player"].String()));
    
        if(highscorepage == HighScorePage::SCENARIO)
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

        int divide = (highscorepage == HighScorePage::SCENARIO) ? 1 : 5;
        for(auto & creature : creatures) {
            if(curData["points"].Integer() / divide <= creature["max"].Integer() && curData["points"].Integer() / divide >= creature["min"].Integer())
                images.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[CreatureID::decode(creature["creature"].String())]->getIconIndex(), 0, 670, y - 15 + i * 50));
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
    highscorepage = HighScorePage::SCENARIO;
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

CHighScoreInputScreen::CHighScoreInputScreen()
	: CWindowObject(BORDERED)
{
    addUsedEvents(LCLICK);

    OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

    int border = 100;
    int textareaW = ((pos.w - 2 * border) / 4);
    std::vector<std::string> t = { "438", "439", "440", "441", "676" };
    for (int i = 0; i < 5; i++)
        texts.push_back(std::make_shared<CMultiLineLabel>(Rect(textareaW * i + border - (textareaW / 2), 450, textareaW, 100), FONT_HIGH_SCORE, ETextAlignment::TOPCENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt." + t[i])));
}

void CHighScoreInputScreen::addEntry(std::string text) {

}

void CHighScoreInputScreen::show(Canvas & to)
{
	CCS->videoh->update(pos.x, pos.y, to.getInternalSurface(), true, false);
    redraw();

	CIntObject::show(to);
}

void CHighScoreInputScreen::activate()
{
    CCS->videoh->open(VideoPath::builtin("HSLOOP.SMK"));
	CIntObject::activate();
}

void CHighScoreInputScreen::deactivate()
{
    CCS->videoh->close();
	CIntObject::deactivate();
}

void CHighScoreInputScreen::clickPressed(const Point & cursorPosition)
{
    OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

    if(!input)
    {
        input = std::make_shared<CHighScoreInput>(
        [&] (std::string text) 
        {
            if(!text.empty())
            {
                addEntry(text);
            }
            close();
        });
    }
}

CHighScoreInput::CHighScoreInput(std::function<void(std::string text)> readyCB)
	: CWindowObject(0), ready(readyCB)
{
    OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos = center(Rect(0, 0, 232, 212));
	updateShadow();

    background = std::make_shared<CPicture>(ImagePath::builtin("HIGHNAME"));
    text = std::make_shared<CMultiLineLabel>(Rect(15, 15, 202, 202), FONT_SMALL, ETextAlignment::TOPCENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.96"));

    buttonOk = std::make_shared<CButton>(Point(26, 142), AnimationPath::builtin("MUBCHCK.DEF"), CGI->generaltexth->zelp[560], std::bind(&CHighScoreInput::okay, this), EShortcut::GLOBAL_ACCEPT);
    buttonCancel = std::make_shared<CButton>(Point(142, 142), AnimationPath::builtin("MUBCANC.DEF"), CGI->generaltexth->zelp[561], std::bind(&CHighScoreInput::abort, this), EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 186, 218, 18), 7, 186));
    textInput = std::make_shared<CTextInput>(Rect(18, 104, 200, 25), FONT_SMALL, 0);
    textInput->setText(settings["general"]["playerName"].String());
}

void CHighScoreInput::okay()
{
    ready(textInput->getText());
}

void CHighScoreInput::abort()
{
    ready("");
}