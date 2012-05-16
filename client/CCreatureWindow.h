#pragma once

#include "UIFramework/CIntObject.h"
#include "../lib/HeroBonus.h"
#include "GUIClasses.h"

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
class CCommanderInstance;
class CStack;
class ArtifactLocation;
class CCreatureArtifactInstance;
class CAdventureMapButton;
class CBonusItem;
class CGHeroInstance;
class CComponent;
class LRClickableAreaWText;
class MoraleLuckBox;
class CAdventureMapButton;
struct UpgradeInfo;
class CPicture;
class CCreaturePic;
class LRClickableAreaWTextComp;
class CSlider;
class CLabel;
class CAnimImage;

// New creature window
class CCreatureWindow : public CArtifactHolder
{
public:
	enum CreWinType {OTHER = 0, BATTLE = 1, ARMY = 2, HERO = 3, COMMANDER = 4}; // > 3 are opened permanently
	//bool active; //TODO: comment me
	int type;//0 - rclick popup; 1 - normal window
	int bonusRows; //height of skill window

	std::string count; //creature count in text format
	const CCreature *c; //related creature
	const CStackInstance *stack;
	const CBonusSystemNode *stackNode;
	const CCommanderInstance * commander;
	const CGHeroInstance *heroOwner;
	const CArtifactInstance *creatureArtifact; //currently worn artifact
	std::vector<CComponent*> upgResCost; //cost of upgrade (if not possible then empty)
	std::vector<CBonusItem*> bonusItems;
	std::vector<LRClickableAreaWText*> spellEffects;

	CPicture *bitmap; //background
	CCreaturePic *anim; //related creature's animation
	MoraleLuckBox *luck, *morale;
	LRClickableAreaWTextComp * expArea; //displays exp details
	CSlider * slider; //Abilities
	CAdventureMapButton *dismiss, *upgrade, *ok;
	CAdventureMapButton * leftArtRoll, * rightArtRoll; //artifact selection
	CAdventureMapButton * passArtToHero;
	CAnimImage *artifactImage;

	void setArt(const CArtifactInstance *creatureArtifact);

	void artifactRemoved (const ArtifactLocation &artLoc);
	void artifactMoved (const ArtifactLocation &artLoc, const ArtifactLocation &destLoc);
	void artifactDisassembled (const ArtifactLocation &artLoc) {return;};
	void artifactAssembled (const ArtifactLocation &artLoc) {return;};

	boost::function<void()> dsm; //dismiss button callback
	boost::function<void()> Upg; //upgrade button callback

	CCreatureWindow(const CStack & stack, int type); //battle c-tor
	CCreatureWindow (const CStackInstance &stack, int Type); //pop-up c-tor
	CCreatureWindow(const CStackInstance &st, int Type, boost::function<void()> Upg, boost::function<void()> Dsm, UpgradeInfo *ui); //full garrison window
	CCreatureWindow(const CCommanderInstance * commander); //commander window
	CCreatureWindow(int Cid, int Type, int creatureCount); //c-tor

	void init(const CStackInstance *stack, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner);
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void printLine(int nr, const std::string &text, int baseVal, int val=-1, bool range=false);
	void close();
	void clickRight(tribool down, bool previousState); //call-in
	void sliderMoved(int newpos);
	~CCreatureWindow(); //d-tor

	void recreateSkillList(int pos);
	void scrollArt(int dir);
	void passArtifactToHero();
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

/// original creature info window
class CCreInfoWindow : public CIntObject
{
public:
	CPicture * background;
	CLabel * creatureCount;
	CLabel * creatureName;
	CLabel * abilityText;

	CCreaturePic * animation;
	std::vector<CComponent *> upgResCost; //cost of upgrade (if not possible then empty)
	std::vector<CAnimImage *> effects;
	std::map<size_t, std::pair<CLabel *, CLabel * > > infoTexts;

	MoraleLuckBox * luck, * morale;

	CAdventureMapButton * dismiss, * upgrade, * ok;

	CCreInfoWindow(const CStackInstance & st, bool LClicked, boost::function<void()> Upg = 0, boost::function<void()> Dsm = 0, UpgradeInfo * ui = NULL);
	CCreInfoWindow(const CStack & st, bool LClicked = 0);
	CCreInfoWindow(int Cid, bool LClicked, int creatureCount);
	~CCreInfoWindow();

	void init(const CCreature * cre, const CBonusSystemNode * stackNode, const CGHeroInstance * heroOwner, int creatureCount, bool LClicked);
	void printLine(int nr, const std::string & text, int baseVal, int val = -1, bool range = false);

	void clickRight(tribool down, bool previousState);
	void close();
	void show(SDL_Surface * to);
};

CIntObject *createCreWindow(const CStack *s, bool lclick = false);
CIntObject *createCreWindow(int Cid, int Type, int creatureCount);
CIntObject *createCreWindow(const CStackInstance *s, int type, boost::function<void()> Upg = 0, boost::function<void()> Dsm = 0, UpgradeInfo *ui = NULL);
