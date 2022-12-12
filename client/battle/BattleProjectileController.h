/*
 * BattleSiegeController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/CCreatureHandler.h"
#include "../gui/Geometries.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class CSpell;

VCMI_LIB_NAMESPACE_END

struct Point;
class CAnimation;
class Canvas;
class BattleInterface;

/// Base class for projectiles
struct ProjectileBase
{
	virtual ~ProjectileBase() = default;
	virtual void show(Canvas & canvas) =  0;

	Point from; // initial position on the screen
	Point dest; // target position on the screen

	int step;      // current step counter
	int steps;     // total number of steps/frames to show
	int shooterID; // ID of shooter stack
	bool playing;  // if set to true, projectile animation is playing, e.g. flying to target
};

/// Projectile for most shooters - render pre-selected frame moving in straight line from origin to destination
struct ProjectileMissile : ProjectileBase
{
	void show(Canvas & canvas) override;

	std::shared_ptr<CAnimation> animation;
	int frameNum;  // frame to display from projectile animation
	bool reverse;  // if true, projectile will be flipped by vertical axis
};

/// Projectile for spell - render animation moving in straight line from origin to destination
struct ProjectileAnimatedMissile : ProjectileMissile
{
	void show(Canvas & canvas) override;
	float frameProgress;
};

/// Projectile for catapult - render spinning projectile moving at parabolic trajectory to its destination
struct ProjectileCatapult : ProjectileBase
{
	void show(Canvas & canvas) override;

	std::shared_ptr<CAnimation> animation;
	int frameNum;  // frame to display from projectile animation
};

/// Projectile for mages/evil eye - render ray expanding from origin position to destination
struct ProjectileRay : ProjectileBase
{
	void show(Canvas & canvas) override;

	std::vector<CCreature::CreatureAnimation::RayColor> rayConfig;
};

/// Class that manages all ongoing projectiles in the game
/// ... even though in H3 only 1 projectile can be on screen at any point of time
class BattleProjectileController
{
	BattleInterface * owner;

	/// all projectiles loaded during current battle
	std::map<std::string, std::shared_ptr<CAnimation>> projectilesCache;

	/// projectiles currently flying on battlefield
	std::vector<std::shared_ptr<ProjectileBase>> projectiles;

	std::shared_ptr<CAnimation> getProjectileImage(const CStack * stack);
	std::shared_ptr<CAnimation> createProjectileImage(const std::string & path );
	void initStackProjectile(const CStack * stack);

	bool stackUsesRayProjectile(const CStack * stack);
	bool stackUsesMissileProjectile(const CStack * stack);

	void showProjectile(Canvas & canvas, std::shared_ptr<ProjectileBase> projectile);

	const CCreature * getShooter(const CStack * stack);

	int computeProjectileFrameID( Point from, Point dest, const CStack * stack);
	int computeProjectileFlightTime( Point from, Point dest, double speed);

public:
	BattleProjectileController(BattleInterface * owner);

	/// renders all currently active projectiles
	void showProjectiles(Canvas & canvas);

	/// returns true if stack has projectile that is yet to hit target
	bool hasActiveProjectile(const CStack * stack) const;

	/// starts rendering previously created projectile
	void emitStackProjectile(const CStack * stack);

	/// creates (but not emits!) projectile and initializes it based on parameters
	void createProjectile(const CStack * shooter, Point from, Point dest);
	void createSpellProjectile(const CStack * shooter, Point from, Point dest, const CSpell * spell);
	void createCatapultProjectile(const CStack * shooter, Point from, Point dest);
};
