/*
 * mapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapHandler.h"
#include "MapRendererContext.h"
#include "MapView.h"

#include "../render/CAnimation.h"
#include "../render/CFadeAnimation.h"
#include "../render/Colors.h"
#include "../gui/CGuiHandler.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../CGameInfo.h"
#include "../render/Graphics.h"
#include "../render/IImage.h"
#include "../render/Canvas.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/Color.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/RoadHandler.h"
#include "../../lib/RiverHandler.h"
#include "../../lib/TerrainHandler.h"

/*
std::shared_ptr<IImage> CMapWorldViewBlitter::objectToIcon(Obj id, si32 subId, PlayerColor owner) const
{
	int ownerIndex = 0;
	if(owner < PlayerColor::PLAYER_LIMIT)
	{
		ownerIndex = owner.getNum() * 19;
	}
	else if (owner == PlayerColor::NEUTRAL)
	{
		ownerIndex = PlayerColor::PLAYER_LIMIT.getNum() * 19;
	}

	switch(id)
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
		return info->icons->getImage((int)EWorldViewIcon::TELEPORT);
	case Obj::SUBTERRANEAN_GATE:
		return info->icons->getImage((int)EWorldViewIcon::GATE);
	case Obj::ARTIFACT:
		return info->icons->getImage((int)EWorldViewIcon::ARTIFACT);
	case Obj::TOWN:
		return info->icons->getImage((int)EWorldViewIcon::TOWN + ownerIndex);
	case Obj::HERO:
		return info->icons->getImage((int)EWorldViewIcon::HERO + ownerIndex);
	case Obj::MINE:
		return info->icons->getImage((int)EWorldViewIcon::MINE_WOOD + subId + ownerIndex);
	case Obj::RESOURCE:
		return info->icons->getImage((int)EWorldViewIcon::RES_WOOD + subId + ownerIndex);
	}
	return std::shared_ptr<IImage>();
}
*/

/*
void CMapWorldViewBlitter::drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	auto drawIcon = [this,targetSurf](Obj id, si32 subId, PlayerColor owner)
	{
		auto wvIcon = this->objectToIcon(id, subId, owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	};

	auto & objects = tile.objects;
	for(auto & object : objects)
	{
		const CGObjectInstance * obj = object.obj;
		if(!obj)
			continue;

		const bool sameLevel = obj->pos.z == pos.z;

		//FIXME: Don't read options in  a loop :v
		const bool isVisible = settings["session"]["spectate"].Bool() ? true : (*info->visibilityMap)[pos.z][pos.x][pos.y];
		const bool isVisitable = obj->visitableAt(pos.x, pos.y);

		if(sameLevel && isVisible && isVisitable)
			drawIcon(obj->ID, obj->subID, obj->tempOwner);
	}
}
*/
/*
void CMapWorldViewBlitter::drawOverlayEx(SDL_Surface * targetSurf)
{
	if(nullptr == info->additionalIcons)
		return;

	const int3 bottomRight = pos + tileCount;

	for(const ObjectPosInfo & iconInfo : *(info->additionalIcons))
	{
		if( iconInfo.pos.z != pos.z)
			continue;

		if((iconInfo.pos.x < topTile.x) || (iconInfo.pos.y < topTile.y))
			continue;

		if((iconInfo.pos.x > bottomRight.x) || (iconInfo.pos.y > bottomRight.y))
			continue;

		realPos.x = initPos.x + (iconInfo.pos.x - topTile.x) * tileSize;
		realPos.y = initPos.y + (iconInfo.pos.y - topTile.y) * tileSize;

		auto wvIcon = this->objectToIcon(iconInfo.id, iconInfo.subId, iconInfo.owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	}
}
*/
/*
void CMapPuzzleViewBlitter::drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	CMapBlitter::drawObjects(targetSurf, tile);

	// grail X mark
	if(pos.x == info->grailPos.x && pos.y == info->grailPos.y)
	{
		const auto mark = graphics->heroMoveArrows->getImage(0);
		mark->draw(targetSurf,realTileRect.x,realTileRect.y);
	}
}
*/
/*
void CMapPuzzleViewBlitter::postProcessing(SDL_Surface * targetSurf) const
{
	CSDL_Ext::applyEffect(targetSurf, info->drawBounds, static_cast<int>(!ADVOPT.puzzleSepia));
}
*/
/*
bool CMapPuzzleViewBlitter::canDrawObject(const CGObjectInstance * obj) const
{
	if (!CMapBlitter::canDrawObject(obj))
		return false;

	//don't print flaggable objects in puzzle mode
	if (obj->isVisitable())
		return false;

	if(std::find(unblittableObjects.begin(), unblittableObjects.end(), obj->ID) != unblittableObjects.end())
		return false;

	return true;
}
*/
/*
CMapPuzzleViewBlitter::CMapPuzzleViewBlitter(CMapHandler * parent)
	: CMapNormalBlitter(parent)
{
	unblittableObjects.push_back(Obj::HOLE);
}
*/

