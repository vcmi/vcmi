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

#include "../render/ColorFilter.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleHex;
class BattleAction;
class CStack;
class CSpell;
class SpellID;
class Point;

VCMI_LIB_NAMESPACE_END

struct StackAttackedInfo;
struct StackAttackInfo;

class ColorFilter;
class Canvas;
class BattleInterface;
class BattleAnimation;
class CreatureAnimation;
class BattleAnimation;
class BattleRenderer;
class IImage;

struct BattleStackFilterEffect
{
	ColorFilter effect;
	const CStack * target;
	const CSpell * source;
	bool persistent;
};

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
	std::vector<BattleAnimation *> currentAnimations;

	/// currently active color effects on stacks, in order of their addition (which corresponds to their apply order)
	std::vector<BattleStackFilterEffect> stackFilterEffects;

	/// animations of creatures from fighting armies (order by BattleInfo's stacks' ID)
	std::map<int32_t, std::shared_ptr<CreatureAnimation>> stackAnimation;

	/// <creatureID, if false reverse creature's animation> //TODO: move it to battle callback
	std::map<int, bool> stackFacingRight;

	/// currently active stack; nullptr - no one
	const CStack *activeStack;

	/// stacks or their battle queue images below mouse pointer (multiple stacks possible while spellcasting), used for border animation
	std::vector<const CStack *> mouseHoveredStacks;

	///when animation is playing, we should wait till the end to make the next stack active; nullptr of none
	const CStack *stackToActivate;

	/// stack that was selected for multi-target spells - Teleport / Sacrifice
	const CStack *selectedStack;

	/// for giving IDs for animations
	ui32 animIDhelper;

	bool stackNeedsAmountBox(const CStack * stack) const;
	void showStackAmountBox(Canvas & canvas, const CStack * stack);
	BattleHex getStackCurrentPosition(const CStack * stack) const;

	std::shared_ptr<IImage> getStackAmountBox(const CStack * stack);

	void removeExpiredColorFilters();

	void initializeBattleAnimations();
	void stepFrameBattleAnimations();

	void updateBattleAnimations();
	void updateHoveredStacks();

	std::vector<const CStack *> selectHoveredStacks();

	bool shouldAttackFacingRight(const CStack * attacker, const CStack * defender);

public:
	BattleStacksController(BattleInterface & owner);

	bool shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex) const;
	bool facingRight(const CStack * stack) const;

	void stackReset(const CStack * stack);
	void stackAdded(const CStack * stack, bool instant); //new stack appeared on battlefield
	void stackRemoved(uint32_t stackID); //stack disappeared from batlefiled
	void stackActivated(const CStack *stack); //active stack has been changed
	void stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void stackTeleported(const CStack *stack, std::vector<BattleHex> destHex, int distance); //stack with id number moved to destHex
	void stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos); //called when a certain amount of stacks has been attacked
	void stackAttacking(const StackAttackInfo & info); //called when stack with id ID is attacking something on hex dest

	void startAction(const BattleAction* action);
	void endAction(const BattleAction* action);

	void deactivateStack(); //copy activeStack to stackToActivate, then set activeStack to nullptr to temporary disable current stack

	void activateStack(); //copy stackToActivate to activeStack to enable controls of the stack

	void setActiveStack(const CStack *stack);
	void setSelectedStack(const CStack *stack);

	void showAliveStack(Canvas & canvas, const CStack * stack);
	void showStack(Canvas & canvas, const CStack * stack);

	void collectRenderableObjects(BattleRenderer & renderer);

	/// Adds new color filter effect targeting stack
	/// Effect will last as long as stack is affected by specified spell (unless effect is persistent)
	/// If effect from same (target, source) already exists, it will be updated
	void setStackColorFilter(const ColorFilter & effect, const CStack * target, const CSpell *source, bool persistent);
	void addNewAnim(BattleAnimation *anim); //adds new anim to pendingAnims

	const CStack* getActiveStack() const;
	const CStack* getSelectedStack() const;
	const std::vector<uint32_t> getHoveredStacksUnitIds() const;

	void update();

	/// returns position of animation needed to place stack in specific hex
	Point getStackPositionAtHex(BattleHex hexNum, const CStack * creature) const;

	friend class BattleAnimation; // for exposing pendingAnims/creAnims/creDir to animations
};
