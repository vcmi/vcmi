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

class Bonus;
class CCreature;
class CStackInstance;
class CStack;
class AdventureMapButton;

class CCreatureWindow : public CIntObject
{
public:
	enum CreWinType {OTHER = 0, BATTLE = 1, ARMY = 2, HERO = 3};
	//bool active; //TODO: comment me
	int type;//0 - rclick popup; 1 - normal window
	CPicture *bitmap; //background
	std::string count; //creature count in text format

	boost::function<void()> dsm; //dismiss button callback
	boost::function<void()> Upg; //upgrade button callback
	CCreaturePic *anim; //related creature's animation
	const CCreature *c; //related creature
	std::vector<SComponent*> upgResCost; //cost of upgrade (if not possible then empty)

	MoraleLuckBox *luck, *morale;
	LRClickableAreaWText * expArea; //displays exp details
	CArtPlace *creatureArtifact;

	CSlider * slider; //Abilities
	AdventureMapButton *dismiss, *upgrade, *ok;
	AdventureMapButton * leftArtRoll, * rightArtRoll; //artifact selection - do we need it?
	//TODO: Arifact drop

	//CCreatureWindow(const CStackInstance &st, boost::function<void()> Upg = 0, boost::function<void()> Dsm = 0, UpgradeInfo *ui = NULL); //c-tor
	CCreatureWindow(const CStack & stack, int type); //battle c-tor
	CCreatureWindow (const CStackInstance &stack, int Type); //pop-up c-tor
	CCreatureWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui); //full garrison window
	//CCreatureWindow(int Cid, int Type, int creatureCount); //c-tor
	void init(const CStackInstance *stack, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner);
	void printLine(int nr, const std::string &text, int baseVal, int val=-1, bool range=false);
	//~CCreatureWindow(); //d-tor
	//void activate();
	void close();
	//void clickRight(tribool down, bool previousState); //call-in
	//void dismissF();
	//void keyPressed (const SDL_KeyboardEvent & key); //call-in
	//void deactivate();
	//void show(SDL_Surface * to);
	void scrollArt(int dir);
};

class CBonusItem : LRClickableAreaWTextComp //responsible for displaying creature skill, active or not
{
	const Bonus * bonus;
};