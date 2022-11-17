/*
 * CBattleSiegeController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/CCreatureHandler.h"

struct Point;
struct SDL_Surface;
class CAnimation;
class CStack;
class CBattleInterface;

/// Small struct which is needed for drawing the parabolic trajectory of the catapult cannon
struct CatapultProjectileInfo
{
	CatapultProjectileInfo(const Point &from, const Point &dest);

	double facA, facB, facC;

	double calculateY(double x);
};

/// Small struct which contains information about the position and the velocity of a projectile
struct ProjectileInfo
{
	double x0, y0; //initial position on the screen
	double x, y; //position on the screen
	double dx, dy; //change in position in one step
	int step, lastStep; //to know when finish showing this projectile
	int creID; //ID of creature that shot this projectile
	int stackID; //ID of stack
	int frameNum; //frame to display form projectile animation
	//bool spin; //if true, frameNum will be increased
	int animStartDelay; //frame of shooter animation when projectile should appear
	bool shotDone; // actual shot already done, projectile is flying
	bool reverse; //if true, projectile will be flipped by vertical asix
	std::shared_ptr<CatapultProjectileInfo> catapultInfo; // holds info about the parabolic trajectory of the cannon
};

class CBattleProjectileController
{
	CBattleInterface * owner;

	std::map<int, std::shared_ptr<CAnimation>> idToProjectile;
	std::map<int, std::vector<CCreature::CreatureAnimation::RayColor>> idToRay;

	std::list<ProjectileInfo> projectiles; //projectiles flying on battlefield

public:
	CBattleProjectileController(CBattleInterface * owner);

	void showProjectiles(SDL_Surface *to);
	void initStackProjectile(const CStack * stack);

	bool hasActiveProjectile(const CStack * stack);

	void createProjectile(const CStack * shooter, const CStack * target, Point from, Point dest);
};
