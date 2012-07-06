#include "UIFramework/CIntObject.h"
#include "AdventureMapClasses.h"
#include "GUIClasses.h"

#include "../lib/CGameState.h"

/*
 * CCreatureWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreature;
class CStackInstance;
class CAdventureMapButton;
class CGHeroInstance;
class CComponent;
class LRClickableAreaWText;
class CAdventureMapButton;
class CPicture;
class CCreaturePic;
class LRClickableAreaWTextComp;
class CSlider;
class CLabel;
struct QuestInfo;

class CQuestMinimap : public CMinimap
{
	void clickLeft(tribool down, bool previousState){};
	void mouseMoved (const SDL_MouseMotionEvent & sEvent){};

public:

	CQuestMinimap (const Rect & position) : CMinimap (position){};
	//should be called to invalidate whole map - different player or level
	void update(){};
	void setLevel(int level){};
	void addQuestMarks (QuestInfo q){};

	void showAll(SDL_Surface * to){};
};

class CQuestLog : public CWindowObject
{
	const std::vector<QuestInfo> quests;
	CTextBox * description;
	CQuestMinimap * minimap;
	CSlider * slider; //scrolls quests
	CAdventureMapButton *ok;

public:

	CQuestLog (const std::vector<QuestInfo> & Quests);

	~CQuestLog(){};

	void init ();
	void selectQuest (int which){};
	void updateMinimap (int which){};
	void printDescription (int which){};
	void sliderMoved(int newpos){};
};