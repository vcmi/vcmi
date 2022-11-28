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

#include "CBattleInterface.h"
#include "CBattleSiegeController.h"
#include "CBattleStacksController.h"
#include "CCreatureAnimation.h"

#include "../gui/Geometries.h"
#include "../gui/CAnimation.h"
#include "../gui/CCanvas.h"
#include "../CGameInfo.h"

#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"

static double calculateCatapultParabolaY(const Point & from, const Point & dest, int x)
{
	double facA = 0.005; // seems to be constant

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

	double facB = detB / det;
	double facC = detC / det;

	return facA *pow(x, 2.0) + facB *x + facC;
}

void ProjectileMissile::show(std::shared_ptr<CCanvas> canvas)
{
	size_t group = reverse ? 1 : 0;
	auto image = animation->getImage(frameNum, group, true);

	if(image)
	{
		float progress = float(step) / steps;

		Point pos {
			CSDL_Ext::lerp(from.x, dest.x, progress) - image->width() / 2,
			CSDL_Ext::lerp(from.y, dest.y, progress) - image->height() / 2,
		};

		canvas->draw(image, pos);
	}

	++step;
}

void ProjectileCatapult::show(std::shared_ptr<CCanvas> canvas)
{
	size_t group = reverse ? 1 : 0;
	auto image = animation->getImage(frameNum, group, true);

	if(image)
	{
		float progress = float(step) / steps;

		int posX = CSDL_Ext::lerp(from.x, dest.x, progress);
		int posY = calculateCatapultParabolaY(from, dest, posX);
		Point pos(posX, posY);

		canvas->draw(image, pos);

		frameNum = (frameNum + 1) % animation->size(0);
	}
	++step;
}

void ProjectileRay::show(std::shared_ptr<CCanvas> canvas)
{
	float progress = float(step) / steps;

	Point curr {
		CSDL_Ext::lerp(from.x, dest.x, progress),
		CSDL_Ext::lerp(from.y, dest.y, progress),
	};

	Point length = curr - from;

	//select axis to draw ray on, we want angle to be less than 45 degrees so individual sub-rays won't overlap each other

	if (std::abs(length.x) > std::abs(length.y)) // draw in horizontal axis
	{
		int y1 =  from.y - rayConfig.size() / 2;
		int y2 =  curr.y - rayConfig.size() / 2;

		int x1 = from.x;
		int x2 = curr.x;

		for (size_t i = 0; i < rayConfig.size(); ++i)
		{
			auto ray = rayConfig[i];
			SDL_Color beginColor{ ray.r1, ray.g1, ray.b1, ray.a1};
			SDL_Color endColor  { ray.r2, ray.g2, ray.b2, ray.a2};

			canvas->drawLine(Point(x1, y1 + i), Point(x2, y2+i), beginColor, endColor);
		}
	}
	else // draw in vertical axis
	{
		int x1 = from.x - rayConfig.size() / 2;
		int x2 = curr.x - rayConfig.size() / 2;

		int y1 = from.y;
		int y2 = curr.y;

		for (size_t i = 0; i < rayConfig.size(); ++i)
		{
			auto ray = rayConfig[i];
			SDL_Color beginColor{ ray.r1, ray.g1, ray.b1, ray.a1};
			SDL_Color endColor  { ray.r2, ray.g2, ray.b2, ray.a2};

			canvas->drawLine(Point(x1 + i, y1), Point(x2 + i, y2), beginColor, endColor);
		}
	}
	++step;
}

CBattleProjectileController::CBattleProjectileController(CBattleInterface * owner):
	owner(owner)
{}

const CCreature * CBattleProjectileController::getShooter(const CStack * stack)
{
	const CCreature * creature = stack->getCreature();

	if(creature->idNumber == CreatureID::ARROW_TOWERS)
		creature = owner->siegeController->getTurretCreature();

	if(creature->animation.missleFrameAngles.empty())
	{
		logAnim->error("Mod error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Trying to use archer's data instead...", creature->nameSing);
		creature = CGI->creh->objects[CreatureID::ARCHER];
	}

	return creature;
}

bool CBattleProjectileController::stackUsesRayProjectile(const CStack * stack)
{
	return !getShooter(stack)->animation.projectileRay.empty();
}

bool CBattleProjectileController::stackUsesMissileProjectile(const CStack * stack)
{
	return !getShooter(stack)->animation.projectileImageName.empty();
}

void CBattleProjectileController::initStackProjectile(const CStack * stack)
{
	if (!stackUsesMissileProjectile(stack))
		return;

	const CCreature * creature = getShooter(stack);

	std::shared_ptr<CAnimation> projectile = std::make_shared<CAnimation>(creature->animation.projectileImageName);
	projectile->preload();

	if(projectile->size(1) != 0)
		logAnim->error("Expected empty group 1 in stack projectile");
	else
		projectile->createFlippedGroup(0, 1);

	projectilesCache[creature->animation.projectileImageName] = projectile;
}

