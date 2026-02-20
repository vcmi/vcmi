/*
 * MapAudioPlayer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapAudioPlayer.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../mapView/mapHandler.h"
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

bool MapAudioPlayer::hasOngoingAnimations()
{
	return false;
}

void MapAudioPlayer::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(obj == currentSelection)
		update();
}

void MapAudioPlayer::onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(obj == currentSelection)
		update();
}

void MapAudioPlayer::onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(obj == currentSelection)
		update();
}

void MapAudioPlayer::onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(obj == currentSelection)
		update();
}

void MapAudioPlayer::onObjectFadeIn(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	addObject(obj);
}

void MapAudioPlayer::onObjectFadeOut(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	removeObject(obj);
}

void MapAudioPlayer::onObjectInstantAdd(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	addObject(obj);
}

void MapAudioPlayer::onObjectInstantRemove(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	removeObject(obj);
}

void MapAudioPlayer::addObject(const CGObjectInstance * obj)
{
	if(obj->isTile2Terrain())
	{
		// terrain overlay - all covering tiles act as sound source
		for(int fx = 0; fx < obj->getWidth(); ++fx)
		{
			for(int fy = 0; fy < obj->getHeight(); ++fy)
			{
				int3 currTile(obj->anchorPos().x - fx, obj->anchorPos().y - fy, obj->anchorPos().z);

				if(GAME->interface()->cb->isInTheMap(currTile) && obj->coveringAt(currTile))
					objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);
			}
		}
		return;
	}

	if(obj->isVisitable())
	{
		// visitable object - visitable tile acts as sound source
		int3 currTile = obj->visitablePos();

		if(GAME->interface()->cb->isInTheMap(currTile))
			objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);

		return;
	}

	if(!obj->isVisitable())
	{
		// static object - blocking tiles act as sound source
		auto tiles = obj->getBlockedOffsets();

		for(const auto & tile : tiles)
		{
			int3 currTile = obj->anchorPos() + tile;

			if(GAME->interface()->cb->isInTheMap(currTile))
				objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);
		}
		return;
	}
}

void MapAudioPlayer::removeObject(const CGObjectInstance * obj)
{
	for(int z = 0; z < GAME->interface()->cb->getMapSize().z; z++)
		for(int x = 0; x < GAME->interface()->cb->getMapSize().x; x++)
			for(int y = 0; y < GAME->interface()->cb->getMapSize().y; y++)
				vstd::erase(objects[z][x][y], obj->id);
}

std::vector<AudioPath> MapAudioPlayer::getAmbientSounds(const int3 & tile)
{
	std::vector<AudioPath> result;

	for(auto & objectID : objects[tile.z][tile.x][tile.y])
	{
		const auto & object = GAME->map().getMap()->getObject(objectID);

		assert(object);
		if (!object)
			logGlobal->warn("Already removed object %d found on tile! (%d %d %d)", objectID.getNum(), tile.x, tile.y, tile.z);

		if(object)
		{
			auto ambientSound = object->getAmbientSound(CRandomGenerator::getDefault());
			if (ambientSound)
				result.push_back(ambientSound.value());
		}
	}

	if(GAME->map().getMap()->isCoastalTile(tile))
		result.emplace_back(AudioPath::builtin("LOOPOCEA"));

	return result;
}

void MapAudioPlayer::updateAmbientSounds()
{
	std::map<AudioPath, int> currentSounds;
	auto updateSounds = [&](const AudioPath& soundId, int distance) -> void
	{
		if(vstd::contains(currentSounds, soundId))
			currentSounds[soundId] = std::min(currentSounds[soundId], distance);
		else
			currentSounds.insert(std::make_pair(soundId, distance));
	};

	int3 pos = currentSelection->getSightCenter();
	FowTilesType tiles;
	GAME->interface()->cb->getVisibleTilesInRange(tiles, pos, ENGINE->sound().ambientGetRange(), int3::DIST_CHEBYSHEV);
	for(int3 tile : tiles)
	{
		int dist = pos.dist(tile, int3::DIST_CHEBYSHEV);

		for(auto & soundName : getAmbientSounds(tile))
			updateSounds(soundName, dist);
	}
	ENGINE->sound().ambientUpdateChannels(currentSounds);
}

void MapAudioPlayer::updateMusic()
{
	if(audioPlaying && playerMakingTurn && currentSelection)
	{
		const auto * tile = GAME->interface()->cb->getTile(currentSelection->visitablePos());

		if (tile)
			ENGINE->music().playMusicFromSet("terrain", tile->getTerrain()->getJsonKey(), true, false);
	}

	if(audioPlaying && enemyMakingTurn)
	{
		ENGINE->music().playMusicFromSet("enemy-turn", true, false);
	}
}

void MapAudioPlayer::update()
{
	updateMusic();

	if(audioPlaying && playerMakingTurn && currentSelection)
		updateAmbientSounds();
}

MapAudioPlayer::MapAudioPlayer()
{
	auto mapSize = GAME->interface()->cb->getMapSize();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	for(const auto & obj : GAME->map().getMap()->getObjects())
	{
		addObject(obj);
	}
}

MapAudioPlayer::~MapAudioPlayer()
{
	ENGINE->sound().ambientStopAllChannels();
	ENGINE->music().stopMusic(1000);
}

void MapAudioPlayer::onSelectionChanged(const CArmedInstance * newSelection)
{
	currentSelection = newSelection;
	update();
}

void MapAudioPlayer::onAudioPaused()
{
	audioPlaying = false;
	ENGINE->sound().ambientStopAllChannels();
	ENGINE->music().stopMusic(1000);
}

void MapAudioPlayer::onAudioResumed()
{
	audioPlaying = true;
	update();
}

void MapAudioPlayer::onPlayerTurnStarted()
{
	enemyMakingTurn = false;
	playerMakingTurn = true;
	update();
}

void MapAudioPlayer::onEnemyTurnStarted()
{
	playerMakingTurn = false;
	enemyMakingTurn = true;
	update();
}

void MapAudioPlayer::onPlayerTurnEnded()
{
	playerMakingTurn = false;
	enemyMakingTurn = false;
	ENGINE->sound().ambientStopAllChannels();
	ENGINE->music().stopMusic(1000);
}
