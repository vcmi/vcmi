/*
 * BattleObstacleController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleHex;
struct CObstacleInstance;
class JsonNode;
class ObstacleChanges;
class Point;

VCMI_LIB_NAMESPACE_END

class IImage;
class Canvas;
class CAnimation;
class BattleInterface;
class BattleRenderer;

/// Controls all currently active projectiles on the battlefield
/// (with exception of moat, which is apparently handled by siege controller)
class BattleObstacleController
{
	BattleInterface & owner;

	/// total time, in seconds, since start of battle. Used for animating obstacles
	float timePassed;

	/// cached animations of all obstacles in current battle
	std::map<AnimationPath, std::shared_ptr<CAnimation>> animationsCache;

	/// list of all obstacles that are currently being rendered
	std::map<si32, std::shared_ptr<CAnimation>> obstacleAnimations;

	void loadObstacleImage(const CObstacleInstance & oi);

	std::shared_ptr<IImage> getObstacleImage(const CObstacleInstance & oi);
	Point getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle);
	Point getObstaclePosition(std::shared_ptr<IImage> image, const JsonNode & obstacle);

public:
	BattleObstacleController(BattleInterface & owner);

	/// called every frame
	void tick(uint32_t msPassed);

	/// call-in from network pack, add newly placed obstacles with any required animations
	void obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> & oi);

	/// call-in from network pack, remove required obstacles with any required animations
	void obstacleRemoved(const std::vector<ObstacleChanges> & obstacles);

	/// renders all "absolute" obstacles
	void showAbsoluteObstacles(Canvas & canvas);

	/// adds all non-"absolute" visible obstacles for rendering
	void collectRenderableObjects(BattleRenderer & renderer);
};
