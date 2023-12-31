/*
 * MapViewController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IMapRendererObserver.h"
#include "../../lib/Point.h"

VCMI_LIB_NAMESPACE_BEGIN
struct ObjectPosInfo;
class PlayerColor;
VCMI_LIB_NAMESPACE_END

struct MapRendererContextState;

class MapViewCache;
class MapViewModel;
class IMapRendererContext;
class MapRendererAdventureContext;
class MapRendererAdventureFadingContext;
class MapRendererAdventureMovingContext;
class MapRendererAdventureTransitionContext;
class MapRendererWorldViewContext;
class MapRendererSpellViewContext;
class MapRendererPuzzleMapContext;

/// Class responsible for updating view state,
/// such as its position and any animations
class MapViewController : public IMapObjectObserver
{
	std::shared_ptr<IMapRendererContext> context;
	std::shared_ptr<MapRendererContextState> state;
	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<MapViewCache> view;

	// all potential contexts for view
	// only some are present at any given moment, rest are nullptr's
	std::shared_ptr<MapRendererAdventureContext> adventureContext;
	std::shared_ptr<MapRendererAdventureMovingContext> movementContext;
	std::shared_ptr<MapRendererAdventureTransitionContext> teleportContext;
	std::shared_ptr<MapRendererAdventureFadingContext> fadingOutContext;
	std::shared_ptr<MapRendererAdventureFadingContext> fadingInContext;
	std::shared_ptr<MapRendererWorldViewContext> worldViewContext;
	std::shared_ptr<MapRendererSpellViewContext> spellViewContext;
	std::shared_ptr<MapRendererPuzzleMapContext> puzzleMapContext;

private:
	const int defaultTileSize = 32;
	const int zoomTileDeadArea = 5;
	Point targetTileSize = Point(32, 32);
	bool wasInDeadZone = true;

	bool isEventInstant(const CGObjectInstance * obj, const PlayerColor & initiator);
	bool isEventVisible(const CGObjectInstance * obj, const PlayerColor & initiator);
	bool isEventVisible(const CGHeroInstance * obj, const int3 & from, const int3 & dest);

	void fadeOutObject(const CGObjectInstance * obj);
	void fadeInObject(const CGObjectInstance * obj);

	void removeObject(const CGObjectInstance * obj);
	void addObject(const CGObjectInstance * obj);

	// IMapObjectObserver impl
	bool hasOngoingAnimations() override;
	void onObjectFadeIn(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void onObjectFadeOut(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void onObjectInstantAdd(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void onObjectInstantRemove(const CGObjectInstance * obj, const PlayerColor & initiator) override;
	void onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onBeforeHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;

	void resetContext();
	void updateState();

public:
	MapViewController(std::shared_ptr<MapViewModel> model, std::shared_ptr<MapViewCache> view);

	std::shared_ptr<IMapRendererContext> getContext() const;

	void setViewCenter(const int3 & position);
	void setViewCenter(const Point & position, int level);
	void setTileSize(const Point & tileSize);
	void modifyTileSize(int stepsChange);
	void tick(uint32_t timePassed);
	void afterRender();

	void activateAdventureContext(uint32_t animationTime);
	void activateAdventureContext();
	void activateWorldViewContext();
	void activateSpellViewContext();
	void activatePuzzleMapContext(const int3 & grailPosition);

	void setTerrainVisibility(bool showAllTerrain);
	void setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions);
};