/*
void CMapBlitter::blit(SDL_Surface * targetSurf, const MapDrawingInfo * info)
{
	init(info);
	auto prevClip = clip(targetSurf);

	pos = int3(0, 0, topTile.z);

	for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
	{
		if (pos.x < 0 || pos.x >= parent->sizes.x)
			continue;

		for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
		{
			if (pos.y < 0 || pos.y >= parent->sizes.y)
				continue;

			const bool isVisible = canDrawCurrentTile();

			realTileRect.x = realPos.x;
			realTileRect.y = realPos.y;

			const TerrainTile2 & tile = parent->ttiles[pos.z][pos.x][pos.y];
			const TerrainTile & tinfo = parent->map->getTile(pos);
			const TerrainTile * tinfoUpper = pos.y > 0 ? &parent->map->getTile(int3(pos.x, pos.y - 1, pos.z)) : nullptr;

			if(isVisible || info->showAllTerrain)
			{
				drawTileTerrain(targetSurf, tinfo, tile);
				if(tinfo.riverType->getId() != River::NO_RIVER)
					drawRiver(targetSurf, tinfo);
				drawRoad(targetSurf, tinfo, tinfoUpper);
			}

			if(isVisible)
				drawObjects(targetSurf, tile);
		}
	}

	for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
	{
		for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
		{
			realTileRect.x = realPos.x;
			realTileRect.y = realPos.y;

			if (pos.x < 0 || pos.x >= parent->sizes.x ||
				pos.y < 0 || pos.y >= parent->sizes.y)
			{
				drawFrame(targetSurf);
			}
			else
			{
				const TerrainTile2 & tile = parent->ttiles[pos.z][pos.x][pos.y];

				if(!settings["session"]["spectate"].Bool() && !(*info->visibilityMap)[topTile.z][pos.x][pos.y] && !info->showAllTerrain)
					drawFow(targetSurf);

				// overlay needs to be drawn over fow, because of artifacts-aura-like spells
				drawTileOverlay(targetSurf, tile);

				// drawDebugVisitables()
				if (settings["session"]["showBlock"].Bool())
				{
					if(parent->map->getTile(int3(pos.x, pos.y, pos.z)).blocked) //temporary hiding blocked positions
					{
						static std::shared_ptr<IImage> block;
						if (!block)
							block = IImage::createFromFile("blocked");

						block->draw(targetSurf, realTileRect.x, realTileRect.y);
					}
				}
				if (settings["session"]["showVisit"].Bool())
				{
					if(parent->map->getTile(int3(pos.x, pos.y, pos.z)).visitable) //temporary hiding visitable positions
					{
						static std::shared_ptr<IImage> visit;
						if (!visit)
							visit = IImage::createFromFile("visitable");

						visit->draw(targetSurf, realTileRect.x, realTileRect.y);
					}
				}
			}
		}
	}

	drawOverlayEx(targetSurf);

	// drawDebugGrid()
	if (settings["gameTweaks"]["showGrid"].Bool())
	{
		for (realPos.x = initPos.x, pos.x = topTile.x; pos.x < topTile.x + tileCount.x; pos.x++, realPos.x += tileSize)
		{
			for (realPos.y = initPos.y, pos.y = topTile.y; pos.y < topTile.y + tileCount.y; pos.y++, realPos.y += tileSize)
			{
				constexpr ColorRGBA color(0x55, 0x55, 0x55);

				if (realPos.y >= info->drawBounds.y &&
					realPos.y < info->drawBounds.y + info->drawBounds.h)
					for(int i = 0; i < tileSize; i++)
						if (realPos.x + i >= info->drawBounds.x &&
							realPos.x + i < info->drawBounds.x + info->drawBounds.w)
							CSDL_Ext::putPixelWithoutRefresh(targetSurf, realPos.x + i, realPos.y, color.r, color.g, color.b);

				if (realPos.x >= info->drawBounds.x &&
					realPos.x < info->drawBounds.x + info->drawBounds.w)
					for(int i = 0; i < tileSize; i++)
						if (realPos.y + i >= info->drawBounds.y &&
							realPos.y + i < info->drawBounds.y + info->drawBounds.h)
							CSDL_Ext::putPixelWithoutRefresh(targetSurf, realPos.x, realPos.y + i, color.r, color.g, color.b);
			}
		}
	}

	postProcessing(targetSurf);

	CSDL_Ext::setClipRect(targetSurf, prevClip);
}
*/
/*
bool CMapHandler::updateObjectsFade()
{
	for (auto iter = fadeAnims.begin(); iter != fadeAnims.end(); )
	{
		int3 pos = (*iter).second.first;
		CFadeAnimation * anim = (*iter).second.second;

		anim->update();

		if (anim->isFading())
			++iter;
		else // fade finished
		{
			auto &objs = ttiles[pos.z][pos.x][pos.y].objects;
			for (auto objIter = objs.begin(); objIter != objs.end(); ++objIter)
			{
				if ((*objIter).fadeAnimKey == (*iter).first)
				{
					logAnim->trace("Fade anim finished for obj at %s; remaining: %d", pos.toString(), fadeAnims.size() - 1);
					if (anim->fadingMode == CFadeAnimation::EMode::OUT)
						objs.erase(objIter); // if this was fadeout, remove the object from the map
					else
						(*objIter).fadeAnimKey = -1; // for fadein, just remove its connection to the finished fade
					break;
				}
			}
			delete (*iter).second.second;
			iter = fadeAnims.erase(iter);
		}
	}

	return !fadeAnims.empty();
}
*/
/*
bool CMapHandler::startObjectFade(TerrainTileObject & obj, bool in, int3 pos)
{
	SDL_Surface * fadeBitmap;
	assert(obj.obj);

	auto objData = normalBlitter->findObjectBitmap(obj.obj, 0);
	if (objData.objBitmap)
	{
		if (objData.isMoving) // ignore fading of moving objects (for now?)
		{
			logAnim->debug("Ignoring fade of moving object");
			return false;
		}

		fadeBitmap = CSDL_Ext::newSurface(32, 32); // TODO cache these bitmaps instead of creating new ones?
		Rect objSrcRect(obj.rect.x, obj.rect.y, 32, 32);
		objData.objBitmap->draw(fadeBitmap,0,0,&objSrcRect);

		if (objData.flagBitmap)
		{
			if (obj.obj->pos.x - 1 == pos.x && obj.obj->pos.y - 1 == pos.y) // -1 to draw flag in top-center instead of right-bottom; kind of a hack
			{
				Rect flagSrcRect(32, 0, 32, 32);
				objData.flagBitmap->draw(fadeBitmap,0,0, &flagSrcRect);
			}
		}
		auto anim = new CFadeAnimation();
		anim->init(in ? CFadeAnimation::EMode::IN : CFadeAnimation::EMode::OUT, fadeBitmap, true);
		fadeAnims[++fadeAnimCounter] = std::pair<int3, CFadeAnimation*>(pos, anim);
		obj.fadeAnimKey = fadeAnimCounter;

		logAnim->trace("Fade anim started for obj %d at %s; anim count: %d", obj.obj->ID, pos.toString(), fadeAnims.size());
		return true;
	}

	return false;
}
*/
/*
bool CMapHandler::printObject(const CGObjectInstance * obj, bool fadein)
{
	auto animation = graphics->getAnimation(obj);

	if(!animation)
		return false;

	auto bitmap = animation->getImage(0);

	if(!bitmap)
		return false;

	const int tilesW = bitmap->width()/32;
	const int tilesH = bitmap->height()/32;

	auto ttilesWidth = ttiles.shape()[1];
	auto ttilesHeight = ttiles.shape()[2];

	for(int fx=0; fx<tilesW; ++fx)
	{
		for(int fy=0; fy<tilesH; ++fy)
		{
			Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;

			if((obj->pos.x + fx - tilesW + 1) >= 0 &&
				(obj->pos.x + fx - tilesW + 1) < ttilesWidth - frameW &&
				(obj->pos.y + fy - tilesH + 1) >= 0 &&
				(obj->pos.y + fy - tilesH + 1) < ttilesHeight - frameH)
			{
				int3 pos(obj->pos.x + fx - tilesW + 1, obj->pos.y + fy - tilesH + 1, obj->pos.z);
				TerrainTile2 & curt = ttiles[pos.z][pos.x][pos.y];

				TerrainTileObject toAdd(obj, cr, obj->visitableAt(pos.x, pos.y));
				if (fadein && ADVOPT.objectFading)
				{
					startObjectFade(toAdd, true, pos);
				}

				auto i = curt.objects.begin();
				for(; i != curt.objects.end(); i++)
				{
					if(objectBlitOrderSorter(toAdd, *i))
					{
						curt.objects.insert(i, toAdd);
						i = curt.objects.begin(); //to validate and avoid adding it second time
						break;
					}
				}

				if(i == curt.objects.end())
					curt.objects.insert(i, toAdd);
			}
		}
	}
	return true;
}
*/
/*
bool CMapHandler::hideObject(const CGObjectInstance * obj, bool fadeout)
{
	for(size_t z = 0; z < map->levels(); z++)
	{
		for(size_t x = 0; x < map->width; x++)
		{
			for(size_t y = 0; y < map->height; y++)
			{
				auto &objs = ttiles[(int)z][(int)x][(int)y].objects;
				for(size_t i = 0; i < objs.size(); i++)
				{
					if (objs[i].obj && objs[i].obj->id == obj->id)
					{
						if (fadeout && ADVOPT.objectFading) // object should be faded == erase is delayed until the end of fadeout
						{
							if (startObjectFade(objs[i], false, int3((si32)x, (si32)y, (si32)z)))
								objs[i].obj = nullptr;
							else
								objs.erase(objs.begin() + i);
						}
						else
							objs.erase(objs.begin() + i);
						break;
					}
				}
			}
		}
	}

	return true;
}
*/

