#include "../global.h"
#include "GUIBase.h"
#include "GUIClasses.h"
#include "../lib/HeroBonus.h"

/*
 * CCreatureWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct Bonus;
class CCreature;
class CStackInstance;
class CStack;
class AdventureMapButton;
class CBonusItem;

class CCreatureWindow : public CIntObject
{
public:
	enum CreWinType {OTHER = 0, BATTLE = 1, ARMY = 2, HERO = 3}; //only last one should open permanently
	//bool active; //TODO: comment me
	int type;//0 - rclick popup; 1 - normal window
	int bonusRows; //height of skill window

	std::string count; //creature count in text format
	const CCreature *c; //related creature
	const CStackInstance *stack;
	const CBonusSystemNode *stackNode;
	const CGHeroInstance *heroOwner;
	const CStack * battleStack; //determine the umber of shots in battle
	std::vector<SComponent*> upgResCost; //cost of upgrade (if not possible then empty)
	std::vector<CBonusItem*> bonusItems;
	std::vector<LRClickableAreaWText*> spellEffects;

	CPicture *bitmap; //background
	CCreaturePic *anim; //related creature's animation
	MoraleLuckBox *luck, *morale;
	LRClickableAreaWTextComp * expArea; //displays exp details
	CArtPlace *creatureArtifact;
	CSlider * slider; //Abilities
	AdventureMapButton *dismiss, *upgrade, *ok;
	AdventureMapButton * leftArtRoll, * rightArtRoll; //artifact selection
	//TODO: Artifact drop

	boost::function<void()> dsm; //dismiss button callback
	boost::function<void()> Upg; //upgrade button callback

	CCreatureWindow(const CStack & stack, int type); //battle c-tor
	CCreatureWindow (const CStackInstance &stack, int Type); //pop-up c-tor
	CCreatureWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui); //full garrison window
	CCreatureWindow(int Cid, int Type, int creatureCount); //c-tor
	void init(const CStackInstance *stack, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner);
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void printLine(int nr, const std::string &text, int baseVal, int val=-1, bool range=false);
	void recreateSkillList(int pos);
	~CCreatureWindow(); //d-tor
	//void activate();
	//void deactivate();
	void close();
	void clickRight(tribool down, bool previousState); //call-in
	void sliderMoved(int newpos);
	//void keyPressed (const SDL_KeyboardEvent & key); //call-in
	void scrollArt(int dir);
};

class CBonusItem : public LRClickableAreaWTextComp //responsible for displaying creature skill, active or not
{
public:
	std::string name, description;
	CPicture * bonusGraphics;
	bool visible;

	CBonusItem();
	CBonusItem(const Rect &Pos, const std::string &Name, const std::string &Description, const std::string &graphicsName);
	~CBonusItem();

	void setBonus (const Bonus &bonus);
	void showAll (SDL_Surface * to);
};