std::shared_ptr<CAnimation> CBattleProjectileController::getProjectileImage(const CStack * stack)
{
	const CCreature * creature = getShooter(stack);
	std::string imageName = creature->animation.projectileImageName;

	if (!projectilesCache.count(imageName))
		initStackProjectile(stack);

	return projectilesCache[imageName];
}

void CBattleProjectileController::emitStackProjectile(const CStack * stack)
{
	for (auto projectile : projectiles)
	{
		if ( !projectile->playing && projectile->shooterID == stack->ID)
		{
			projectile->playing = true;
			return;
		}
	}
}

void CBattleProjectileController::showProjectiles(std::shared_ptr<CCanvas> canvas)
{
	for ( auto it = projectiles.begin(); it != projectiles.end();)
	{
		auto projectile = *it;
		// Check if projectile is already visible (shooter animation did the shot)
		//if (!it->shotDone)
		//	continue;

		if ( projectile->playing )
			projectile->show(canvas);

		// finished flying
		if ( projectile->step > projectile->steps)
			it = projectiles.erase(it);
		else
			it++;
	}
}

bool CBattleProjectileController::hasActiveProjectile(const CStack * stack)
{
	for(auto const & instance : projectiles)
	{
		if(instance->shooterID == stack->ID)
		{
			return true;
		}
	}
	return false;
}

void CBattleProjectileController::createProjectile(const CStack * shooter, const CStack * target, Point from, Point dest)
{
	const CCreature *shooterInfo = getShooter(shooter);

	std::shared_ptr<ProjectileBase> projectile;

	if (!target)
	{
		auto catapultProjectile= new ProjectileCatapult();
		projectile.reset(catapultProjectile);

		catapultProjectile->animation = getProjectileImage(shooter);
		catapultProjectile->wallDamageAmount = 0; //FIXME - receive from caller
		catapultProjectile->frameNum = 0;
		catapultProjectile->reverse = false;
		catapultProjectile->step = 0;
		catapultProjectile->steps = 0;

		//double animSpeed = AnimationControls::getProjectileSpeed() / 10;
		//catapultProjectile->steps = std::round(std::abs((dest.x - from.x) / animSpeed));
	}
	else
	{
		if (stackUsesRayProjectile(shooter) && stackUsesMissileProjectile(shooter))
		{
			logAnim->error("Mod error: Creature '%s' has both missile and ray projectiles configured. Mod should be fixed. Using ray projectile configuration...", shooterInfo->nameSing);
		}

		if (stackUsesRayProjectile(shooter))
		{
			auto rayProjectile = new ProjectileRay();
			projectile.reset(rayProjectile);

			rayProjectile->rayConfig = shooterInfo->animation.projectileRay;
		}
		else if (stackUsesMissileProjectile(shooter))
		{
			auto missileProjectile = new ProjectileMissile();
			projectile.reset(missileProjectile);

			auto & angles = shooterInfo->animation.missleFrameAngles;

			missileProjectile->animation = getProjectileImage(shooter);
			missileProjectile->reverse  = !owner->stacksController->facingRight(shooter);

			// only frames below maxFrame are usable: anything  higher is either no present or we don't know when it should be used
			size_t maxFrame = std::min<size_t>(angles.size(), missileProjectile->animation->size(0));

			assert(maxFrame > 0);
			double projectileAngle = -atan2(dest.y - from.y, std::abs(dest.x - from.x));

			// values in angles array indicate position from which this frame was rendered, in degrees.
			// possible range is 90 ... -90, where projectile for +90 will be used for shooting upwards, +0 for shots towards right and -90 for downwards shots
			// find frame that has closest angle to one that we need for this shot
			int bestID = 0;
			double bestDiff = fabs( angles[0] / 180 * M_PI - projectileAngle );

			for (int i=1; i<maxFrame; i++)
			{
				double currentDiff = fabs( angles[i] / 180 * M_PI - projectileAngle );
				if (currentDiff < bestDiff)
				{
					bestID = i;
					bestDiff = currentDiff;
				}
			}
			missileProjectile->frameNum = bestID;
		}
	}

	double animSpeed = AnimationControls::getProjectileSpeed(); // flight speed of projectile
	if (!target)
		animSpeed *= 0.2; // catapult attack needs slower speed

	double distanceSquared = (dest.x - from.x) * (dest.x - from.x) + (dest.y - from.y) * (dest.y - from.y);
	double distance = sqrt(distanceSquared);
	projectile->steps = std::round(distance / animSpeed);
	if(projectile->steps == 0)
		projectile->steps = 1;

	projectile->from     = from;
	projectile->dest     = dest;
	projectile->shooterID = shooter->ID;
	projectile->step     = 0;
	projectile->playing  = false;

	projectiles.push_back(projectile);
}