bool CMapHandler::hasActiveAnimations()
{
	return false; // !fadeAnims.empty(); // don't allow movement during fade animation
}
/*
void CMapHandler::updateWater() //shift colors in palettes of water tiles
{
	for(auto & elem : terrainImages["lava"])
	{
		for(auto img : elem)
			img->shiftPalette(246, 9);
	}

	for(auto & elem : terrainImages["water"])
	{
		for(auto img : elem)
		{
			img->shiftPalette(229, 12);
			img->shiftPalette(242, 14);
		}
	}

	for(auto & elem : riverImages["clrrvr"])
	{
		for(auto img : elem)
		{
			img->shiftPalette(183, 12);
			img->shiftPalette(195, 6);
		}
	}

	for(auto & elem : riverImages["mudrvr"])
	{
		for(auto img : elem)
		{
			img->shiftPalette(228, 12);
			img->shiftPalette(183, 6);
			img->shiftPalette(240, 6);
		}
	}

	for(auto & elem : riverImages["lavrvr"])
	{
		for(auto img : elem)
			img->shiftPalette(240, 9);
	}
}
*/

bool CMapHandler::hasObjectHole(const int3 & pos) const
{
	//const TerrainTile2 & tt = ttiles[pos.z][pos.x][pos.y];

	//for(auto & elem : tt.objects)
	//{
	//	if(elem.obj && elem.obj->ID == Obj::HOLE)
	//		return true;
	//}
	return false;
}

