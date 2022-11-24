/*
 * CBattleProjectileController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleProjectileController.h"
#include "../gui/Geometries.h"
#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../CGameInfo.h"
#include "../gui/CAnimation.h"
#include "CBattleInterface.h"
#include "CBattleSiegeController.h"
#include "CBattleStacksController.h"
#include "CCreatureAnimation.h"

CatapultProjectileInfo::CatapultProjectileInfo(const Point &from, const Point &dest)
{
	facA = 0.005; // seems to be constant

	// system of 2 linear equations, solutions of which are missing coefficients
	// for quadratic equation a*x*x + b*x + c
	double eq[2][3] = {
		{ static_cast<double>(from.x), 1.0, from.y - facA*from.x*from.x },
		{ static_cast<double>(dest.x), 1.0, dest.y - facA*dest.x*dest.x }
	};

	// solve system via determinants
	double det  = eq[0][0] *eq[1][1] - eq[1][0] *eq[0][1];
	double detB = eq[0][2] *eq[1][1] - eq[1][2] *eq[0][1];
	double detC = eq[0][0] *eq[1][2] - eq[1][0] *eq[0][2];

	facB = detB / det;
	facC = detC / det;

	// make sure that parabola is correct e.g. passes through from and dest
	assert(fabs(calculateY(from.x) - from.y) < 1.0);
	assert(fabs(calculateY(dest.x) - dest.y) < 1.0);
}

double CatapultProjectileInfo::calculateY(double x)
{
	return facA *pow(x, 2.0) + facB *x + facC;
}

CBattleProjectileController::CBattleProjectileController(CBattleInterface * owner):
	owner(owner)
{

}

void CBattleProjectileController::initStackProjectile(const CStack * stack)
{
	const CCreature * creature;//creature whose shots should be loaded
	if(stack->getCreature()->idNumber == CreatureID::ARROW_TOWERS)
		creature = owner->siegeController->getTurretCreature();
	else
		creature = stack->getCreature();

	if (creature->animation.projectileRay.empty())
	{
		std::shared_ptr<CAnimation> projectile = std::make_shared<CAnimation>(creature->animation.projectileImageName);
		projectile->preload();

		if(projectile->size(1) != 0)
			logAnim->error("Expected empty group 1 in stack projectile");
		else
			projectile->createFlippedGroup(0, 1);

		idToProjectile[stack->getCreature()->idNumber] = projectile;
	}
	else
	{
		idToRay[stack->getCreature()->idNumber] = creature->animation.projectileRay;
	}
}

void CBattleProjectileController::fireStackProjectile(const CStack * stack)
{
	for (auto it = projectiles.begin(); it!=projectiles.end(); ++it)
	{
		if ( !it->shotDone && it->stackID == stack->ID)
		{
			it->shotDone = true;
			return;
		}
	}
}

void CBattleProjectileController::showProjectiles(SDL_Surface *to)
{
	assert(to);

	std::list< std::list<ProjectileInfo>::iterator > toBeDeleted;
	for (auto it = projectiles.begin(); it!=projectiles.end(); ++it)
	{
		// Check if projectile is already visible (shooter animation did the shot)
		if (!it->shotDone)
			continue;

		if (idToProjectile.count(it->creID))
		{
			size_t group = it->reverse ? 1 : 0;
			auto image = idToProjectile[it->creID]->getImage(it->frameNum, group, true);

			if(image)
			{
				SDL_Rect dst;
				dst.h = image->height();
				dst.w = image->width();
				dst.x = static_cast<int>(it->x - dst.w / 2);
				dst.y = static_cast<int>(it->y - dst.h / 2);

				image->draw(to, &dst, nullptr);
			}
		}
		if (idToRay.count(it->creID))
		{
			auto const & ray = idToRay[it->creID];

			if (std::abs(it->dx) > std::abs(it->dy)) // draw in horizontal axis
			{
				int y1 =  it->y0 - ray.size() / 2;
				int y2 =  it->y - ray.size() / 2;

				int x1 = it->x0;
				int x2 = it->x;

				for (size_t i = 0; i < ray.size(); ++i)
				{
					SDL_Color beginColor{ ray[i].r1, ray[i].g1, ray[i].b1, ray[i].a1};
					SDL_Color endColor  { ray[i].r2, ray[i].g2, ray[i].b2, ray[i].a2};

					CSDL_Ext::drawLine(to, x1, y1 + i, x2, y2 + i, beginColor, endColor);
				}
			}
			else // draw in vertical axis
			{
				int x1 = it->x0 - ray.size() / 2;
				int x2 = it->x - ray.size() / 2;

				int y1 =  it->y0;
				int y2 =  it->y;

				for (size_t i = 0; i < ray.size(); ++i)
				{
					SDL_Color beginColor{ ray[i].r1, ray[i].g1, ray[i].b1, ray[i].a1};
					SDL_Color endColor  { ray[i].r2, ray[i].g2, ray[i].b2, ray[i].a2};

					CSDL_Ext::drawLine(to, x1 + i, y1, x2 + i, y2, beginColor, endColor);
				}
			}
		}

		// Update projectile
		++it->step;
		if (it->step > it->lastStep)
		{
			toBeDeleted.insert(toBeDeleted.end(), it);
		}
		else
		{
			if (it->catapultInfo)
			{
				// Parabolic shot of the trajectory, as follows: f(x) = ax^2 + bx + c
				it->x += it->dx;
				it->y = it->catapultInfo->calculateY(it->x);

				++(it->frameNum);
				it->frameNum %= idToProjectile[it->creID]->size(0);
			}
			else
			{
				// Normal projectile, just add the calculated "deltas" to the x and y positions.
				it->x += it->dx;
				it->y += it->dy;
			}
		}
	}

	for (auto & elem : toBeDeleted)
		projectiles.erase(elem);
}

bool CBattleProjectileController::hasActiveProjectile(const CStack * stack)
{
	for(auto const & instance : projectiles)
	{
		if(instance.creID == stack->getCreature()->idNumber)
		{
			return true;
		}
	}
	return false;
}


void CBattleProjectileController::createProjectile(const CStack * shooter, const CStack * target, Point from, Point dest)
{
	// Get further info about the shooter e.g. relative pos of projectile to unit.
	// If the creature id is 149 then it's a arrow tower which has no additional info so get the
	// actual arrow tower shooter instead.
	const CCreature *shooterInfo = shooter->getCreature();

	if(shooterInfo->idNumber == CreatureID::ARROW_TOWERS)
		shooterInfo = owner->siegeController->getTurretCreature();

	if(!shooterInfo->animation.missleFrameAngles.size())
		logAnim->error("Mod error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Trying to use archer's data instead..."
			, shooterInfo->nameSing);

	auto & angles = shooterInfo->animation.missleFrameAngles.size()
		? shooterInfo->animation.missleFrameAngles
		: CGI->creh->operator[](CreatureID::ARCHER)->animation.missleFrameAngles;

	// recalculate angle taking in account offsets
	//projectileAngle = atan2(fabs(destPos.y - spi.y), fabs(destPos.x - spi.x));
	//if(shooter->position < dest)
	//	projectileAngle = -projectileAngle;

	ProjectileInfo spi;
	spi.shotDone = false;
	spi.creID = shooter->getCreature()->idNumber;
	spi.stackID = shooter->ID;
	// reverse if creature is facing right OR this is non-existing stack that is not tower (war machines)
	spi.reverse = shooter ? !owner->stacksController->facingRight(shooter) : shooter->getCreature()->idNumber != CreatureID::ARROW_TOWERS;

	spi.step = 0;
	spi.frameNum = 0;

	spi.x0 = from.x;
	spi.y0 = from.y;

	spi.x = from.x;
	spi.y = from.y;

	if (target)
	{
		double animSpeed = AnimationControls::getProjectileSpeed(); // flight speed of projectile
		double distanceSquared = (dest.x - spi.x) * (dest.x - spi.x) + (dest.y - spi.y) * (dest.y - spi.y);
		double distance = sqrt(distanceSquared);
		spi.lastStep = std::round(distance / animSpeed);
		if(spi.lastStep == 0)
			spi.lastStep = 1;
		spi.dx = (dest.x - spi.x) / spi.lastStep;
		spi.dy = (dest.y - spi.y) / spi.lastStep;
	}
	else
	{
		// Catapult attack
		spi.catapultInfo.reset(new CatapultProjectileInfo(Point((int)spi.x, (int)spi.y), dest));

		double animSpeed = AnimationControls::getProjectileSpeed() / 10;
		spi.lastStep = static_cast<int>(std::abs((dest.x - spi.x) / animSpeed));
		spi.dx = animSpeed;
		spi.dy = 0;

		auto img = idToProjectile[spi.creID]->getImage(0);

		// Add explosion anim
		Point animPos(dest.x - 126 + img->width() / 2,
					  dest.y - 105 + img->height() / 2);

		//owner->addNewAnim( new CEffectAnimation(owner, catapultDamage ? "SGEXPL.DEF" : "CSGRCK.DEF", animPos.x, animPos.y));
	}
	double pi = std::atan(1)*4;

	//in some cases (known one: hero grants shooter bonus to unit) the shooter stack's projectile may not be properly initialized
	if (!idToProjectile.count(spi.creID) && !idToRay.count(spi.creID))
		initStackProjectile(shooter);

	if (idToProjectile.count(spi.creID))
	{
		// only frames below maxFrame are usable: anything  higher is either no present or we don't know when it should be used
		size_t maxFrame = std::min<size_t>(angles.size(), idToProjectile.at(spi.creID)->size(0));

		assert(maxFrame > 0);
		double projectileAngle = atan2(fabs((double)dest.y - from.y), fabs((double)dest.x - from.x));
		//if(shooter->getPosition() < dest)
		//	projectileAngle = -projectileAngle;

		// values in angles array indicate position from which this frame was rendered, in degrees.
		// find frame that has closest angle to one that we need for this shot
		size_t bestID = 0;
		double bestDiff = fabs( angles[0] / 180 * pi - projectileAngle );

		for (size_t i=1; i<maxFrame; i++)
		{
			double currentDiff = fabs( angles[i] / 180 * pi - projectileAngle );
			if (currentDiff < bestDiff)
			{
				bestID = i;
				bestDiff = currentDiff;
			}
		}

		spi.frameNum = static_cast<int>(bestID);
	}
	else if (idToRay.count(spi.creID))
	{
		// no-op
	}
	else
	{
		logGlobal->error("Unable to find valid projectile for shooter %d", spi.creID);
	}

	// Set projectile animation start delay which is specified in frames
	projectiles.push_back(spi);
}
