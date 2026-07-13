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

#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../GameEngine.h"

#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpell.h"

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
}

void ProjectileMissile::tick(uint32_t msPassed)
{
	float timePassed = msPassed / 1000.f;
	progress += timePassed * speed;
}

void ProjectileAnimatedMissile::tick(uint32_t msPassed)
{
	ProjectileMissile::tick(msPassed);
	frameProgress += AnimationControls::getSpellEffectSpeed() * msPassed / 1000;
	size_t animationSize = animation->size(reverse ? 1 : 0);
	while (frameProgress > animationSize)
		frameProgress -= animationSize;

	frameNum = std::floor(frameProgress);
}

void ProjectileCatapult::tick(uint32_t msPassed)
{
	frameProgress += AnimationControls::getSpellEffectSpeed() * msPassed / 1000;
	float timePassed = msPassed / 1000.f;
	progress += timePassed * speed;
}

void ProjectileCatapult::show(Canvas & canvas)
{
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
}

/// draws a single ray segment as a bundle of parallel gradient sub-lines from `from` to `curr`
static void drawRaySegment(Canvas & canvas, const Point & from, const Point & curr, const std::vector<RayColor> & rayConfig)
{
	Point length = curr - from;
	int lines = static_cast<int>(rayConfig.size());

	//select axis to draw ray on, we want angle to be less than 45 degrees so individual sub-rays won't overlap each other

	if (std::abs(length.x) > std::abs(length.y)) // draw in horizontal axis
	{
		int y1 =  from.y - lines / 2;
		int y2 =  curr.y - lines / 2;

		for (int i = 0; i < lines; ++i)
			canvas.drawLine(Point(from.x, y1 + i), Point(curr.x, y2 + i), rayConfig[i].start, rayConfig[i].end);
	}
	else // draw in vertical axis
	{
		int x1 = from.x - lines / 2;
		int x2 = curr.x - lines / 2;

		for (int i = 0; i < lines; ++i)
			canvas.drawLine(Point(x1 + i, from.y), Point(x2 + i, curr.y), rayConfig[i].start, rayConfig[i].end);
	}
}

void ProjectileRay::show(Canvas & canvas)
{
	if (path.empty())
	{
		Point curr {
			vstd::lerp(from.x, dest.x, progress),
			vstd::lerp(from.y, dest.y, progress),
		};
		drawRaySegment(canvas, from, curr, rayConfig);
		return;
	}

	// jagged polyline, revealed one hop at a time as `progress` advances
	int totalSegments = static_cast<int>(path.size()) - 1;
	int segmentsPerHop = totalSegments / hopCount;
	int revealedHops = std::min(hopCount, static_cast<int>(progress * hopCount) + 1);

	for (int i = 0; i < revealedHops * segmentsPerHop; ++i)
		drawRaySegment(canvas, path[i], path[i + 1], rayConfig);
}

void ProjectileRay::tick(uint32_t msPassed)
{
	float timePassed = msPassed / 1000.f;

	if (progress < 1.f)
		progress = std::min(1.f, progress + timePassed * speed); // hold fully drawn once complete
	else if (linger > 0.f)
		linger -= timePassed;
	else
		progress = 2.f; // linger expired -> let controller erase it
}

BattleProjectileController::BattleProjectileController(BattleInterface & owner):
	owner(owner)
{}

