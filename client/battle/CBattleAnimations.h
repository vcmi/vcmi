/*
 * CBattleAnimations.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"
#include "../widgets/Images.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;

VCMI_LIB_NAMESPACE_END

class CBattleInterface;
class CCreatureAnimation;
struct CatapultProjectileInfo;
struct StackAttackedInfo;

/// Base class of battle animations
class CBattleAnimation
{
protected:
	CBattleInterface * owner;
public:
	virtual bool init() = 0; //to be called - if returned false, call again until returns true
	virtual void nextFrame() {} //call every new frame
	virtual void endAnim(); //to be called mostly internally; in this class it removes animation from pendingAnims list
	virtual ~CBattleAnimation();

	bool isEarliest(bool perStackConcurrency); //determines if this animation is earliest of all

	ui32 ID; //unique identifier
	CBattleAnimation(CBattleInterface * _owner);
};

/// Sub-class which is responsible for managing the battle stack animation.
class CBattleStackAnimation : public CBattleAnimation
{
public:
	std::shared_ptr<CCreatureAnimation> myAnim; //animation for our stack, managed by CBattleInterface
	const CStack * stack; //id of stack whose animation it is

	CBattleStackAnimation(CBattleInterface * _owner, const CStack * _stack);

	void shiftColor(const ColorShifter * shifter);
};

/// This class is responsible for managing the battle attack animation
class CAttackAnimation : public CBattleStackAnimation
{
	bool soundPlayed;

protected:
	BattleHex dest; //attacked hex
	bool shooting;
	CCreatureAnim::EAnimType group; //if shooting is true, print this animation group
	const CStack *attackedStack;
	const CStack *attackingStack;
	int attackingStackPosBeforeReturn; //for stacks with return_after_strike feature
public:
	void nextFrame() override;
	void endAnim() override;
	bool checkInitialConditions();

	CAttackAnimation(CBattleInterface *_owner, const CStack *attacker, BattleHex _dest, const CStack *defender);
};

/// Animation of a defending unit
class CDefenceAnimation : public CBattleStackAnimation
{
	CCreatureAnim::EAnimType getMyAnimType();
	std::string getMySound();

	void startAnimation();

	const CStack * attacker; //attacking stack
	bool rangedAttack; //if true, stack has been attacked by shooting
	bool killed; //if true, stack has been killed

	float timeToWait; // for how long this animation should be paused
public:
	bool init() override;
	void nextFrame() override;
	void endAnim() override;

	CDefenceAnimation(StackAttackedInfo _attackedInfo, CBattleInterface * _owner);
	virtual ~CDefenceAnimation(){};
};

class CDummyAnimation : public CBattleAnimation
{
private:
	int counter;
	int howMany;
public:
	bool init() override;
	void nextFrame() override;
	void endAnim() override;

	CDummyAnimation(CBattleInterface * _owner, int howManyFrames);
	virtual ~CDummyAnimation(){}
};

/// Hand-to-hand attack
class CMeleeAttackAnimation : public CAttackAnimation
{
public:
	bool init() override;
	void endAnim() override;

	CMeleeAttackAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked);
	virtual ~CMeleeAttackAnimation(){};
};

/// Move animation of a creature
class CMovementAnimation : public CBattleStackAnimation
{
private:
	std::vector<BattleHex> destTiles; //full path, includes already passed hexes
	ui32 curentMoveIndex; // index of nextHex in destTiles

	BattleHex oldPos; //position of stack before move

	double begX, begY; // starting position
	double distanceX, distanceY; // full movement distance, may be negative if creture moves topleft

	double timeToMove; // full length of movement animation
	double progress; // range 0 -> 1, indicates move progrees. 0 = movement starts, 1 = move ends

public:
	BattleHex nextHex; // next hex, to which creature move right now

	bool init() override;
	void nextFrame() override;
	void endAnim() override;

	CMovementAnimation(CBattleInterface *_owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance);
	virtual ~CMovementAnimation(){};
};

/// Move end animation of a creature
class CMovementEndAnimation : public CBattleStackAnimation
{
private:
	BattleHex destinationTile;
public:
	bool init() override;
	void endAnim() override;

	CMovementEndAnimation(CBattleInterface * _owner, const CStack * _stack, BattleHex destTile);
	virtual ~CMovementEndAnimation(){};
};

/// Move start animation of a creature
class CMovementStartAnimation : public CBattleStackAnimation
{
public:
	bool init() override;
	void endAnim() override;

	CMovementStartAnimation(CBattleInterface * _owner, const CStack * _stack);
	virtual ~CMovementStartAnimation(){};
};

/// Class responsible for animation of stack chaning direction (left <-> right)
class CReverseAnimation : public CBattleStackAnimation
{
	BattleHex hex;
public:
	bool priority; //true - high, false - low
	bool init() override;

	static void rotateStack(CBattleInterface * owner, const CStack * stack, BattleHex hex);

	void setupSecondPart();
	void endAnim() override;

	CReverseAnimation(CBattleInterface * _owner, const CStack * stack, BattleHex dest, bool _priority);
	virtual ~CReverseAnimation(){};
};

class CRangedAttackAnimation : public CAttackAnimation
{
public:
	CRangedAttackAnimation(CBattleInterface * owner_, const CStack * attacker, BattleHex dest_, const CStack * defender);
protected:

};

/// Shooting attack
class CShootingAnimation : public CRangedAttackAnimation
{
private:
	int catapultDamage;
public:
	bool init() override;
	void nextFrame() override;
	void endAnim() override;

	//last two params only for catapult attacks
	CShootingAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest,
		const CStack * _attacked, bool _catapult = false, int _catapultDmg = 0);
	virtual ~CShootingAnimation(){};
};

class CCastAnimation : public CRangedAttackAnimation
{
public:
	CCastAnimation(CBattleInterface * owner_, const CStack * attacker, BattleHex dest_, const CStack * defender);

	bool init() override;
	void nextFrame() override;
	void endAnim() override;

};


/// This class manages effect animation
class CEffectAnimation : public CBattleAnimation
{
private:
	BattleHex destTile;
	std::shared_ptr<CAnimation>	customAnim;
	int	x, y, dx, dy;
	bool Vflip;
	bool alignToBottom;
public:
	bool init() override;
	void nextFrame() override;
	void endAnim() override;

	CEffectAnimation(CBattleInterface * _owner, std::string _customAnim, int _x, int _y, int _dx = 0, int _dy = 0, bool _Vflip = false, bool _alignToBottom = false);

	CEffectAnimation(CBattleInterface * _owner, std::shared_ptr<CAnimation> _customAnim, int _x, int _y, int _dx = 0, int _dy = 0);

	CEffectAnimation(CBattleInterface * _owner, std::string _customAnim, BattleHex _destTile, bool _Vflip = false, bool _alignToBottom = false);
	virtual ~CEffectAnimation(){};
};
