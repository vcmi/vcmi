#include "StdInc.h"
#include "scenelayer.h"
#include "mainwindow.h"
#include "../lib/mapping/CMapEditManager.h"
#include "inspector.h"
#include "mapview.h"
#include "mapcontroller.h"

AbstractLayer::AbstractLayer(MapScene * s): scene(s)
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
	
	if(show)
	{
		if(pixmap)
		{
			if(item)
				item->setPixmap(*pixmap);
			else
				item.reset(scene->addPixmap(*pixmap));
		}
		else
		{
			if(item)
				item->setPixmap(emptyPixmap);
			else
				item.reset(scene->addPixmap(emptyPixmap));
		}
	}
	else
	{
		item->setPixmap(emptyPixmap);
	}
	
	isShown = show;
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

GridLayer::GridLayer(MapScene * s): AbstractLayer(s)
{
}

void GridLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(QColor(0, 0, 0, 0));
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

PassabilityLayer::PassabilityLayer(MapScene * s): AbstractLayer(s)
{
}

void PassabilityLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(QColor(0, 0, 0, 0));
	
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

SelectionTerrainLayer::SelectionTerrainLayer(MapScene * s): AbstractLayer(s)
{
}

void SelectionTerrainLayer::update()
{
	if(!map)
		return;
	
	area.clear();
	areaAdd.clear();
	areaErase.clear();
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(QColor(0, 0, 0, 0));
	
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
}

void SelectionTerrainLayer::clear()
{
	areaErase = area;
	areaAdd.clear();
	area.clear();
}

const std::set<int3> & SelectionTerrainLayer::selection() const
{
	return area;
}

TerrainLayer::TerrainLayer(MapScene * s): AbstractLayer(s)
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
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	
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
			//TODO: fix water and roads
			handler->drawTerrainTile(painter, t.x, t.y, scene->level);
			//main->getMapHandler()->drawRiver(painter, t.x, t.y, scene->level);
			//main->getMapHandler()->drawRoad(painter, t.x, t.y, scene->level);
		}
	}
	else
	{
		for(int j = 0; j < map->height; ++j)
		{
			for(int i = 0; i < map->width; ++i)
			{
				//TODO: fix water and roads
				handler->drawTerrainTile(painter, i, j, scene->level);
				//main->getMapHandler()->drawRiver(painter, i, j, scene->level);
				//main->getMapHandler()->drawRoad(painter, i, j, scene->level);
			}
		}
	}
	
	dirty.clear();
	redraw();
}

ObjectsLayer::ObjectsLayer(MapScene * s): AbstractLayer(s)
{
}

void ObjectsLayer::update()
{
	if(!map)
		return;
	
	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(QColor(0, 0, 0, 0));
	draw(false);
}

void ObjectsLayer::draw(bool onlyDirty)
{
	if(!pixmap)
		return;
	
	if(!map)
		return;
	
	pixmap->fill(QColor(0, 0, 0, 0));
	QPainter painter(pixmap.get());
	//painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	std::set<const CGObjectInstance *> drawen;
	
	
	for(int j = 0; j < map->height; ++j)
	{
		for(int i = 0; i < map->width; ++i)
		{
			handler->drawObjects(painter, i, j, scene->level);
			/*auto & objects = main->getMapHandler()->getObjects(i, j, scene->level);
			 for(auto & object : objects)
			 {
			 if(!object.obj || drawen.count(object.obj))
			 continue;
			 
			 if(!onlyDirty || dirty.count(object.obj))
			 {
			 main->getMapHandler()->drawObject(painter, object);
			 drawen.insert(object.obj);
			 }
			 }*/
		}
	}
	
	dirty.clear();
	redraw();
}

void ObjectsLayer::setDirty(int x, int y)
{
	/*auto & objects = main->getMapHandler()->getObjects(x, y, scene->level);
	for(auto & object : objects)
	{
		if(object.obj)
			dirty.insert(object.obj);
	}*/
}

void ObjectsLayer::setDirty(const CGObjectInstance * object)
{
	dirty.insert(object);
}

SelectionObjectsLayer::SelectionObjectsLayer(MapScene * s): AbstractLayer(s), newObject(nullptr)
{
}

void SelectionObjectsLayer::update()
{
	if(!map)
		return;
	
	selectedObjects.clear();
	shift = QPoint();
	if(newObject)
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
	
	pixmap->fill(QColor(0, 0, 0, 0));
	
	QPainter painter(pixmap.get());
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.setPen(QColor(255, 255, 255));
	
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
		if(selectionMode == 2 && (shift.x() || shift.y()))
		{
			painter.setOpacity(0.5);
			auto newPos = QPoint(obj->getPosition().x, obj->getPosition().y) + shift;
			handler->drawObjectAt(painter, obj, newPos.x(), newPos.y());
		}
	}
	
	redraw();
}

CGObjectInstance * SelectionObjectsLayer::selectObjectAt(int x, int y) const
{
	if(!map || !map->isInTheMap(int3(x, y, scene->level)))
		return nullptr;
	
	auto & objects = handler->getObjects(x, y, scene->level);
	
	//visitable is most important
	for(auto & object : objects)
	{
		if(!object.obj)
			continue;
		
		if(object.obj->visitableAt(x, y))
		{
			return object.obj;
		}
	}
	
	//if not visitable tile - try to get blocked
	for(auto & object : objects)
	{
		if(!object.obj)
			continue;
		
		if(object.obj->blockingAt(x, y))
		{
			return object.obj;
		}
	}
	
	//finally, we can take any object
	for(auto & object : objects)
	{
		if(!object.obj)
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
			for(auto & o : handler->getObjects(i, j, scene->level))
				selectObject(o.obj);
		}
	}
}

void SelectionObjectsLayer::selectObject(CGObjectInstance * obj)
{
	selectedObjects.insert(obj);
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
	shift.setX(0);
	shift.setY(0);
}
