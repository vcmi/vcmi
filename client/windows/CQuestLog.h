#pragma once

#include "../widgets/AdventureMapClasses.h"
#include "../widgets/TextControls.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/Images.h"
#include "CWindowObject.h"

/*
 * CQuestLog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreature;
class CStackInstance;
class CButton;
class CToggleButton;
class CGHeroInstance;
class CComponentBox;
class LRClickableAreaWText;
class CButton;
class CPicture;
class CCreaturePic;
class LRClickableAreaWTextComp;
class CSlider;
class CLabel;
struct QuestInfo;

const int QUEST_COUNT = 6;
const int DESCRIPTION_HEIGHT_MAX = 355;

class CQuestLabel : public LRClickableAreaWText, public CMultiLineLabel
{
public:
	std::function<void()> callback;

	CQuestLabel (Rect position, EFonts Font = FONT_SMALL, EAlignment Align = TOPLEFT, const SDL_Color &Color = Colors::WHITE, const std::string &Text =  "")
		: CMultiLineLabel (position, FONT_SMALL, TOPLEFT, Colors::WHITE, Text){};
	void clickLeft(tribool down, bool previousState) override;
	void showAll(SDL_Surface * to) override;
};

class CQuestIcon : public CAnimImage
{
public:
	std::function<void()> callback; //TODO: merge with other similar classes?

	CQuestIcon (const std::string &defname, int index, int x=0, int y=0);

	void clickLeft(tribool down, bool previousState) override;
	void showAll(SDL_Surface * to) override;
};

class CQuestMinimap : public CMinimap
{
	std::vector <std::shared_ptr<CQuestIcon>> icons;

	void clickLeft(tribool down, bool previousState) override{}; //minimap ignores clicking on its surface
	void iconClicked();
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override{};

public:

	const QuestInfo * currentQuest;

	CQuestMinimap (const Rect & position);
	//should be called to invalidate whole map - different player or level
	void update();
	void addQuestMarks (const QuestInfo * q);

	void showAll(SDL_Surface * to) override;
};

class CQuestLog : public CWindowObject
{
	int questIndex;
	const QuestInfo * currentQuest;
	CComponentBox * componentsBox;
	bool hideComplete;
	CToggleButton * hideCompleteButton;
	CLabel * hideCompleteLabel;

	const std::vector<QuestInfo> quests;
	std::vector <std::shared_ptr<CQuestLabel>> labels;
	CTextBox * description;
	CQuestMinimap * minimap;
	CSlider * slider; //scrolls quests
	CButton *ok;

	void init ();
public:

	CQuestLog (const std::vector<QuestInfo> & Quests);

	~CQuestLog(){};

	void selectQuest (int which, int labelId);
	void updateMinimap (int which){};
	void printDescription (int which){};
	void sliderMoved (int newpos);
	void recreateLabelList();
	void recreateQuestList (int pos);
	void toggleComplete(bool on);
	void showAll (SDL_Surface * to) override;
};
