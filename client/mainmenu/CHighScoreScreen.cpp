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
#include "../CMusicHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/TextOperations.h"

#include "vstd/DateUtils.h"

auto HighScoreCalculation::calculate()
{
	struct result
	{
		int basic;
		int total;
		int sumDays;
		bool cheater;
	};
	
	std::vector<int> scoresBasic;
	std::vector<int> scoresTotal;
	double sumBasic = 0;
	double sumTotal = 0;
	int sumDays = 0;
	bool cheater = false;
	for(auto & param : parameters)
	{
		double tmp = 200 - (param.day + 10) / (param.townAmount + 5) + (param.allDefeated ? 25 : 0) + (param.hasGrail ? 25 : 0);
		scoresBasic.push_back(static_cast<int>(tmp));
		sumBasic += tmp;
		if(param.difficulty == 0)
			tmp *= 0.8;
		if(param.difficulty == 1)
			tmp *= 1.0;
		if(param.difficulty == 2)
			tmp *= 1.3;
		if(param.difficulty == 3)
			tmp *= 1.6;
		if(param.difficulty == 4)
			tmp *= 2.0;
		scoresTotal.push_back(static_cast<int>(tmp));
		sumTotal += tmp;
		sumDays += param.day;
		if(param.usedCheat)
			cheater = true;
	}

	if(scoresBasic.size() == 1)
		return result { scoresBasic[0], scoresTotal[0], sumDays , cheater};

	return result { static_cast<int>((sumBasic / parameters.size()) * 5.0), static_cast<int>((sumTotal / parameters.size()) * 5.0), sumDays, cheater };
}

CreatureID HighScoreCalculation::getCreatureForPoints(int points, bool campaign)
{
	static const JsonNode configCreatures(JsonPath::builtin("CONFIG/highscoreCreatures.json"));
	auto creatures = configCreatures["creatures"].Vector();
	int divide = campaign ? 5 : 1;

	for(auto & creature : creatures)
		if(points / divide <= creature["max"].Integer() && points / divide >= creature["min"].Integer())
			return CreatureID::decode(creature["creature"].String());

	return -1;
}

CHighScoreScreen::CHighScoreScreen(HighScorePage highscorepage, int highlighted)
	: CWindowObject(BORDERED), highscorepage(highscorepage), highlighted(highlighted)
{
	addUsedEvents(SHOW_POPUP);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

	addHighScores();
	addButtons();
}

void CHighScoreScreen::showPopupWindow(const Point & cursorPosition)
{
	for (int i = 0; i < 11; i++)
	{
		Rect r = Rect(80, 40 + i * 50, 635, 50);
		if(r.isInside(cursorPosition - pos))
		{
			std::string tmp = persistentStorage["highscore"][highscorepage == HighScorePage::SCENARIO ? "scenario" : "campaign"][std::to_string(i)]["datetime"].String();
			if(!tmp.empty())
				CRClickPopup::createAndPush(tmp);
		}
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
		ColorRGBA color = (i == highlighted) ? Colors::YELLOW : Colors::WHITE;

		texts.push_back(std::make_shared<CLabel>(115, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(i+1)));
		std::string tmp = curData["player"].String();
		TextOperations::trimRightUnicode(tmp, std::max(0, (int)TextOperations::getUnicodeCharactersCount(tmp) - 13));
		texts.push_back(std::make_shared<CLabel>(220, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, tmp));

		if(highscorepage == HighScorePage::SCENARIO)
		{
			std::string tmp = curData["land"].String();
			TextOperations::trimRightUnicode(tmp, std::max(0, (int)TextOperations::getUnicodeCharactersCount(tmp) - 25));
			texts.push_back(std::make_shared<CLabel>(400, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, tmp));
			texts.push_back(std::make_shared<CLabel>(555, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["days"].Integer())));
			texts.push_back(std::make_shared<CLabel>(625, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["points"].Integer())));
		}
		else
		{
			std::string tmp = curData["campaign"].String();
			TextOperations::trimRightUnicode(tmp, std::max(0, (int)TextOperations::getUnicodeCharactersCount(tmp) - 25));
			texts.push_back(std::make_shared<CLabel>(410, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, tmp));
			texts.push_back(std::make_shared<CLabel>(590, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["points"].Integer())));
		}

		if(curData["points"].Integer() > 0)
			images.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[HighScoreCalculation::getCreatureForPoints(curData["points"].Integer(), highscorepage == HighScorePage::CAMPAIGN)]->getIconIndex(), 0, 670, y - 15 + i * 50));
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

