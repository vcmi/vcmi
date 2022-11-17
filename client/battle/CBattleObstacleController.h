/*
 * CBattleObstacleController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_Surface;
struct BattleObjectsByHex;
class IImage;
class CAnimation;
class CBattleInterface;
class CObstacleInstance;
struct Point;

class CBattleObstacleController
{
	std::map<std::string, std::shared_ptr<CAnimation>> animationsCache;

	CBattleInterface * owner;

	std::map<si32, std::shared_ptr<CAnimation>> obstacleAnimations;
public:
	CBattleObstacleController(CBattleInterface * owner);

	void obstaclePlaced(const CObstacleInstance & oi);
	void showObstacles(SDL_Surface *to, std::vector<std::shared_ptr<const CObstacleInstance>> &obstacles);
	void showAbsoluteObstacles(SDL_Surface *to);

	void sortObjectsByHex(BattleObjectsByHex & sorted);

	std::shared_ptr<IImage> getObstacleImage(const CObstacleInstance & oi);

	Point getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle);

	void redrawBackgroundWithHexes();
};
