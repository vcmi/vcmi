#include "StdInc.h"
#include <boost/math/constants/constants.hpp>
#include "CShootingAnimation.h"

#include "../../lib/BattleState.h"
#include "CBattleInterface.h"
#include "CCreatureAnimation.h"
#include "../CGameInfo.h"
#include "../../lib/CTownHandler.h"
#include "CMovementStartAnimation.h"
#include "CReverseAnimation.h"
#include "CSpellEffectAnimation.h"
#include "CClickableHex.h"

bool CShootingAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	const CStack * shooter = attackingStack;

	if(!shooter || myAnim()->getType() == 5)
	{
		endAnim();
		return false;
	}

	// Create the projectile animation

	double projectileAngle; //in radians; if positive, projectiles goes up
	double straightAngle = 0.2; //maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	int fromHex = shooter->position;
	projectileAngle = atan2(static_cast<double>(abs(dest - fromHex) / GameConstants::BFIELD_WIDTH), static_cast<double>(abs(dest - fromHex) % GameConstants::BFIELD_WIDTH));
	if(fromHex < dest)
		projectileAngle = -projectileAngle;

	// Get further info about the shooter e.g. relative pos of projectile to unit.
	// If the creature id is 149 then it's a arrow tower which has no additional info so get the
	// actual arrow tower shooter instead.
	const CCreature *shooterInfo = shooter->getCreature();
	if (shooterInfo->idNumber == 149)
	{
		int creID = CGI->creh->factionToTurretCreature[owner->siegeH->town->town->typeID];
		shooterInfo = CGI->creh->creatures[creID];
	}

	SProjectileInfo spi;
	spi.creID = shooter->getCreature()->idNumber;
	spi.stackID = shooter->ID;
	spi.reverse = !shooter->attackerOwned;

	spi.step = 0;
	spi.frameNum = 0;
	if(vstd::contains(CGI->creh->idToProjectileSpin, shooterInfo->idNumber))
		spi.spin = CGI->creh->idToProjectileSpin[shooterInfo->idNumber];
	else
	{
		tlog2 << "Warning - no projectile spin for spi.creID " << shooterInfo->idNumber << std::endl;
		spi.spin = false;
	}

	SPoint xycoord = CClickableHex::getXYUnitAnim(shooter->position, true, shooter, owner);
	SPoint destcoord;


	// The "master" point where all projectile positions relate to.
	static const SPoint projectileOrigin(181, 252);

	if (attackedStack)
	{
		destcoord = CClickableHex::getXYUnitAnim(dest, false, attackedStack, owner); 
		destcoord.x += 250; destcoord.y += 210; //TODO: find a better place to shoot

		// Calculate projectile start position. Offsets are read out of the CRANIM.TXT.
		if (projectileAngle > straightAngle)
		{
			//upper shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->upperRightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->upperRightMissleOffsetY;
		}
		else if (projectileAngle < -straightAngle) 
		{
			//lower shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->lowerRightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->lowerRightMissleOffsetY;
		}
		else 
		{
			//straight shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->rightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->rightMissleOffsetY;
		}

		double animSpeed = 23.0 * owner->getAnimSpeed(); // flight speed of projectile
		spi.lastStep = static_cast<int>(sqrt(static_cast<double>((destcoord.x - spi.x) * (destcoord.x - spi.x) + (destcoord.y - spi.y) * (destcoord.y - spi.y))) / animSpeed);
		if(spi.lastStep == 0)
			spi.lastStep = 1;
		spi.dx = (destcoord.x - spi.x) / spi.lastStep;
		spi.dy = (destcoord.y - spi.y) / spi.lastStep;
		spi.catapultInfo = 0;
	}
	else 
	{
		// Catapult attack
		// These are the values for equations of this kind: f(x) = ax^2 + bx + c
		static const std::vector<SCatapultSProjectileInfo*> trajectoryCurves = boost::assign::list_of<SCatapultSProjectileInfo*>(new SCatapultSProjectileInfo(4.309, -3.198, 569.2, -296, 182))
			(new SCatapultSProjectileInfo(4.710, -3.11, 558.68, -258, 175))(new SCatapultSProjectileInfo(5.056, -3.003, 546.9, -236, 174))
			(new SCatapultSProjectileInfo(4.760, -2.74, 526.47, -216, 215))(new SCatapultSProjectileInfo(4.288, -2.496, 508.98, -223, 274))
			(new SCatapultSProjectileInfo(3.683, -3.018, 558.39, -324, 176))(new SCatapultSProjectileInfo(2.884, -2.607, 528.95, -366, 312))
			(new SCatapultSProjectileInfo(3.783, -2.364, 501.35, -227, 318));

		static std::map<int, int> hexToCurve = boost::assign::map_list_of<int, int>(29, 0)(62, 1)(95, 2)(130, 3)(182, 4)(12, 5)(50, 6)(183, 7);

		std::map<int, int>::iterator it = hexToCurve.find(dest.hex);

		if (it == hexToCurve.end())
		{
			tlog1 << "For the hex position " << dest.hex << " is no curve defined.";
			endAnim();
			return false;
		}
		else
		{
			int curveID = it->second;
			spi.catapultInfo = trajectoryCurves[curveID];
			double animSpeed = 3.318 * owner->getAnimSpeed();
			spi.lastStep = static_cast<int>((spi.catapultInfo->toX - spi.catapultInfo->fromX) / animSpeed);
			spi.dx = animSpeed;
			spi.dy = 0;
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->rightMissleOffsetX + 17.;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->rightMissleOffsetY + 10.;

			// Add explosion anim
			int xEnd = static_cast<int>(spi.x + spi.lastStep * spi.dx);
			int yEnd = static_cast<int>(spi.catapultInfo->calculateY(xEnd));
			owner->addNewAnim( new CSpellEffectAnimation(owner, "SGEXPL.DEF", xEnd - 126, yEnd - 105));
		}
	}

	// Set starting frame
	if(spi.spin)
	{
		spi.frameNum = 0;
	}
	else
	{
		double pi = boost::math::constants::pi<double>();
		spi.frameNum = static_cast<int>(((pi / 2.0 - projectileAngle) / (2.0 * pi) + 1/(static_cast<double>(2*(owner->idToProjectile[spi.creID]->ourImages.size()-1)))) * (owner->idToProjectile[spi.creID]->ourImages.size()-1));
	}

	// Set projectile animation start delay which is specified in frames
	spi.animStartDelay = shooterInfo->attackClimaxFrame;
	owner->projectiles.push_back(spi);

	//attack animation

	shooting = true;

	if(projectileAngle > straightAngle) //upper shot
		group = CCreatureAnim::SHOOT_UP;
	else if(projectileAngle < -straightAngle) //lower shot
		group = CCreatureAnim::SHOOT_DOWN;
	else //straight shot
		group = CCreatureAnim::SHOOT_FRONT;

	return true;
}

void CShootingAnimation::nextFrame()
{
	for(std::list<std::pair<CBattleAnimation *, bool> >::const_iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CMovementStartAnimation * anim = dynamic_cast<CMovementStartAnimation *>(it->first);
		CReverseAnimation * anim2 = dynamic_cast<CReverseAnimation *>(it->first);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
			return;
	}

	CAttackAnimation::nextFrame();
}

void CShootingAnimation::endAnim()
{
	CBattleAnimation::endAnim();
	delete this;
}

CShootingAnimation::CShootingAnimation(CBattleInterface * _owner, const CStack * attacker, SBattleHex _dest, const CStack * _attacked, bool _catapult, int _catapultDmg)
	: CAttackAnimation(_owner, attacker, _dest, _attacked), catapultDamage(_catapultDmg), catapult(_catapult)
{

}