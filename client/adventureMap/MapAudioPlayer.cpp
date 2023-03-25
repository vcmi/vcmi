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

#include "../CCallback.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../mapView/mapHandler.h"

#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CArmedInstance.h"
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

void MapAudioPlayer::onObjectFadeIn(const CGObjectInstance * obj)
{
	addObject(obj);
}

void MapAudioPlayer::onObjectFadeOut(const CGObjectInstance * obj)
{
	removeObject(obj);
}

void MapAudioPlayer::onObjectInstantAdd(const CGObjectInstance * obj)
{
	addObject(obj);
}

void MapAudioPlayer::onObjectInstantRemove(const CGObjectInstance * obj)
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
				int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);

				if(LOCPLINT->cb->isInTheMap(currTile) && obj->coveringAt(currTile.x, currTile.y))
					objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);
			}
		}
		return;
	}

	if(obj->isVisitable())
	{
		// visitable object - visitable tile acts as sound source
		int3 currTile = obj->visitablePos();

		if(LOCPLINT->cb->isInTheMap(currTile))
			objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);

		return;
	}

	if(!obj->isVisitable())
	{
		// static object - blocking tiles act as sound source
		auto tiles = obj->getBlockedOffsets();

		for(const auto & tile : tiles)
		{
			int3 currTile = obj->pos + tile;

			if(LOCPLINT->cb->isInTheMap(currTile))
				objects[currTile.z][currTile.x][currTile.y].push_back(obj->id);
		}
		return;
	}
}

void MapAudioPlayer::removeObject(const CGObjectInstance * obj)
{
	for(int z = 0; z < LOCPLINT->cb->getMapSize().z; z++)
		for(int x = 0; x < LOCPLINT->cb->getMapSize().x; x++)
			for(int y = 0; y < LOCPLINT->cb->getMapSize().y; y++)
				vstd::erase(objects[z][x][y], obj->id);
}

std::vector<std::string> MapAudioPlayer::getAmbientSounds(const int3 & tile)
{
	std::vector<std::string> result;

	for(auto & objectID : objects[tile.z][tile.x][tile.y])
	{
		const auto & object = CGI->mh->getMap()->objects[objectID.getNum()];

		assert(object);
		if (!object)
			logGlobal->warn("Already removed object %d found on tile! (%d %d %d)", objectID.getNum(), tile.x, tile.y, tile.z);

		if(object && object->getAmbientSound())
			result.push_back(object->getAmbientSound().get());
	}

	if(CGI->mh->getMap()->isCoastalTile(tile))
		result.emplace_back("LOOPOCEA");

	return result;
}

void MapAudioPlayer::updateAmbientSounds()
{
	std::map<std::string, int> currentSounds;
	auto updateSounds = [&](const std::string& soundId, int distance) -> void
	{
		if(vstd::contains(currentSounds, soundId))
			currentSounds[soundId] = std::min(currentSounds[soundId], distance);
		else
			currentSounds.insert(std::make_pair(soundId, distance));
	};

	int3 pos = currentSelection->getSightCenter();
	std::unordered_set<int3, ShashInt3> tiles;
	LOCPLINT->cb->getVisibleTilesInRange(tiles, pos, CCS->soundh->ambientGetRange(), int3::DIST_CHEBYSHEV);
	for(int3 tile : tiles)
	{
		int dist = pos.dist(tile, int3::DIST_CHEBYSHEV);

		for(auto & soundName : getAmbientSounds(tile))
			updateSounds(soundName, dist);
	}
	CCS->soundh->ambientUpdateChannels(currentSounds);
}

void MapAudioPlayer::updateMusic()
{
	if(audioPlaying && playerMakingTurn && currentSelection)
	{
		const auto * terrain = LOCPLINT->cb->getTile(currentSelection->visitablePos())->terType;
		CCS->musich->playMusicFromSet("terrain", terrain->getJsonKey(), true, false);
	}

	if(audioPlaying && enemyMakingTurn)
	{
		CCS->musich->playMusicFromSet("enemy-turn", true, false);
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
	auto mapSize = LOCPLINT->cb->getMapSize();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	for(const auto & obj : CGI->mh->getMap()->objects)
	{
		if (obj)
			addObject(obj);
	}
}

MapAudioPlayer::~MapAudioPlayer()
{
	CCS->soundh->ambientStopAllChannels();
	CCS->musich->stopMusic(1000);
}

void MapAudioPlayer::onSelectionChanged(const CArmedInstance * newSelection)
{
	currentSelection = newSelection;
	update();
}

void MapAudioPlayer::onAudioPaused()
{
	audioPlaying = false;
	CCS->soundh->ambientStopAllChannels();
	CCS->musich->stopMusic(1000);
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
	CCS->soundh->ambientStopAllChannels();
	CCS->musich->stopMusic(1000);
}
