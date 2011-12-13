#pragma once

#include "CAttackAnimation.h"
#include "../../lib/SHexField.h"

class CBattleInterface;
class CStack;
struct SCatapultSProjectileInfo;

/*
 * CShootingAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Small struct which contains information about the position and the velocity of a projectile
struct SProjectileInfo
{
	double x, y; //position on the screen
	double dx, dy; //change in position in one step
	int step, lastStep; //to know when finish showing this projectile
	int creID; //ID of creature that shot this projectile
	int stackID; //ID of stack
	int frameNum; //frame to display form projectile animation
	bool spin; //if true, frameNum will be increased
	int animStartDelay; //how many times projectile must be attempted to be shown till it's really show (decremented after hit)
	bool reverse; //if true, projectile will be flipped by vertical asix
	SCatapultSProjectileInfo *catapultInfo; // holds info about the parabolic trajectory of the cannon
};

/// Shooting attack
class CShootingAnimation : public CAttackAnimation
{
private:
	int catapultDamage;
	bool catapult;
public:
	bool init();
	void nextFrame();
	void endAnim();

	//last param only for catapult attacks
	CShootingAnimation(CBattleInterface *_owner, const CStack *attacker, 
		SHexField _dest, const CStack *_attacked, bool _catapult = false, int _catapultDmg = 0);
};