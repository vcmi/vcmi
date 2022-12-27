/*
 * scenelayer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "scenelayer.h"
#include "mainwindow.h"
#include "../lib/mapping/CMapEditManager.h"
#include "inspector/inspector.h"
#include "mapview.h"
#include "mapcontroller.h"

AbstractLayer::AbstractLayer(MapSceneBase * s): scene(s)
{
}

void AbstractLayer::initialize(MapController & controller)
{
	map = controller.map();
	handler = controller.mapHandler();
}

void AbstractLayer::show(bool show)
{
	if(isShown == show)
		return;
	
	isShown = show;
	
	redraw();
}

void AbstractLayer::redraw()
{
	if(item)
	{
		if(pixmap && isShown)
			item->setPixmap(*pixmap);
		else
			item->setPixmap(emptyPixmap);
	}
	else
	{
		if(pixmap && isShown)
			item.reset(scene->addPixmap(*pixmap));
		else
			item.reset(scene->addPixmap(emptyPixmap));
	}
}

GridLayer::GridLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void GridLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(Qt::transparent);
	QPainter painter(pixmap.get());
	painter.setPen(QColor(0, 0, 0, 190));
	
	for(int j = 0; j < map->height; ++j)
	{
		painter.drawLine(0, j * 32, map->width * 32 - 1, j * 32);
	}
	for(int i = 0; i < map->width; ++i)
	{
		painter.drawLine(i * 32, 0, i * 32, map->height * 32 - 1);
	}
	
	redraw();
}

PassabilityLayer::PassabilityLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void PassabilityLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(Qt::transparent);
	
	if(scene->level == 0 || map->twoLevel)
	{
		QPainter painter(pixmap.get());
		for(int j = 0; j < map->height; ++j)
		{
			for(int i = 0; i < map->width; ++i)
			{
				auto tl = map->getTile(int3(i, j, scene->level));
				if(tl.blocked || tl.visitable)
				{
					painter.fillRect(i * 32, j * 32, 31, 31, tl.visitable ? QColor(200, 200, 0, 64) : QColor(255, 0, 0, 64));
				}
			}
		}
	}
	
	redraw();
}

SelectionTerrainLayer::SelectionTerrainLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void SelectionTerrainLayer::update()
{
	if(!map)
		return;
	
	area.clear();
	areaAdd.clear();
	areaErase.clear();
	onSelection();
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(Qt::transparent);
	
	redraw();
}

void SelectionTerrainLayer::draw()
{
	if(!pixmap)
		return;
	
	QPainter painter(pixmap.get());
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	for(auto & t : areaAdd)
	{
		painter.fillRect(t.x * 32, t.y * 32, 31, 31, QColor(128, 128, 128, 96));
	}
	for(auto & t : areaErase)
	{
		painter.fillRect(t.x * 32, t.y * 32, 31, 31, QColor(0, 0, 0, 0));
	}
	
	areaAdd.clear();
	areaErase.clear();
	
	redraw();
}

void SelectionTerrainLayer::select(const int3 & tile)
{
	if(!map || !map->isInTheMap(tile))
		return;
	
	if(!area.count(tile))
	{
		area.insert(tile);
		areaAdd.insert(tile);
		areaErase.erase(tile);
	}
	onSelection();
}

void SelectionTerrainLayer::erase(const int3 & tile)
{
	if(!map || !map->isInTheMap(tile))
		return;
	
	if(area.count(tile))
	{
		area.erase(tile);
		areaErase.insert(tile);
		areaAdd.erase(tile);
	}
	onSelection();
}

void SelectionTerrainLayer::clear()
{
	areaErase = area;
	areaAdd.clear();
	area.clear();
	onSelection();
}

const std::set<int3> & SelectionTerrainLayer::selection() const
{
	return area;
}

void SelectionTerrainLayer::onSelection()
{
	emit selectionMade(!area.empty());
}


TerrainLayer::TerrainLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void TerrainLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	draw(false);
}

void TerrainLayer::setDirty(const int3 & tile)
{
	dirty.insert(tile);
}

void TerrainLayer::draw(bool onlyDirty)
{
	if(!pixmap)
		return;
	
	if(!map)
		return;
	
	QPainter painter(pixmap.get());
	//painter.setCompositionMode(QPainter::CompositionMode_Source);
	
	if(onlyDirty)
	{
		std::set<int3> forRedrawing(dirty), neighbours;
		for(auto & t : dirty)
		{
			for(auto & tt : int3::getDirs())
			{
				if(map->isInTheMap(t + tt))
					neighbours.insert(t + tt);
			}
		}
		for(auto & t : neighbours)
		{
			for(auto & tt : int3::getDirs())
			{
				forRedrawing.insert(t);
				if(map->isInTheMap(t + tt))
					forRedrawing.insert(t + tt);
			}
		}
		for(auto & t : forRedrawing)
		{
			handler->drawTerrainTile(painter, t.x, t.y, scene->level);
			handler->drawRiver(painter, t.x, t.y, scene->level);
			handler->drawRoad(painter, t.x, t.y, scene->level);
		}
	}
	else
	{
		for(int j = 0; j < map->height; ++j)
		{
			for(int i = 0; i < map->width; ++i)
			{
				handler->drawTerrainTile(painter, i, j, scene->level);
				handler->drawRiver(painter, i, j, scene->level);
				handler->drawRoad(painter, i, j, scene->level);
			}
		}
	}
	
	dirty.clear();
	redraw();
}

ObjectsLayer::ObjectsLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void ObjectsLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(Qt::transparent);
	draw(false);
}

void ObjectsLayer::draw(bool onlyDirty)
{
	if(!pixmap)
		return;
	
	if(!map)
		return;
	
	QPainter painter(pixmap.get());
	std::set<const CGObjectInstance *> drawen;
	
	if(onlyDirty)
	{
		//objects could be modified
		for(auto * obj : objDirty)
			setDirty(obj);
		
		//clear tiles which will be redrawn. It's needed because some object could be replaced
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		for(auto & p : dirty)
			painter.fillRect(p.x * 32, p.y * 32, 32, 32, Qt::transparent);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		
		for(auto & p : dirty)
			handler->drawObjects(painter, p.x, p.y, p.z);
	}
	else
	{
		pixmap->fill(Qt::transparent);
		for(int j = 0; j < map->height; ++j)
		{
			for(int i = 0; i < map->width; ++i)
			{
				handler->drawObjects(painter, i, j, scene->level);
			}
		}
	}
	
	dirty.clear();
	redraw();
}

void ObjectsLayer::setDirty(int x, int y)
{
	int3 pos(x, y, scene->level);
	if(map->isInTheMap(pos))
		dirty.insert(pos);
}

void ObjectsLayer::setDirty(const CGObjectInstance * object)
{
	objDirty.insert(object);
	//mark tiles under object as dirty
	for(int j = 0; j < object->getHeight(); ++j)
	{
		for(int i = 0; i < object->getWidth(); ++i)
		{
			setDirty(object->getPosition().x - i, object->getPosition().y - j);
		}
	}
}

SelectionObjectsLayer::SelectionObjectsLayer(MapSceneBase * s): AbstractLayer(s), newObject(nullptr)
{
}

void SelectionObjectsLayer::update()
{
	if(!map)
		return;
	
	selectedObjects.clear();
	onSelection();
	shift = QPoint();
	delete newObject;
	newObject = nullptr;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	//pixmap->fill(QColor(0, 0, 0, 0));
	
	draw();
}

void SelectionObjectsLayer::draw()
{
	if(!pixmap)
		return;
	
	pixmap->fill(Qt::transparent);
	
	QPainter painter(pixmap.get());
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.setPen(Qt::white);
	
	for(auto * obj : selectedObjects)
	{
		if(obj != newObject)
		{
			QRect bbox(obj->getPosition().x, obj->getPosition().y, 1, 1);
			for(auto & t : obj->getBlockedPos())
			{
				QPoint topLeft(std::min(t.x, bbox.topLeft().x()), std::min(t.y, bbox.topLeft().y()));
				bbox.setTopLeft(topLeft);
				QPoint bottomRight(std::max(t.x, bbox.bottomRight().x()), std::max(t.y, bbox.bottomRight().y()));
				bbox.setBottomRight(bottomRight);
			}
			
			painter.setOpacity(1.0);
			painter.drawRect(bbox.x() * 32, bbox.y() * 32, bbox.width() * 32, bbox.height() * 32);
		}
		
		//show translation
		if(selectionMode == SelectionMode::MOVEMENT && (shift.x() || shift.y()))
		{
			painter.setOpacity(0.7);
			auto newPos = QPoint(obj->getPosition().x, obj->getPosition().y) + shift;
			handler->drawObjectAt(painter, obj, newPos.x(), newPos.y());
		}
	}
	
	redraw();
}

CGObjectInstance * SelectionObjectsLayer::selectObjectAt(int x, int y, const CGObjectInstance * ignore) const
{
	if(!map || !map->isInTheMap(int3(x, y, scene->level)))
		return nullptr;
	
	auto & objects = handler->getObjects(x, y, scene->level);
	
	//visitable is most important
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore)
			continue;
		
		if(object.obj->visitableAt(x, y))
		{
			return object.obj;
		}
	}
	
	//if not visitable tile - try to get blocked
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore)
			continue;
		
		if(object.obj->blockingAt(x, y))
		{
			return object.obj;
		}
	}
	
	//finally, we can take any object
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore)
			continue;
		
		if(object.obj->coveringAt(x, y))
		{
			return object.obj;
		}
	}
	
	return nullptr;
}

void SelectionObjectsLayer::selectObjects(int x1, int y1, int x2, int y2)
{
	if(!map)
		return;
	
	if(x1 > x2)
		std::swap(x1, x2);
	
	if(y1 > y2)
		std::swap(y1, y2);
	
	for(int j = y1; j < y2; ++j)
	{
		for(int i = x1; i < x2; ++i)
		{
			if(map->isInTheMap(int3(i, j, scene->level)))
			{
				for(auto & o : handler->getObjects(i, j, scene->level))
					selectObject(o.obj, false); //do not inform about each object added
			}
		}
	}
	onSelection();
}

void SelectionObjectsLayer::selectObject(CGObjectInstance * obj, bool inform /* = true */)
{
	selectedObjects.insert(obj);
	if (inform)
	{
		onSelection();
	}
}

