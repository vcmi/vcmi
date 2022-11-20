/*
 * CBattleStacksController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct BattleObjectsByHex;
struct SDL_Surface;
struct BattleHex;
struct StackAttackedInfo;
struct BattleAction;

class SpellID;
class CBattleInterface;
class CBattleAnimation;
class CCreatureAnimation;
class CStack;
class CBattleAnimation;

class CBattleStacksController
{
	CBattleInterface * owner;

	SDL_Surface *amountNormal;
	SDL_Surface *amountNegative;
	SDL_Surface *amountPositive;
	SDL_Surface *amountEffNeutral;

	std::list<std::pair<CBattleAnimation *, bool>> pendingAnims; //currently displayed animations <anim, initialized>
	std::map<int32_t, std::shared_ptr<CCreatureAnimation>> creAnims; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map<int, bool> creDir; // <creatureID, if false reverse creature's animation> //TODO: move it to battle callback

	const CStack *activeStack; //number of active stack; nullptr - no one
	const CStack *mouseHoveredStack; // stack below mouse pointer, used for border animation
	const CStack *stackToActivate; //when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack *selectedStack; //for Teleport / Sacrifice

	bool stackCanCastSpell; //if true, active stack could possibly cast some target spell
	si32 creatureSpellToCast;

	ui32 animIDhelper; //for giving IDs for animations
public:
	CBattleStacksController(CBattleInterface * owner);
	~CBattleStacksController();

	void sortObjectsByHex(BattleObjectsByHex & sorted);
	bool shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex);
	bool facingRight(const CStack * stack);

	void stackReset(const CStack * stack);
	void stackAdded(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting); //called when stack with id ID is attacking something on hex dest

	void startAction(const BattleAction* action);
	void endAction(const BattleAction* action);

	bool activeStackSpellcaster();
	SpellID activeStackSpellToCast();

	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all

	void setActiveStack(const CStack *stack);
	void setHoveredStack(const CStack *stack);
	void setSelectedStack(const CStack *stack);

	void showAliveStacks(SDL_Surface *to, std::vector<const CStack *> stacks);
	void showStacks(SDL_Surface *to, std::vector<const CStack *> stacks);

	void addNewAnim(CBattleAnimation *anim); //adds new anim to pendingAnims
	void updateBattleAnimations();

	const CStack* getActiveStack();
	const CStack* getSelectedStack();

	friend class CBattleAnimation; // for exposing pendingAnims/creAnims/creDir to animations
};
