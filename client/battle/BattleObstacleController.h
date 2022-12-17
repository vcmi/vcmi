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

VCMI_LIB_NAMESPACE_BEGIN

struct BattleHex;
struct CObstacleInstance;

VCMI_LIB_NAMESPACE_END

class IImage;
class Canvas;
class CAnimation;
class BattleInterface;
class BattleRenderer;
struct Point;

/// Controls all currently active projectiles on the battlefield
/// (with exception of moat, which is apparently handled by siege controller)
class BattleObstacleController
{
	BattleInterface & owner;

	/// cached animations of all obstacles in current battle
	std::map<std::string, std::shared_ptr<CAnimation>> animationsCache;

	/// list of all obstacles that are currently being rendered
	std::map<si32, std::shared_ptr<CAnimation>> obstacleAnimations;

	void loadObstacleImage(const CObstacleInstance & oi);

	std::shared_ptr<IImage> getObstacleImage(const CObstacleInstance & oi);
	Point getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle);

public:
	BattleObstacleController(BattleInterface & owner);

	/// call-in from network pack, add newly placed obstacles with any required animations
	void obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> & oi);

	/// renders all "absolute" obstacles
	void showAbsoluteObstacles(Canvas & canvas, const Point & offset);

	/// adds all non-"absolute" visible obstacles for rendering
	void collectRenderableObjects(BattleRenderer & renderer);
};
