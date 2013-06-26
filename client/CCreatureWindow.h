#pragma once

#include "gui/CIntObject.h"
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
struct ArtifactLocation;
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
class CSelectableSkill;

// New creature window
class CCreatureWindow : public CWindowObject, public CArtifactHolder
{
public:
	enum CreWinType {OTHER = 0, BATTLE = 1, ARMY = 2, HERO = 3, COMMANDER = 4, COMMANDER_LEVEL_UP = 5, COMMANDER_BATTLE = 6}; // > 3 are opened permanently
	//bool active; //TODO: comment me
	CreWinType type;
	int bonusRows; //height of skill window
	ArtifactPosition displayedArtifact;

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

	CCreaturePic *anim; //related creature's animation
	MoraleLuckBox *luck, *morale;
	LRClickableAreaWTextComp * expArea; //displays exp details
	CSlider * slider; //Abilities
	CAdventureMapButton *dismiss, *upgrade, *ok;
	CAdventureMapButton * leftArtRoll, * rightArtRoll; //artifact selection
	CAdventureMapButton * passArtToHero;
	CAnimImage *artifactImage;

	//commander level-up
	int selectedOption; //index for upgradeOptions
	std::vector<ui32> upgradeOptions; //value 0-5 - secondary skills, 100+ - special skills
	std::vector<CSelectableSkill *> selectableSkills, selectableBonuses;
	std::vector<CPicture *> skillPictures; //secondary skills

	std::string skillToFile(int skill); //return bitmap for secondary skill depending on selection / avaliability
	void selectSkill (ui32 which);
	void setArt(const CArtifactInstance *creatureArtifact);

	void artifactRemoved (const ArtifactLocation &artLoc);
	void artifactMoved (const ArtifactLocation &artLoc, const ArtifactLocation &destLoc);
	void artifactDisassembled (const ArtifactLocation &artLoc) {return;};
	void artifactAssembled (const ArtifactLocation &artLoc) {return;};

	std::function<void()> dsm; //dismiss button callback
	std::function<void()> Upg; //upgrade button callback
	std::function<void(ui32)> levelUp; //choose commander skill to level up

	CCreatureWindow(const CStack & stack, CreWinType type); //battle c-tor
	CCreatureWindow (const CStackInstance &stack, CreWinType Type); //pop-up c-tor
	CCreatureWindow(const CStackInstance &st, CreWinType Type, std::function<void()> Upg, std::function<void()> Dsm, UpgradeInfo *ui); //full garrison window
	CCreatureWindow(const CCommanderInstance * commander, const CStack * stack = nullptr); //commander window
	CCreatureWindow(std::vector<ui32> &skills, const CCommanderInstance * commander, std::function<void(ui32)> callback); 
	CCreatureWindow(CreatureID Cid, CreWinType Type, int creatureCount); //c-tor

	void init(const CStackInstance *stack, const CBonusSystemNode *stackNode, const CGHeroInstance *heroOwner);
	void showAll(SDL_Surface * to);
	void show(SDL_Surface * to);
	void printLine(int nr, const std::string &text, int baseVal, int val=-1, bool range=false);
	void sliderMoved(int newpos);
	void close();
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

	void showAll (SDL_Surface * to);
};

class CSelectableSkill : public LRClickableAreaWText
{
public:
	std::function<void()> callback; //TODO: create more generic clickable class than AdvMapButton?

	virtual void clickLeft(tribool down, bool previousState);
	virtual void clickRight(tribool down, bool previousState){};
};

/// original creature info window
class CCreInfoWindow : public CWindowObject
{
public:
	CLabel * creatureCount;
	CLabel * creatureName;
	CLabel * abilityText;

	CCreaturePic * animation;
	std::vector<CComponent *> upgResCost; //cost of upgrade (if not possible then empty)
	std::vector<CAnimImage *> effects;
	std::map<size_t, std::pair<CLabel *, CLabel * > > infoTexts;

	MoraleLuckBox * luck, * morale;

	CAdventureMapButton * dismiss, * upgrade, * ok;

	CCreInfoWindow(const CStackInstance & st, bool LClicked, std::function<void()> Upg = 0, std::function<void()> Dsm = 0, UpgradeInfo * ui = nullptr);
	CCreInfoWindow(const CStack & st, bool LClicked = 0);
	CCreInfoWindow(int Cid, bool LClicked, int creatureCount);
	~CCreInfoWindow();

	void init(const CCreature * cre, const CBonusSystemNode * stackNode, const CGHeroInstance * heroOwner, int creatureCount, bool LClicked);
	void printLine(int nr, const std::string & text, int baseVal, int val = -1, bool range = false);

	void show(SDL_Surface * to);
};

CIntObject *createCreWindow(const CStack *s, bool lclick = false);
CIntObject *createCreWindow(CreatureID Cid, CCreatureWindow::CreWinType Type, int creatureCount);
CIntObject *createCreWindow(const CStackInstance *s, CCreatureWindow::CreWinType type, std::function<void()> Upg = 0, std::function<void()> Dsm = 0, UpgradeInfo *ui = nullptr);
