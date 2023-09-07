/*
 * MapAudioPlayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mapView/IMapRendererObserver.h"
#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
class ObjectInstanceID;
class CArmedInstance;
VCMI_LIB_NAMESPACE_END

class MapAudioPlayer : public IMapObjectObserver
{
	using MapObjectsList = std::vector<ObjectInstanceID>;

	boost::multi_array<MapObjectsList, 3> objects;
	const CArmedInstance * currentSelection = nullptr;
	bool playerMakingTurn = false;
	bool enemyMakingTurn = false;
	bool audioPlaying = true;

	void addObject(const CGObjectInstance * obj);
	void removeObject(const CGObjectInstance * obj);

	std::vector<AudioPath> getAmbientSounds(const int3 & tile);
	void updateAmbientSounds();
	void updateMusic();
	void update();

protected:
	// IMapObjectObserver impl
	bool hasOngoingAnimations() override;
	void onObjectFadeIn(const CGObjectInstance * obj) override;
	void onObjectFadeOut(const CGObjectInstance * obj) override;
	void onObjectInstantAdd(const CGObjectInstance * obj) override;
	void onObjectInstantRemove(const CGObjectInstance * obj) override;

	void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;

	void onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override {}
	void onBeforeHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override {}
	void onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override {}

public:
	MapAudioPlayer();
	~MapAudioPlayer() override;

	/// Called whenever current adventure map selection changes
	void onSelectionChanged(const CArmedInstance * newSelection);

	/// Called when local player starts his turn
	void onPlayerTurnStarted();

	/// Called when AI or non-local player start his turn
	void onEnemyTurnStarted();

	/// Called when local player ends his turn
	void onPlayerTurnEnded();

	/// Called when map audio should be paused, e.g. on combat or town scren access
	void onAudioPaused();

	/// Called when map audio should be resume, opposite to onPaused
	void onAudioResumed();
};
