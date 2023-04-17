/*
 * BattleProjectileController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleProjectileController.h"

#include "BattleInterface.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "CreatureAnimation.h"

#include "../render/Canvas.h"
#include "../gui/CGuiHandler.h"
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

void ProjectileMissile::show(Canvas & canvas)
{
	size_t group = reverse ? 1 : 0;
	auto image = animation->getImage(frameNum, group, true);

	if(image)
	{
		Point pos {
			vstd::lerp(from.x, dest.x, progress) - image->width() / 2,
			vstd::lerp(from.y, dest.y, progress) - image->height() / 2,
		};

		canvas.draw(image, pos);
	}

	float timePassed = GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	progress += timePassed * speed;
}

void ProjectileAnimatedMissile::show(Canvas & canvas)
{
	ProjectileMissile::show(canvas);
	frameProgress += AnimationControls::getSpellEffectSpeed() * GH.mainFPSmng->getElapsedMilliseconds() / 1000;
	size_t animationSize = animation->size(reverse ? 1 : 0);
	while (frameProgress > animationSize)
		frameProgress -= animationSize;

	frameNum = std::floor(frameProgress);
}

void ProjectileCatapult::show(Canvas & canvas)
{
	frameProgress += AnimationControls::getSpellEffectSpeed() * GH.mainFPSmng->getElapsedMilliseconds() / 1000;
	int frameCounter = std::floor(frameProgress);
	int frameIndex = (frameCounter + 1) % animation->size(0);

	auto image = animation->getImage(frameIndex, 0, true);

	if(image)
	{
		int posX = vstd::lerp(from.x, dest.x, progress);
		int posY = calculateCatapultParabolaY(from, dest, posX);
		Point pos(posX, posY);

		canvas.draw(image, pos);
	}

	float timePassed = GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	progress += timePassed * speed;
}

void ProjectileRay::show(Canvas & canvas)
{
	Point curr {
		vstd::lerp(from.x, dest.x, progress),
		vstd::lerp(from.y, dest.y, progress),
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
			canvas.drawLine(Point(x1, y1 + i), Point(x2, y2+i), ray.start, ray.end);
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

			canvas.drawLine(Point(x1 + i, y1), Point(x2 + i, y2), ray.start, ray.end);
		}
	}

	float timePassed = GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	progress += timePassed * speed;
}

BattleProjectileController::BattleProjectileController(BattleInterface & owner):
	owner(owner)
{}

const CCreature & BattleProjectileController::getShooter(const CStack * stack) const
{
	const CCreature * creature = stack->unitType();

	if(creature->getId() == CreatureID::ARROW_TOWERS)
		creature = owner.siegeController->getTurretCreature();

	if(creature->animation.missleFrameAngles.empty())
	{
		logAnim->error("Mod error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Trying to use archer's data instead...", creature->getNameSingularTranslated());
		creature = CGI->creh->objects[CreatureID::ARCHER];
	}

	return *creature;
}

bool BattleProjectileController::stackUsesRayProjectile(const CStack * stack) const
{
	return !getShooter(stack).animation.projectileRay.empty();
}

bool BattleProjectileController::stackUsesMissileProjectile(const CStack * stack) const
{
	return !getShooter(stack).animation.projectileImageName.empty();
}

void BattleProjectileController::initStackProjectile(const CStack * stack)
{
	if (!stackUsesMissileProjectile(stack))
		return;

	const CCreature & creature = getShooter(stack);
	projectilesCache[creature.animation.projectileImageName] = createProjectileImage(creature.animation.projectileImageName);
}

std::shared_ptr<CAnimation> BattleProjectileController::createProjectileImage(const std::string & path )
{
	std::shared_ptr<CAnimation> projectile = std::make_shared<CAnimation>(path);
	projectile->preload();

	if(projectile->size(1) != 0)
		logAnim->error("Expected empty group 1 in stack projectile");
	else
		projectile->createFlippedGroup(0, 1);

	return projectile;
}

std::shared_ptr<CAnimation> BattleProjectileController::getProjectileImage(const CStack * stack)
{
	const CCreature & creature = getShooter(stack);
	std::string imageName = creature.animation.projectileImageName;

	if (!projectilesCache.count(imageName))
		initStackProjectile(stack);

	return projectilesCache[imageName];
}

void BattleProjectileController::emitStackProjectile(const CStack * stack)
{
	int stackID = stack ? stack->ID : -1;

	for(const auto & projectile : projectiles)
	{
		if ( !projectile->playing && projectile->shooterID == stackID)
		{
			projectile->playing = true;
			return;
		}
	}
}

void BattleProjectileController::showProjectiles(Canvas & canvas)
{
	for(const auto & projectile : projectiles)
	{
		if ( projectile->playing )
			projectile->show(canvas);
	}

	vstd::erase_if(projectiles, [&](const std::shared_ptr<ProjectileBase> & projectile){
		return projectile->progress > 1.0f;
	});
}

bool BattleProjectileController::hasActiveProjectile(const CStack * stack, bool emittedOnly) const
{
	int stackID = stack ? stack->ID : -1;

	for(const auto & instance : projectiles)
	{
		if(instance->shooterID == stackID && (instance->playing || !emittedOnly))
		{
			return true;
		}
	}
	return false;
}

float BattleProjectileController::computeProjectileFlightTime( Point from, Point dest, double animSpeed)
{
	float distanceSquared = (dest.x - from.x) * (dest.x - from.x) + (dest.y - from.y) * (dest.y - from.y);
	float distance = sqrt(distanceSquared);

	assert(distance > 1.f);

	return animSpeed / std::max( 1.f, distance);
}

int BattleProjectileController::computeProjectileFrameID( Point from, Point dest, const CStack * stack)
{
	const CCreature & creature = getShooter(stack);

	auto & angles = creature.animation.missleFrameAngles;
	auto animation = getProjectileImage(stack);

	// only frames below maxFrame are usable: anything  higher is either no present or we don't know when it should be used
	size_t maxFrame = std::min<size_t>(angles.size(), animation->size(0));

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
	return bestID;
}

void BattleProjectileController::createCatapultProjectile(const CStack * shooter, Point from, Point dest)
{
	auto catapultProjectile       = new ProjectileCatapult();

	catapultProjectile->animation = getProjectileImage(shooter);
	catapultProjectile->progress  = 0;
	catapultProjectile->speed     = computeProjectileFlightTime(from, dest, AnimationControls::getCatapultSpeed());
	catapultProjectile->from      = from;
	catapultProjectile->dest      = dest;
	catapultProjectile->shooterID = shooter->ID;
	catapultProjectile->playing   = false;
	catapultProjectile->frameProgress = 0.f;

	projectiles.push_back(std::shared_ptr<ProjectileBase>(catapultProjectile));
}

void BattleProjectileController::createProjectile(const CStack * shooter, Point from, Point dest)
{
	const CCreature & shooterInfo = getShooter(shooter);

	std::shared_ptr<ProjectileBase> projectile;
	if (stackUsesRayProjectile(shooter) && stackUsesMissileProjectile(shooter))
	{
		logAnim->error("Mod error: Creature '%s' has both missile and ray projectiles configured. Mod should be fixed. Using ray projectile configuration...", shooterInfo.getNameSingularTranslated());
	}

	if (stackUsesRayProjectile(shooter))
	{
		auto rayProjectile = new ProjectileRay();
		projectile.reset(rayProjectile);

		rayProjectile->rayConfig = shooterInfo.animation.projectileRay;
		rayProjectile->speed     = computeProjectileFlightTime(from, dest, AnimationControls::getRayProjectileSpeed());
	}
	else if (stackUsesMissileProjectile(shooter))
	{
		auto missileProjectile = new ProjectileMissile();
		projectile.reset(missileProjectile);

		missileProjectile->animation = getProjectileImage(shooter);
		missileProjectile->reverse   = !owner.stacksController->facingRight(shooter);
		missileProjectile->frameNum  = computeProjectileFrameID(from, dest, shooter);
		missileProjectile->speed     = computeProjectileFlightTime(from, dest, AnimationControls::getProjectileSpeed());
	}


	projectile->from      = from;
	projectile->dest      = dest;
	projectile->shooterID = shooter->ID;
	projectile->progress  = 0;
	projectile->playing   = false;

	projectiles.push_back(projectile);
}

void BattleProjectileController::createSpellProjectile(const CStack * shooter, Point from, Point dest, const CSpell * spell)
{
	double projectileAngle = std::abs(atan2(dest.x - from.x, dest.y - from.y));
	std::string animToDisplay = spell->animationInfo.selectProjectile(projectileAngle);

	assert(!animToDisplay.empty());

	if(!animToDisplay.empty())
	{
		auto projectile = new ProjectileAnimatedMissile();

		projectile->animation     = createProjectileImage(animToDisplay);
		projectile->frameProgress = 0;
		projectile->frameNum      = 0;
		projectile->reverse       = from.x > dest.x;
		projectile->from          = from;
		projectile->dest          = dest;
		projectile->shooterID     = shooter ? shooter->ID : -1;
		projectile->progress      = 0;
		projectile->speed         = computeProjectileFlightTime(from, dest, AnimationControls::getProjectileSpeed());
		projectile->playing       = false;

		projectiles.push_back(std::shared_ptr<ProjectileBase>(projectile));
	}
}