CHighScoreInputScreen::CHighScoreInputScreen(bool won, HighScoreCalculation calc)
	: CWindowObject(BORDERED), won(won), calc(calc)
{
	addUsedEvents(LCLICK);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

	if(won)
	{
		int border = 100;
		int textareaW = ((pos.w - 2 * border) / 4);
		std::vector<std::string> t = { "438", "439", "440", "441", "676" };
		for (int i = 0; i < 5; i++)
			texts.push_back(std::make_shared<CMultiLineLabel>(Rect(textareaW * i + border - (textareaW / 2), 450, textareaW, 100), FONT_HIGH_SCORE, ETextAlignment::TOPCENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt." + t[i])));

		std::string creatureName = (calc.calculate().cheater) ? CGI->generaltexth->translate("core.genrltxt.260") : (*CGI->creh)[HighScoreCalculation::getCreatureForPoints(calc.calculate().total, calc.isCampaign)]->getNameSingularTranslated();
		t = { std::to_string(calc.calculate().sumDays), std::to_string(calc.calculate().basic), CGI->generaltexth->translate("core.arraytxt." + std::to_string((142 + calc.parameters[0].difficulty))), std::to_string(calc.calculate().total), creatureName };
		for (int i = 0; i < 5; i++)
			texts.push_back(std::make_shared<CMultiLineLabel>(Rect(textareaW * i + border - (textareaW / 2), 530, textareaW, 100), FONT_HIGH_SCORE, ETextAlignment::TOPCENTER, Colors::WHITE, t[i]));
 
		CCS->musich->playMusic(AudioPath::builtin("music/Win Scenario"), true, true);
	}
	else
		CCS->musich->playMusic(AudioPath::builtin("music/UltimateLose"), false, true);

	video = won ? "HSANIM.SMK" : "LOSEGAME.SMK";
}

int CHighScoreInputScreen::addEntry(std::string text) {
	for (int i = 0; i < 11; i++)
	{
		if(calc.calculate().cheater)
			i = 10;

		JsonNode node = persistentStorage["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(i)];
		
		if(node["points"].isNull() || node["points"].Integer() <= calc.calculate().total)
		{
			// move following entries down
			for (int j = 10; j + 1 >= i; j--)
			{
				JsonNode node = persistentStorage["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(j - 1)];
				Settings entry = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(j)];
				entry->Struct() = node.Struct();
			}

			Settings entry = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(i)]["player"];
			entry->String() = text;
			if(calc.isCampaign)
			{
				Settings entry2 = persistentStorage.write["highscore"]["campaign"][std::to_string(i)]["campaign"];
				entry2->String() = calc.calculate().cheater ? CGI->generaltexth->translate("core.genrltxt.260") : calc.parameters[0].campaign;
			}
			else
			{
				Settings entry3 = persistentStorage.write["highscore"]["scenario"][std::to_string(i)]["land"];
				entry3->String() = calc.calculate().cheater ? CGI->generaltexth->translate("core.genrltxt.260") : calc.parameters[0].land;
			}
			Settings entry4 = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(i)]["days"];
			entry4->Integer() = calc.calculate().sumDays;
			Settings entry5 = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(i)]["points"];
			entry5->Integer() = calc.calculate().cheater ? 0 : calc.calculate().total;

			Settings entry6 = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"][std::to_string(i)]["datetime"];
			entry6->String() = vstd::getFormattedDateTime(std::time(0));

			return i;
		}
	}

	return -1;
}

void CHighScoreInputScreen::show(Canvas & to)
{
	CCS->videoh->update(pos.x, pos.y, to.getInternalSurface(), true, false,
	[&]()
	{
		if(won)
		{
			CCS->videoh->close();
			video = "HSLOOP.SMK";
			CCS->videoh->open(VideoPath::builtin(video));
		}
		else
			close();
	});
	redraw();

	CIntObject::show(to);
}

void CHighScoreInputScreen::activate()
{
	CCS->videoh->open(VideoPath::builtin(video));
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

	if(!won)
	{
		close();
		return;
	}

	if(!input)
	{
		input = std::make_shared<CHighScoreInput>(
		[&] (std::string text) 
		{
			if(!text.empty())
			{
				int pos = addEntry(text);
				close();
				GH.windows().createAndPushWindow<CHighScoreScreen>(calc.isCampaign ? CHighScoreScreen::HighScorePage::CAMPAIGN : CHighScoreScreen::HighScorePage::SCENARIO, pos);
			}
			else
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