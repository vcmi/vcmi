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
		DRAIN_LIFE   = 52, // hardcoded constant in CGameHandler
		POISON       = 67,
		DEATH_BLOW   = 73,
		REGENERATION = 74,
		MANA_DRAIN   = 77,

		INVALID      = -1,
	};
}

namespace EHeroAnimType
{
enum Type
{
	HOLDING    = 0,
	IDLE       = 1, // idling movement that happens from time to time
	DEFEAT     = 2, // played when army loses stack or on friendly fire
	VICTORY    = 3, // when enemy stack killed or huge damage is dealt
	CAST_SPELL = 4 // spellcasting
};
}

namespace ECreatureAnimType
{
enum Type // list of creature animations, numbers were taken from def files
{
	MOVING          =0,
	MOUSEON         =1,
	HOLDING         =2,
	HITTED          =3,
	DEFENCE         =4,
	DEATH           =5,
	DEATH_RANGED    =6,
	TURN_L          =7,
	TURN_R          =8,
	//TURN_L2       =9, //unused - identical to TURN_L
	//TURN_R2       =10,//unused - identical to TURN_R
	ATTACK_UP       =11,
	ATTACK_FRONT    =12,
	ATTACK_DOWN     =13,
	SHOOT_UP        =14,
	SHOOT_FRONT     =15,
	SHOOT_DOWN      =16,
	CAST_UP         =17,
	CAST_FRONT      =18,
	CAST_DOWN       =19,
	MOVE_START      =20,
	MOVE_END        =21,

	DEAD            = 22, // new group, used to show dead stacks. If empty - last frame from "DEATH" will be copied here
	DEAD_RANGED     = 23, // new group, used to show dead stacks (if DEATH_RANGED was used). If empty - last frame from "DEATH_RANGED" will be copied here
	RESURRECTION    = 24, // new group, used for animating resurrection, if empty - reversed "DEATH" animation will be copiend here

	VCMI_CAST_UP    = 30,
	VCMI_CAST_FRONT = 31,
	VCMI_CAST_DOWN  = 32,
	VCMI_2HEX_UP    = 40,
	VCMI_2HEX_FRONT = 41,
	VCMI_2HEX_DOWN  = 42
};
}