void SelectionObjectsLayer::deselectObject(CGObjectInstance * obj)
{
	selectedObjects.erase(obj);
}

bool SelectionObjectsLayer::isSelected(const CGObjectInstance * obj) const
{
	return selectedObjects.count(const_cast<CGObjectInstance*>(obj));
}

std::set<CGObjectInstance*> SelectionObjectsLayer::getSelection() const
{
	return selectedObjects;
}

void SelectionObjectsLayer::clear()
{
	selectedObjects.clear();
	onSelection();
	shift.setX(0);
	shift.setY(0);
}

void SelectionObjectsLayer::onSelection()
{
	emit selectionMade(!selectedObjects.empty());
}

MinimapLayer::MinimapLayer(MapSceneBase * s): AbstractLayer(s)
{
	
}

void MinimapLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width, map->height));
	
	QPainter painter(pixmap.get());
	//coordinate transfomation
	for(int j = 0; j < map->height; ++j)
	{
		for(int i = 0; i < map->width; ++i)
		{
			handler->drawMinimapTile(painter, i, j, scene->level);
		}
	}
	
	redraw();
}

MinimapViewLayer::MinimapViewLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void MinimapViewLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width, map->height));
	
	draw();
}

void MinimapViewLayer::draw()
{
	if(!map)
		return;
	
	pixmap->fill(Qt::transparent);
	
	//maybe not optimal but ok
	QPainter painter(pixmap.get());
	painter.setPen(Qt::white);
	painter.drawRect(x, y, w, h);
	
	redraw();
}

void MinimapViewLayer::setViewport(int _x, int _y, int _w, int _h)
{
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	draw();
}
