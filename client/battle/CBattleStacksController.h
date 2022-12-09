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

#include "../gui/Geometries.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleHex;
class BattleAction;
class CStack;
class SpellID;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct StackAttackedInfo;

class CCanvas;
class BattleInterface;
class CBattleAnimation;
class CreatureAnimation;
class CBattleAnimation;
class BattleRenderer;
class IImage;

class BattleStacksController
{
	BattleInterface * owner;

	std::shared_ptr<IImage> amountNormal;
	std::shared_ptr<IImage> amountNegative;
	std::shared_ptr<IImage> amountPositive;
	std::shared_ptr<IImage> amountEffNeutral;

	std::vector<CBattleAnimation *> currentAnimations; //currently displayed animations <anim, initialized>
	std::map<int32_t, std::shared_ptr<CreatureAnimation>> stackAnimation; //animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map<int, bool> stackFacingRight; // <creatureID, if false reverse creature's animation> //TODO: move it to battle callback

	const CStack *activeStack; //number of active stack; nullptr - no one
	const CStack *mouseHoveredStack; // stack below mouse pointer, used for border animation
	const CStack *stackToActivate; //when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack *selectedStack; //for Teleport / Sacrifice

	bool stackCanCastSpell; //if true, active stack could possibly cast some target spell
	si32 creatureSpellToCast;

	ui32 animIDhelper; //for giving IDs for animations

	bool stackNeedsAmountBox(const CStack * stack);
	void showStackAmountBox(std::shared_ptr<CCanvas> canvas, const CStack * stack);
	BattleHex getStackCurrentPosition(const CStack * stack);

	std::shared_ptr<IImage> getStackAmountBox(const CStack * stack);

public:
	BattleStacksController(BattleInterface * owner);

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

	void showAliveStack(std::shared_ptr<CCanvas> canvas, const CStack * stack);
	void showStack(std::shared_ptr<CCanvas> canvas, const CStack * stack);

	void collectRenderableObjects(BattleRenderer & renderer);

	void addNewAnim(CBattleAnimation *anim); //adds new anim to pendingAnims
	void updateBattleAnimations();

	const CStack* getActiveStack();
	const CStack* getSelectedStack();

	/// returns position of animation needed to place stack in specific hex
	Point getStackPositionAtHex(BattleHex hexNum, const CStack * creature);

	friend class CBattleAnimation; // for exposing pendingAnims/creAnims/creDir to animations
};