const CCreature & BattleProjectileController::getShooter(const CStack * stack) const
{
	const CCreature * creature = stack->unitType();

	if(stack->isTurret())
		creature = owner.siegeController->getTurretCreature(stack->initialPosition);

	if(creature->animation.missileFrameAngles.empty() && creature->animation.projectileRay.empty())
	{
		logAnim->error("Mod error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Trying to use archer's data instead...", creature->getNameSingularTranslated());
		creature = CreatureID(CreatureID::ARCHER).toCreature();
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

std::shared_ptr<CAnimation> BattleProjectileController::createProjectileImage(const AnimationPath & path )
{
	std::shared_ptr<CAnimation> projectile = ENGINE->renderHandler().loadAnimation(path, EImageBlitMode::COLORKEY);

	if(projectile->size(1) != 0)
		logAnim->error("Expected empty group 1 in stack projectile");
	else
		projectile->createFlippedGroup(0, 1);

	return projectile;
}

std::shared_ptr<CAnimation> BattleProjectileController::getProjectileImage(const CStack * stack)
{
	const CCreature & creature = getShooter(stack);
	AnimationPath imageName = creature.animation.projectileImageName;

	if (!projectilesCache.count(imageName))
		initStackProjectile(stack);

	return projectilesCache[imageName];
}

void BattleProjectileController::emitStackProjectile(const CStack * stack)
{
	int stackID = stack ? stack->unitId() : -1;

	for (auto projectile : projectiles)
	{
		if ( !projectile->playing && projectile->shooterID == stackID)
		{
			projectile->playing = true;
			return;
		}
	}
}

void BattleProjectileController::render(Canvas & canvas)
{
	for ( auto projectile: projectiles)
	{
		if ( projectile->playing )
			projectile->show(canvas);
	}
}

void BattleProjectileController::tick(uint32_t msPassed)
{
	for ( auto projectile: projectiles)
	{
		if ( projectile->playing )
			projectile->tick(msPassed);
	}

	vstd::erase_if(projectiles, [&](const std::shared_ptr<ProjectileBase> & projectile){
		return projectile->progress > 1.0f;
	});
}

bool BattleProjectileController::hasActiveProjectile(const CStack * stack, bool emittedOnly) const
{
	int stackID = stack ? stack->unitId() : -1;

	for(auto const & instance : projectiles)
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

	auto & angles = creature.animation.missileFrameAngles;
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
	catapultProjectile->shooterID = shooter->unitId();
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
	projectile->shooterID = shooter->unitId();
	projectile->progress  = 0;
	projectile->playing   = false;

	projectiles.push_back(projectile);
}

void BattleProjectileController::createSpellProjectile(const CStack * shooter, Point from, Point dest, const CSpell * spell)
{
	double projectileAngle = std::abs(atan2(dest.x - from.x, dest.y - from.y));
	AnimationPath animToDisplay = spell->animationInfo.selectProjectile(projectileAngle);

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
		projectile->shooterID     = shooter ? shooter->unitId() : -1;
		projectile->progress      = 0;
		projectile->speed         = computeProjectileFlightTime(from, dest, AnimationControls::getProjectileSpeed());
		projectile->playing       = false;

		projectiles.push_back(std::shared_ptr<ProjectileBase>(projectile));
	}
}

/// appends a midpoint-displaced polyline from a (exclusive) to b (inclusive) to `out`
static void addJaggedSegment(std::vector<Point> & out, const Point & a, const Point & b, float amplitude, int depth)
{
	if (depth <= 0)
	{
		out.push_back(b);
		return;
	}

	Point dir = b - a;
	float len = std::sqrt(static_cast<float>(dir.x * dir.x + dir.y * dir.y));

	Point mid = (a + b) / 2;
	if (len > 1.f)
	{
		static std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> dist(-amplitude, amplitude);
		float off = dist(rng) * len;
		// displace the midpoint perpendicular to the segment
		mid.x += static_cast<int>(-dir.y / len * off);
		mid.y += static_cast<int>( dir.x / len * off);
	}

	addJaggedSegment(out, a, mid, amplitude, depth - 1);
	addJaggedSegment(out, mid, b, amplitude, depth - 1);
}

/// expands `keys` (edge -> center gradient) into a symmetric `width`-line gradient with interpolated intermediates
static std::vector<RayColor> expandRayColors(const std::vector<RayColor> & keys, int width)
{
	if (keys.empty() || width <= static_cast<int>(keys.size()))
		return keys;

	std::vector<RayColor> out(width);
	float center = (width - 1) / 2.f;
	for (int k = 0; k < width; ++k)
	{
		// distance from edge, 0 at outer lines -> 1 at center
		float edgeToCenter = center > 0 ? 1.f - std::abs(k - center) / center : 1.f;
		float keyPos = edgeToCenter * (keys.size() - 1);
		int i0 = static_cast<int>(keyPos);
		int i1 = std::min<int>(i0 + 1, keys.size() - 1);
		float f = keyPos - i0;
		out[k].start = vstd::lerp(keys[i0].start, keys[i1].start, f);
		out[k].end   = vstd::lerp(keys[i0].end,   keys[i1].end,   f);
	}
	return out;
}

void BattleProjectileController::createSpellRayProjectile(const CStack * caster, const std::vector<Point> & targetPoints, const std::vector<RayColor> & rayConfig, float jaggedness, float hopDelay, int width)
{
	if (targetPoints.size() < 2)
		return;

	auto ray = new ProjectileRay();

	// each hop between consecutive targets is midpoint-displaced twice -> 4 jagged sub-segments per hop
	ray->path.push_back(targetPoints.front());
	for (size_t i = 1; i < targetPoints.size(); ++i)
		addJaggedSegment(ray->path, targetPoints[i - 1], targetPoints[i], jaggedness, 2);

	ray->hopCount  = static_cast<int>(targetPoints.size()) - 1;
	// progress runs 0->1 across all hops; one hop is revealed every hopDelay seconds
	float totalTime = ray->hopCount * hopDelay;
	ray->rayConfig = expandRayColors(rayConfig, width);
	ray->linger    = 0.1f; // keep the fully-formed ray visible briefly before it vanishes
	ray->shooterID = caster ? caster->unitId() : -1;
	ray->progress  = 0;
	ray->speed     = totalTime > 0 ? 1.f / totalTime : 1000.f;
	ray->playing   = true;

	projectiles.push_back(std::shared_ptr<ProjectileBase>(ray));
}
