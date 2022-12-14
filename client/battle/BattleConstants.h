/*
 * BattleConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

enum class EAnimationEvents {
	OPENING     = 0, // TODO battle opening sound is playing
	ACTION      = 1, // there are any ongoing animations
	BEFORE_MOVE = 2, // TODO effects played before stack can act, e.g. regen or bad morale
	MOVEMENT    = 3, // stacks are moving or turning around
	BEFORE_HIT  = 4, // effects played before all attack/defence/hit animations
	ATTACK      = 5, // attack and defence animations are playing
	HIT         = 6, // hit & death animations are playing
	AFTER_HIT   = 7, // after all hit & death animations are over
	PROJECTILES = 8, // TODO there are any flying projectiles
	COUNT
};

namespace EBattleEffect
{
	enum EBattleEffect
	{
		// list of battle effects that have hardcoded triggers
		FEAR         = 15,
		GOOD_LUCK    = 18,
		GOOD_MORALE  = 20,
		BAD_MORALE   = 30,
		BAD_LUCK     = 48,
		RESURRECT    = 50,
		DRAIN_LIFE   = 52,
		POISON       = 67,
		DEATH_BLOW   = 73,
		REGENERATION = 74,
		MANA_DRAIN   = 77,

		INVALID      = -1,
	};
}

enum class EHeroAnimType
{
	HOLDING    = 0,
	IDLE       = 1, // idling movement that happens from time to time
	DEFEAT     = 2, // played when army loses stack or on friendly fire
	VICTORY    = 3, // when enemy stack killed or huge damage is dealt
	CAST_SPELL = 4  // spellcasting
};

namespace ECreatureAnimType
{
enum Type // list of creature animations, numbers were taken from def files
{
	MOVING          = 0,
	MOUSEON         = 1,
	HOLDING         = 2,  // base idling animation
	HITTED          = 3,  // base animation for when stack is taking damage
	DEFENCE         = 4,  // alternative animation for defending in melee if stack spent its action on defending
	DEATH           = 5,
	DEATH_RANGED    = 6,  // Optional, alternative animation for when stack is killed by ranged attack
	TURN_L          = 7,
	TURN_R          = 8,
	//TURN_L2       = 9,  //unused - identical to TURN_L
	//TURN_R2       = 10, //unused - identical to TURN_R
	ATTACK_UP       = 11,
	ATTACK_FRONT    = 12,
	ATTACK_DOWN     = 13,
	SHOOT_UP        = 14, // Shooters only
	SHOOT_FRONT     = 15, // Shooters only
	SHOOT_DOWN      = 16, // Shooters only
	SPECIAL_UP      = 17, // If empty, fallback to SPECIAL_FRONT
	SPECIAL_FRONT   = 18, // Used for any special moves - dragon breath, spellcasting, (possibly - Pit Lord/Ogre Mage ability)
	SPECIAL_DOWN    = 19, // If empty, fallback to SPECIAL_FRONT
	MOVE_START      = 20, // small animation to be played before MOVING
	MOVE_END        = 21, // small animation to be played after MOVING

	DEAD            = 22, // new group, used to show dead stacks. If empty - last frame from "DEATH" will be copied here
	DEAD_RANGED     = 23, // new group, used to show dead stacks (if DEATH_RANGED was used). If empty - last frame from "DEATH_RANGED" will be copied here
	RESURRECTION    = 24, // new group, used for animating resurrection, if empty - reversed "DEATH" animation will be copiend here

	CAST_UP            = 30,
	CAST_FRONT         = 31,
	CAST_DOWN          = 32,

	GROUP_ATTACK_UP    = 40,
	GROUP_ATTACK_FRONT = 41,
	GROUP_ATTACK_DOWN  = 42
};
}
