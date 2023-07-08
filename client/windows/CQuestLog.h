/*
 * CQuestLog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/TextControls.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/Images.h"
#include "../adventureMap/CMinimap.h"
#include "CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCreature;
class CStackInstance;
class CGHeroInstance;
struct QuestInfo;

VCMI_LIB_NAMESPACE_END

class CButton;
class CToggleButton;
class CComponentBox;
class LRClickableAreaWText;
class CButton;
class CPicture;
class CCreaturePic;
class LRClickableAreaWTextComp;
class CSlider;
class CLabel;

const int QUEST_COUNT = 6;
const int DESCRIPTION_HEIGHT_MAX = 355;

class CQuestLabel : public LRClickableAreaWText, public CMultiLineLabel
{
public:
	std::function<void()> callback;

	CQuestLabel(Rect position, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const SDL_Color &Color = Colors::WHITE, const std::string &Text =  "")
		: CMultiLineLabel (position, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, Text){};
	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

class CQuestIcon : public CAnimImage
{
public:
	std::function<void()> callback; //TODO: merge with other similar classes?

	CQuestIcon(const std::string &defname, int index, int x=0, int y=0);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

class CQuestMinimap : public CMinimap
{
	std::vector<std::shared_ptr<CQuestIcon>> icons;

	void clickPressed(const Point & cursorPosition) override{}; //minimap ignores clicking on its surface
	void iconClicked();
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override{};

public:
	const QuestInfo * currentQuest;

	CQuestMinimap(const Rect & position);
	//should be called to invalidate whole map - different player or level
	void update();
	void addQuestMarks (const QuestInfo * q);

	void showAll(Canvas & to) override;
};

class CQuestLog : public CWindowObject
{
	int questIndex;
	const QuestInfo * currentQuest;
	std::shared_ptr<CComponentBox> componentsBox;
	bool hideComplete;
	std::shared_ptr<CToggleButton> hideCompleteButton;
	std::shared_ptr<CLabel> hideCompleteLabel;

	const std::vector<QuestInfo> quests;
	std::vector<std::shared_ptr<CQuestLabel>> labels;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<CQuestMinimap> minimap;
	std::shared_ptr<CSlider> slider; //scrolls quests
	std::shared_ptr<CButton> ok;

public:
	CQuestLog(const std::vector<QuestInfo> & Quests);

	~CQuestLog(){};

	void selectQuest (int which, int labelId);
	void updateMinimap (int which){};
	void printDescription (int which){};
	void sliderMoved (int newpos);
	void recreateLabelList();
	void recreateQuestList (int pos);
	void toggleComplete(bool on);
	void showAll (Canvas & to) override;
};
