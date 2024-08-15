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
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/VideoWidget.h"
#include "../windows/InfoWindows.h"
#include "../widgets/TextControls.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"

#include "../CGameInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/gameState/HighScore.h"

CHighScoreScreen::CHighScoreScreen(HighScorePage highscorepage, int highlighted)
	: CWindowObject(BORDERED), highscorepage(highscorepage), highlighted(highlighted)
{
	addUsedEvents(SHOW_POPUP);

	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 800, 600));

	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(-pos.x, -pos.y, GH.screenDimensions().x, GH.screenDimensions().y));

	addHighScores();
	addButtons();
}

void CHighScoreScreen::showPopupWindow(const Point & cursorPosition)
{
	for (int i = 0; i < screenRows; i++)
	{
		bool currentGameNotInListEntry = i == (screenRows - 1) && highlighted > (screenRows - 1);

		Rect r = Rect(80, 40 + i * 50, 635, 50);
		if(r.isInside(cursorPosition - pos))
		{
			std::string tmp = persistentStorage["highscore"][highscorepage == HighScorePage::SCENARIO ? "scenario" : "campaign"][currentGameNotInListEntry ? highlighted : i]["datetime"].String();
			if(!tmp.empty())
				CRClickPopup::createAndPush(tmp);
		}
	}
}

void CHighScoreScreen::addButtons()
{
	OBJECT_CONSTRUCTION;
	
	buttons.clear();

	buttons.push_back(std::make_shared<CButton>(Point(31, 113), AnimationPath::builtin("HISCCAM.DEF"), CButton::tooltip(),  [this](){ buttonCampaignClick(); }, EShortcut::HIGH_SCORES_CAMPAIGNS));
	buttons.push_back(std::make_shared<CButton>(Point(31, 345), AnimationPath::builtin("HISCSTA.DEF"), CButton::tooltip(),  [this](){ buttonScenarioClick(); }, EShortcut::HIGH_SCORES_SCENARIOS));
	buttons.push_back(std::make_shared<CButton>(Point(726, 113), AnimationPath::builtin("HISCRES.DEF"), CButton::tooltip(), [this](){ buttonResetClick(); }, EShortcut::HIGH_SCORES_RESET));
	buttons.push_back(std::make_shared<CButton>(Point(726, 345), AnimationPath::builtin("HISCEXT.DEF"), CButton::tooltip(), [this](){ buttonExitClick(); }, EShortcut::GLOBAL_RETURN));
}