void CMapHandler::getTerrainDescr(const int3 & pos, std::string & out, bool isRMB) const
{
	const TerrainTile & t = map->getTile(pos);

	if(t.hasFavorableWinds())
	{
		out = CGI->objtypeh->getObjectName(Obj::FAVORABLE_WINDS, 0);
		return;
	}
	//const TerrainTile2 & tt = ttiles[pos.z][pos.x][pos.y];
	bool isTile2Terrain = false;
	out.clear();

	//for(auto & elem : tt.objects)
	//{
	//	if(elem.obj)
	//	{
	//		out = elem.obj->getObjectName();
	//		if(elem.obj->ID == Obj::HOLE)
	//			return;

	//		isTile2Terrain = elem.obj->isTile2Terrain();
	//		break;
	//	}
	//}

	if(!isTile2Terrain || out.empty())
		out = t.terType->getNameTranslated();

	if(t.getDiggingStatus(false) == EDiggingStatus::CAN_DIG)
	{
		out = boost::str(boost::format(isRMB ? "%s\r\n%s" : "%s %s") // New line for the Message Box, space for the Status Bar
			% out 
			% CGI->generaltexth->allTexts[330]); // 'digging ok'
	}
}

bool CMapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
	if (!a)
		return true;
	if (!b)
		return false;
	if (a->appearance->printPriority != b->appearance->printPriority)
		return a->appearance->printPriority > b->appearance->printPriority;

	if(a->pos.y != b->pos.y)
		return a->pos.y < b->pos.y;

	if(b->ID==Obj::HERO && a->ID!=Obj::HERO)
		return true;
	if(b->ID!=Obj::HERO && a->ID==Obj::HERO)
		return false;

	if(!a->isVisitable() && b->isVisitable())
		return true;
	if(!b->isVisitable() && a->isVisitable())
		return false;
	if(a->pos.x < b->pos.x)
		return true;
	return false;
}

