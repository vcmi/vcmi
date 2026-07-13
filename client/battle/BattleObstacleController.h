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
#include "../../lib/battle/BattleHex.h"
#include "../../lib/Point.h"

struct CObstacleInstance;
class JsonNode;
class ObstacleChanges;

class IImage;
class Canvas;
class CAnimation;
class BattleInterface;
class BattleRenderer;

/// Controls all currently active projectiles on the battlefield
/// (with exception of moat, which is apparently handled by siege controller)
class BattleObstacleController
{
	friend class ClientCommandManager;

	BattleInterface & owner;

	/// total time, in seconds, since start of battle. Used for animating obstacles
	float timePassed;

	/// list of all obstacles that are currently being rendered
	std::map<si32, std::shared_ptr<CAnimation>> obstacleAnimations;

	/// Current images for all present obstacles
	std::map<si32, std::shared_ptr<IImage>> obstacleImages;

	/// cached render position per obstacle, so a removal fade-out draws where the obstacle actually was
	std::map<si32, Point> obstaclePositions;

	/// removed "usual" obstacles currently fading out (they have no removal animation of their own)
	struct FadingObstacle
	{
		std::shared_ptr<IImage> image;
		Point pos;
		BattleHex hex;
		si32 id = 0;
		uint32_t elapsed = 0;
		bool started = false; // stays fully visible until the removing spell's hit stage starts the fade
	};
	std::vector<FadingObstacle> fadingObstacles;

	void loadObstacleImage(const CObstacleInstance & oi);

	/// obstacles awaiting their placement animation; shown one after another
	std::vector<std::shared_ptr<const CObstacleInstance>> obstaclePlacementQueue;
	bool placingObstacle = false;

	/// starts the placement animation of the next queued obstacle, unless one is already playing
	void placeNextObstacle();

	std::shared_ptr<IImage> getObstacleImage(const CObstacleInstance & oi);
	Point getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle);
	Point getObstaclePosition(std::shared_ptr<IImage> image, const JsonNode & obstacle);

public:
	BattleObstacleController(BattleInterface & owner);

	/// called every frame
	void tick(uint32_t msPassed);

	/// call-in from network pack, add a newly placed obstacle with any required animations
	void obstaclePlaced(const std::shared_ptr<const CObstacleInstance> & oi);

	/// call-in from network pack, remove an obstacle with any required animations
	void obstacleRemoved(const ObstacleChanges & obstacle);

	/// renders all "absolute" obstacles
	void showAbsoluteObstacles(Canvas & canvas);

	/// adds all non-"absolute" visible obstacles for rendering
	void collectRenderableObjects(BattleRenderer & renderer);
};
