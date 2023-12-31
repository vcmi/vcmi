/*
 * BattleAnimations.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "BattleConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class CCreature;
class CSpell;
class Point;

VCMI_LIB_NAMESPACE_END

class ColorFilter;
class BattleHero;
class CAnimation;
class BattleInterface;
class CreatureAnimation;
struct StackAttackedInfo;

/// Base class of battle animations
class BattleAnimation
{
protected:
	BattleInterface & owner;
	bool initialized;

	std::vector<BattleAnimation *> & pendingAnimations();
	std::shared_ptr<CreatureAnimation> stackAnimation(const CStack * stack) const;
	bool stackFacingRight(const CStack * stack);
	void setStackFacingRight(const CStack * stack, bool facingRight);

	virtual bool init() = 0; //to be called - if returned false, call again until returns true

public:
	ui32 ID; //unique identifier

	bool isInitialized();
	bool tryInitialize();
	virtual void tick(uint32_t msPassed) {} //call every new frame
	virtual ~BattleAnimation();

	BattleAnimation(BattleInterface & owner);
};

/// Sub-class which is responsible for managing the battle stack animation.
class BattleStackAnimation : public BattleAnimation
{
public:
	std::shared_ptr<CreatureAnimation> myAnim; //animation for our stack, managed by BattleInterface
	const CStack * stack; //id of stack whose animation it is

	BattleStackAnimation(BattleInterface & owner, const CStack * _stack);
	void rotateStack(BattleHex hex);
};

class StackActionAnimation : public BattleStackAnimation
{
	ECreatureAnimType nextGroup;
	ECreatureAnimType currGroup;
	AudioPath sound;
public:
	void setNextGroup( ECreatureAnimType group );
	void setGroup( ECreatureAnimType group );
	void setSound( const AudioPath & sound );

	ECreatureAnimType getGroup() const;

	StackActionAnimation(BattleInterface & owner, const CStack * _stack);
	~StackActionAnimation();

	bool init() override;
};

/// Animation of a defending unit
class DefenceAnimation : public StackActionAnimation
{
public:
	DefenceAnimation(BattleInterface & owner, const CStack * stack);
};

/// Animation of a hit unit
class HittedAnimation : public StackActionAnimation
{
public:
	HittedAnimation(BattleInterface & owner, const CStack * stack);
};

/// Animation of a dying unit
class DeathAnimation : public StackActionAnimation
{
public:
	DeathAnimation(BattleInterface & owner, const CStack * stack, bool ranged);
};

/// Resurrects stack from dead state
class ResurrectionAnimation : public StackActionAnimation
{
public:
	ResurrectionAnimation(BattleInterface & owner, const CStack * _stack);
};

class ColorTransformAnimation : public BattleStackAnimation
{
	std::vector<ColorFilter> steps;
	std::vector<float> timePoints;
	const CSpell * spell;

	float totalProgress;

	bool init() override;
	void tick(uint32_t msPassed) override;

public:
	ColorTransformAnimation(BattleInterface & owner, const CStack * _stack, const std::string & colorFilterName, const CSpell * spell);
};

/// Base class for all animations that play during stack movement
class StackMoveAnimation : public BattleStackAnimation
{
public:
	BattleHex nextHex;
	BattleHex prevHex;

protected:
	StackMoveAnimation(BattleInterface & owner, const CStack * _stack, BattleHex prevHex, BattleHex nextHex);
};

/// Move animation of a creature
class MovementAnimation : public StackMoveAnimation
{
private:
	int moveSoundHander; // sound handler used when moving a unit

	std::vector<BattleHex> destTiles; //full path, includes already passed hexes
	ui32 curentMoveIndex; // index of nextHex in destTiles

	double begX, begY; // starting position
	double distanceX, distanceY; // full movement distance, may be negative if creture moves topleft

	/// progress gain per second
	double progressPerSecond;

	/// range 0 -> 1, indicates move progrees. 0 = movement starts, 1 = move ends
	double progress;

public:
	bool init() override;
	void tick(uint32_t msPassed) override;

	MovementAnimation(BattleInterface & owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance);
	~MovementAnimation();
};

/// Move end animation of a creature
class MovementEndAnimation : public StackMoveAnimation
{
public:
	bool init() override;

	MovementEndAnimation(BattleInterface & owner, const CStack * _stack, BattleHex destTile);
	~MovementEndAnimation();
};

/// Move start animation of a creature
class MovementStartAnimation : public StackMoveAnimation
{
public:
	bool init() override;

	MovementStartAnimation(BattleInterface & owner, const CStack * _stack);
};

/// Class responsible for animation of stack chaning direction (left <-> right)
class ReverseAnimation : public StackMoveAnimation
{
	void setupSecondPart();
public:
	bool init() override;

	ReverseAnimation(BattleInterface & owner, const CStack * stack, BattleHex dest);
};

/// This class is responsible for managing the battle attack animation
class AttackAnimation : public StackActionAnimation
{
protected:
	BattleHex dest; //attacked hex
	const CStack *defendingStack;
	const CStack *attackingStack;
	int attackingStackPosBeforeReturn; //for stacks with return_after_strike feature

	const CCreature * getCreature() const;
	ECreatureAnimType findValidGroup( const std::vector<ECreatureAnimType> candidates ) const;

public:
	AttackAnimation(BattleInterface & owner, const CStack *attacker, BattleHex _dest, const CStack *defender);
};

/// Hand-to-hand attack
class MeleeAttackAnimation : public AttackAnimation
{
	ECreatureAnimType getUpwardsGroup(bool multiAttack) const;
	ECreatureAnimType getForwardGroup(bool multiAttack) const;
	ECreatureAnimType getDownwardsGroup(bool multiAttack) const;

	ECreatureAnimType selectGroup(bool multiAttack);

public:
	MeleeAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, bool multiAttack);

	void tick(uint32_t msPassed) override;
};


class RangedAttackAnimation : public AttackAnimation
{
	void setAnimationGroup();
	void initializeProjectile();
	void emitProjectile();
	void emitExplosion();

protected:
	bool projectileEmitted;

	virtual ECreatureAnimType getUpwardsGroup() const = 0;
	virtual ECreatureAnimType getForwardGroup() const = 0;
	virtual ECreatureAnimType getDownwardsGroup() const = 0;

	virtual void createProjectile(const Point & from, const Point & dest) const = 0;
	virtual uint32_t getAttackClimaxFrame() const = 0;

public:
	RangedAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest, const CStack * defender);
	~RangedAttackAnimation();

	bool init() override;
	void tick(uint32_t msPassed) override;
};

/// Shooting attack
class ShootingAnimation : public RangedAttackAnimation
{
	ECreatureAnimType getUpwardsGroup() const override;
	ECreatureAnimType getForwardGroup() const override;
	ECreatureAnimType getDownwardsGroup() const override;

	void createProjectile(const Point & from, const Point & dest) const override;
	uint32_t getAttackClimaxFrame() const override;

public:
	ShootingAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest, const CStack * defender);

};

/// Catapult attack
class CatapultAnimation : public ShootingAnimation
{
private:
	bool explosionEmitted;
	int catapultDamage;

public:
	CatapultAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest, const CStack * defender, int _catapultDmg = 0);

	void createProjectile(const Point & from, const Point & dest) const override;
	void tick(uint32_t msPassed) override;
};

class CastAnimation : public RangedAttackAnimation
{
	const CSpell * spell;

	ECreatureAnimType getUpwardsGroup() const override;
	ECreatureAnimType getForwardGroup() const override;
	ECreatureAnimType getDownwardsGroup() const override;

	void createProjectile(const Point & from, const Point & dest) const override;
	uint32_t getAttackClimaxFrame() const override;

public:
	CastAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest_, const CStack * defender, const CSpell * spell);
};

class DummyAnimation : public BattleAnimation
{
private:
	int counter;
	int howMany;
public:
	bool init() override;
	void tick(uint32_t msPassed) override;

	DummyAnimation(BattleInterface & owner, int howManyFrames);
};

/// Class that plays effect at one or more positions along with (single) sound effect
class EffectAnimation : public BattleAnimation
{
	std::string soundName;
	bool effectFinished;
	bool reversed;
	int effectFlags;

	std::shared_ptr<CAnimation>	animation;
	std::vector<Point> positions;
	std::vector<BattleHex> battlehexes;

	bool alignToBottom() const;
	bool waitForSound() const;
	bool forceOnTop() const;
	bool screenFill() const;

	void onEffectFinished();
	void clearEffect();
	void playEffect(uint32_t msPassed);

public:
	enum EEffectFlags
	{
		ALIGN_TO_BOTTOM = 1,
		FORCE_ON_TOP    = 2,
		SCREEN_FILL     = 4,
	};

	/// Create animation with screen-wide effect
	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, int effects = 0, bool reversed = false);

	/// Create animation positioned at point(s). Note that positions must be are absolute, including battleint position offset
	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, Point pos                 , int effects = 0, bool reversed = false);
	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, std::vector<Point> pos    , int effects = 0, bool reversed = false);

	/// Create animation positioned at certain hex(es)
	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, BattleHex hex             , int effects = 0, bool reversed = false);
	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, std::vector<BattleHex> hex, int effects = 0, bool reversed = false);

	EffectAnimation(BattleInterface & owner, const AnimationPath & animationName, Point pos, BattleHex hex,   int effects = 0, bool reversed = false);
	 ~EffectAnimation();

	bool init() override;
	void tick(uint32_t msPassed) override;
};

class HeroCastAnimation : public BattleAnimation
{
	std::shared_ptr<BattleHero> hero;
	const CStack * target;
	const CSpell * spell;
	BattleHex tile;
	bool projectileEmitted;

	void initializeProjectile();
	void emitProjectile();
	void emitAnimationEvent();

public:
	HeroCastAnimation(BattleInterface & owner, std::shared_ptr<BattleHero> hero, BattleHex dest, const CStack * defender, const CSpell * spell);

	void tick(uint32_t msPassed) override;
	bool init() override;
};