TerrainTileObject::TerrainTileObject(const CGObjectInstance * obj_, Rect rect_, bool visitablePos)
	: obj(obj_),
	  rect(rect_),
	  fadeAnimKey(-1)
{
	// We store information about ambient sound is here because object might disappear while sound is updating
	if(obj->getAmbientSound())
	{
		// All tiles of static objects are sound sources. E.g Volcanos and special terrains
		// For visitable object only their visitable tile is sound source
		if(!CCS->soundh->ambientCheckVisitable() || !obj->isVisitable() || visitablePos)
			ambientSound = obj->getAmbientSound();
	}
}

TerrainTileObject::~TerrainTileObject()
{

}

CMapHandler::CMapHandler(const CMap * map)
	: map(map)
{
}

const CMap * CMapHandler::getMap()
{
	return map;
}

bool CMapHandler::isInMap( const int3 & tile)
{
	return map->isInTheMap(tile);
}

std::vector<std::string> CMapHandler::getAmbientSounds(const int3 & tile)
{
	std::vector<std::string> result;

	//for(auto & ttObj : ttiles[tile.z][tile.x][tile.y].objects)
	//{
	//	if(ttObj.ambientSound)
	//		result.push_back(ttObj.ambientSound.get());
	//}
	if(map->isCoastalTile(tile))
		result.emplace_back("LOOPOCEA");

	return result;
}

void CMapHandler::onObjectFadeIn(const CGObjectInstance * obj)
{
	onObjectInstantAdd(obj);
}

void CMapHandler::onObjectFadeOut(const CGObjectInstance * obj)
{
	onObjectInstantRemove(obj);
}

void CMapHandler::onObjectInstantAdd(const CGObjectInstance * obj)
{
	MapRendererContext context;
	for (auto * observer : observers)
		observer->onObjectInstantAdd(context, obj);
}

void CMapHandler::onObjectInstantRemove(const CGObjectInstance * obj)
{
	MapRendererContext context;
	for (auto * observer : observers)
		observer->onObjectInstantRemove(context, obj);
}

void CMapHandler::onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	onObjectInstantRemove(obj);
	onObjectInstantAdd(obj);
}

void CMapHandler::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	onObjectInstantRemove(obj);
	onObjectInstantAdd(obj);
}

void CMapHandler::onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	//TODO
}

void CMapHandler::addMapObserver(IMapObjectObserver * object)
{
	observers.push_back(object);
}

void CMapHandler::removeMapObserver(IMapObjectObserver * object)
{
	vstd::erase(observers, object);
}

IMapObjectObserver::IMapObjectObserver()
{
	CGI->mh->addMapObserver(this);
}

IMapObjectObserver::~IMapObjectObserver()
{
	CGI->mh->removeMapObserver(this);
}