void CHighScoreScreen::addHighScores()
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::builtin(highscorepage == HighScorePage::SCENARIO ? "HISCORE" : "HISCORE2"));

	texts.clear();
	images.clear();

	// Header
	texts.push_back(std::make_shared<CLabel>(115, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.433"))); // rank
	texts.push_back(std::make_shared<CLabel>(225, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.434"))); // player

	if(highscorepage == HighScorePage::SCENARIO)
	{
		texts.push_back(std::make_shared<CLabel>(405, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.435"))); // land
		texts.push_back(std::make_shared<CLabel>(557, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.436"))); // days
		texts.push_back(std::make_shared<CLabel>(627, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.75"))); // score
	}
	else
	{
		texts.push_back(std::make_shared<CLabel>(405, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.672"))); // campaign
		texts.push_back(std::make_shared<CLabel>(592, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.75"))); // score
	}

	// Content
	int y = 66;
	auto & data = persistentStorage["highscore"][highscorepage == HighScorePage::SCENARIO ? "scenario" : "campaign"];
	for (int i = 0; i < screenRows; i++)
	{
		bool currentGameNotInListEntry = (i == (screenRows - 1) && highlighted > (screenRows - 1));
		auto & curData = data[currentGameNotInListEntry ? highlighted : i];

		ColorRGBA color = (i == highlighted || currentGameNotInListEntry) ? Colors::YELLOW : Colors::WHITE;

		texts.push_back(std::make_shared<CLabel>(115, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string((currentGameNotInListEntry ? highlighted : i) + 1)));
		texts.push_back(std::make_shared<CLabel>(225, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, curData["player"].String(), 120));

		if(highscorepage == HighScorePage::SCENARIO)
		{
			texts.push_back(std::make_shared<CLabel>(405, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, curData["scenarioName"].String(), 200));
			texts.push_back(std::make_shared<CLabel>(557, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["days"].Integer())));
			texts.push_back(std::make_shared<CLabel>(627, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["points"].Integer())));
		}
		else
		{
			texts.push_back(std::make_shared<CLabel>(405, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, curData["campaignName"].String(), 200));
			texts.push_back(std::make_shared<CLabel>(592, y + i * 50, FONT_MEDIUM, ETextAlignment::CENTER, color, std::to_string(curData["points"].Integer())));
		}

		if(curData["points"].Integer() > 0 && curData["points"].Integer() <= ((highscorepage == HighScorePage::CAMPAIGN) ? 2500 : 500))
			images.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[HighScoreCalculation::getCreatureForPoints(curData["points"].Integer(), highscorepage == HighScorePage::CAMPAIGN)]->getIconIndex(), 0, 670, y - 15 + i * 50));
	}
}

void CHighScoreScreen::buttonCampaignClick()
{
	highscorepage = HighScorePage::CAMPAIGN;
	addHighScores();
	addButtons();
	redraw();
}

void CHighScoreScreen::buttonScenarioClick()
{
	OBJECT_CONSTRUCTION;
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
	addUsedEvents(LCLICK | KEYBOARD);

	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 800, 600));

	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(-pos.x, -pos.y, GH.screenDimensions().x, GH.screenDimensions().y));
	background = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), Colors::BLACK);

	if(won)
	{

		videoPlayer = std::make_shared<VideoWidget>(Point(0, 0), VideoPath::builtin("HSANIM.SMK"), VideoPath::builtin("HSLOOP.SMK"), true);

		int border = 100;
		int textareaW = ((pos.w - 2 * border) / 4);
		std::vector<std::string> t = { "438", "439", "440", "441", "676" }; // time, score, difficulty, final score, rank
		for (int i = 0; i < 5; i++)
			texts.push_back(std::make_shared<CMultiLineLabel>(Rect(textareaW * i + border - (textareaW / 2), 450, textareaW, 100), FONT_HIGH_SCORE, ETextAlignment::TOPCENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt." + t[i])));

		std::string creatureName = (calc.calculate().cheater) ? CGI->generaltexth->translate("core.genrltxt.260") : (*CGI->creh)[HighScoreCalculation::getCreatureForPoints(calc.calculate().total, calc.isCampaign)]->getNameSingularTranslated();
		t = { std::to_string(calc.calculate().sumDays), std::to_string(calc.calculate().basic), CGI->generaltexth->translate("core.arraytxt." + std::to_string((142 + calc.parameters[0].difficulty))), std::to_string(calc.calculate().total), creatureName };
		for (int i = 0; i < 5; i++)
			texts.push_back(std::make_shared<CMultiLineLabel>(Rect(textareaW * i + border - (textareaW / 2), 530, textareaW, 100), FONT_HIGH_SCORE, ETextAlignment::TOPCENTER, Colors::WHITE, t[i]));
 
		CCS->musich->playMusic(AudioPath::builtin("music/Win Scenario"), true, true);
	}
	else
	{
		videoPlayer = std::make_shared<VideoWidgetOnce>(Point(0, 0), VideoPath::builtin("LOSEGAME.SMK"), true, [this](){close();});
		CCS->musich->playMusic(AudioPath::builtin("music/UltimateLose"), false, true);
	}
}

int CHighScoreInputScreen::addEntry(std::string text) {
	std::vector<JsonNode> baseNode = persistentStorage["highscore"][calc.isCampaign ? "campaign" : "scenario"].Vector();
	
	auto sortFunctor = [](const JsonNode & left, const JsonNode & right)
	{
		if(left["points"].Integer() == right["points"].Integer())
			return left["posFlag"].Bool() > right["posFlag"].Bool();
		return left["points"].Integer() > right["points"].Integer();
	};

	JsonNode newNode = JsonNode();
	newNode["player"].String() = text;
	if(calc.isCampaign)
		newNode["campaignName"].String() = calc.calculate().cheater ? CGI->generaltexth->translate("core.genrltxt.260") : calc.parameters[0].campaignName;
	else
		newNode["scenarioName"].String() = calc.calculate().cheater ? CGI->generaltexth->translate("core.genrltxt.260") : calc.parameters[0].scenarioName;
	newNode["days"].Integer() = calc.calculate().sumDays;
	newNode["points"].Integer() = calc.calculate().cheater ? 0 : calc.calculate().total;
	newNode["datetime"].String() = TextOperations::getFormattedDateTimeLocal(std::time(nullptr));
	newNode["posFlag"].Bool() = true;

	baseNode.push_back(newNode);
	boost::range::sort(baseNode, sortFunctor);

	int pos = -1;
	for (int i = 0; i < baseNode.size(); i++)
	{
		if(!baseNode[i]["posFlag"].isNull())
		{
			baseNode[i]["posFlag"].clear();
			pos = i;
		}
	}

	Settings s = persistentStorage.write["highscore"][calc.isCampaign ? "campaign" : "scenario"];
	s->Vector() = baseNode;

	return pos;
}

void CHighScoreInputScreen::show(Canvas & to)
{
	CWindowObject::showAll(to);
}

void CHighScoreInputScreen::clickPressed(const Point & cursorPosition)
{
	OBJECT_CONSTRUCTION;

	if(!won)
	{
		close();
		return;
	}

	if(!input)
	{
		input = std::make_shared<CHighScoreInput>(calc.parameters[0].playerName,
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

void CHighScoreInputScreen::keyPressed(EShortcut key)
{
	clickPressed(Point());
}

CHighScoreInput::CHighScoreInput(std::string playerName, std::function<void(std::string text)> readyCB)
	: CWindowObject(NEEDS_ANIMATED_BACKGROUND, ImagePath::builtin("HIGHNAME")), ready(readyCB)
{
	OBJECT_CONSTRUCTION;

	pos = center(Rect(0, 0, 232, 212));
	updateShadow();

	text = std::make_shared<CMultiLineLabel>(Rect(15, 15, 202, 202), FONT_SMALL, ETextAlignment::TOPCENTER, Colors::WHITE, CGI->generaltexth->translate("core.genrltxt.96"));

	buttonOk = std::make_shared<CButton>(Point(26, 142), AnimationPath::builtin("MUBCHCK.DEF"), CGI->generaltexth->zelp[560], std::bind(&CHighScoreInput::okay, this), EShortcut::GLOBAL_ACCEPT);
	buttonCancel = std::make_shared<CButton>(Point(142, 142), AnimationPath::builtin("MUBCANC.DEF"), CGI->generaltexth->zelp[561], std::bind(&CHighScoreInput::abort, this), EShortcut::GLOBAL_CANCEL);
// FIXME: broken. Never activates?
//	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 186, 218, 18), 7, 186));
	textInput = std::make_shared<CTextInput>(Rect(18, 104, 200, 25), FONT_SMALL, ETextAlignment::CENTER, true);
	textInput->setText(playerName);
}

void CHighScoreInput::okay()
{
	ready(textInput->getText());
}

void CHighScoreInput::abort()
{
	ready("");
}
