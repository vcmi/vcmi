/*
 * BattleStacksController.h, part of VCMI engine
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

struct StackAttackedInfo;
struct StackAttackInfo;

class Canvas;
class BattleInterface;
class CBattleAnimation;
class CreatureAnimation;
class CBattleAnimation;
class BattleRenderer;
class IImage;

/// Class responsible for handling stacks in battle
/// Handles ordering of stacks animation
/// As well as rendering of stacks, their amount boxes
/// And any other effect applied to stacks
class BattleStacksController
{
	BattleInterface & owner;

	std::shared_ptr<IImage> amountNormal;
	std::shared_ptr<IImage> amountNegative;
	std::shared_ptr<IImage> amountPositive;
	std::shared_ptr<IImage> amountEffNeutral;

	/// currently displayed animations <anim, initialized>
	std::vector<CBattleAnimation *> currentAnimations;

	/// animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map<int32_t, std::shared_ptr<CreatureAnimation>> stackAnimation;

	/// <creatureID, if false reverse creature's animation> //TODO: move it to battle callback
	std::map<int, bool> stackFacingRight;

	/// number of active stack; nullptr - no one
	const CStack *activeStack;

	/// stack below mouse pointer, used for border animation
	const CStack *mouseHoveredStack;

	///when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack *stackToActivate;

	/// stack that was selected for multi-target spells - Teleport / Sacrifice
	const CStack *selectedStack;

	/// if true, active stack could possibly cast some target spell
	bool stackCanCastSpell;
	si32 creatureSpellToCast;

	/// for giving IDs for animations
	ui32 animIDhelper;

	bool stackNeedsAmountBox(const CStack * stack) const;
	void showStackAmountBox(Canvas & canvas, const CStack * stack);
	BattleHex getStackCurrentPosition(const CStack * stack) const;

	std::shared_ptr<IImage> getStackAmountBox(const CStack * stack);

	void executeAttackAnimations();
public:
	BattleStacksController(BattleInterface & owner);

	bool shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex) const;
	bool facingRight(const CStack * stack) const;

	void stackReset(const CStack * stack);
	void stackAdded(const CStack * stack); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const StackAttackInfo & info); //called when stack with id ID is attacking something on hex dest

	void startAction(const BattleAction* action);
	void endAction(const BattleAction* action);

	bool activeStackSpellcaster();
	SpellID activeStackSpellToCast();

	void activateStack(); //sets activeStack to stackToActivate etc. //FIXME: No, it's not clear at all

	void setActiveStack(const CStack *stack);
	void setHoveredStack(const CStack *stack);
	void setSelectedStack(const CStack *stack);

	void showAliveStack(Canvas & canvas, const CStack * stack);
	void showStack(Canvas & canvas, const CStack * stack);

	void collectRenderableObjects(BattleRenderer & renderer);

	void addNewAnim(CBattleAnimation *anim); //adds new anim to pendingAnims
	void updateBattleAnimations();

	const CStack* getActiveStack() const;
	const CStack* getSelectedStack() const;

	/// returns position of animation needed to place stack in specific hex
	Point getStackPositionAtHex(BattleHex hexNum, const CStack * creature) const;

	friend class CBattleAnimation; // for exposing pendingAnims/creAnims/creDir to animations
};
