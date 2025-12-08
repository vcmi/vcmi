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
#include "../lib/mapping/CMap.h"
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
	isShown = show;

	redraw();
}


int AbstractLayer::mapWidthPx() const
{
	return map ? map->width * tileSize : 0;
}

int AbstractLayer::mapHeightPx() const
{
	return map ? map->height * tileSize : 0;
}

int AbstractLayer::toInt(double value) const
{
	return static_cast<int>(std::round(value));	// is rounded explicitly in order to avoid rounding down unprecise double values
}

AbstractFixedLayer::AbstractFixedLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void AbstractFixedLayer::redraw()
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

AbstractViewportLayer::AbstractViewportLayer(MapSceneBase * s): AbstractLayer(s)
{
}

void AbstractViewportLayer::createLayer()
{
	QList<QGraphicsItem *>emptyList;
	items.reset(scene->createItemGroup(emptyList));
}

void AbstractViewportLayer::setViewport(const QRectF & viewPort)
{
	if(!map)
		return;
	if (items->boundingRect().contains(viewPort))
		return;

	std::vector<QGraphicsItem *> outOfScreenSectors;
	for (QGraphicsItem * sector : getAllSectors())
	{
		if (!viewPort.intersects(sector->sceneBoundingRect()))
			outOfScreenSectors.push_back(sector);
	}
	for (QGraphicsItem * sector : outOfScreenSectors)
	{
		removeSector(sector);
	}

	std::vector<QRectF> newAreas;

	int left = toInt(viewPort.left());
	int right = toInt(viewPort.right());
	int top = toInt(viewPort.top());
	int bottom = toInt(viewPort.bottom());
	int startX = left - (left % sectorSize);
	int limitX = std::min(right + (sectorSize - right % sectorSize), mapWidthPx());
	int startY = top - (top % sectorSize);
	int limitY = std::min(bottom + (sectorSize - bottom % sectorSize), mapHeightPx());

	for (int x = startX; x < limitX; x += sectorSize)
	{
		for (int y = startY; y < limitY; y += sectorSize)
		{
			int width = x + sectorSize < limitX ? sectorSize : limitX - x;
			int height = y + sectorSize < limitY ? sectorSize : limitY - y;
			QRectF area(x, y, width, height);
			if (!items->boundingRect().intersects(area))
				newAreas.emplace_back(area);
		}
	}

	for(QRectF newSection : newAreas)
	{
		QGraphicsItem * sector = draw(newSection);
		if (sector)
			addSector(sector);
	}
}

void AbstractViewportLayer::update()
{
	redraw();
}

void AbstractViewportLayer::redraw()
{
	std::set<QGraphicsItem *> allSectors;
	for (auto * sector : getAllSectors())
		allSectors.insert(sector);
	redrawSectors(allSectors);
}

void AbstractViewportLayer::redraw(const std::vector<int3> & tiles)
{
	std::set<QGraphicsItem *> sectorsToRedraw = getContainingSectors(tiles);
	redrawSectors(sectorsToRedraw);
}

void AbstractViewportLayer::redrawWithSurroundingTiles(const std::vector<int3> & tiles)
{
	int maxX = 0;
	int maxY = 0;
	int minX = INT_MAX;
	int minY = INT_MAX;
	for (const int3 tile : tiles)
	{
		maxX = std::max(tile.x, maxX);
		maxY = std::max(tile.y, maxY);
		minX = std::min(tile.x, minX);
		minY = std::min(tile.y, minY);
	}

	QRectF bounds((minX - 2) * tileSize, (minY - 2) * tileSize, (maxX - minX + 4) * tileSize, (maxY - minY + 4) * tileSize);	//tiles start with 1, QRectF from 0
	redraw({bounds});
}

void AbstractViewportLayer::redraw(const std::set<CGObjectInstance *> & objects)
{
	std::vector<QRectF> areas;
	areas.reserve(objects.size());
	for (const CGObjectInstance * object : objects)
	{
		areas.push_back(getObjectArea(object));
	}
	redraw(areas);
}

void AbstractViewportLayer::redraw(const std::vector<QRectF> & areas)
{
	std::set<QGraphicsItem *> intersectingSectors;
	for (QGraphicsItem * existingSector : getAllSectors())
	{
		for (auto area : areas)
		{
			if (existingSector->sceneBoundingRect().intersects(area))
			{
				intersectingSectors.insert(existingSector);
			}
		}
	}
	redrawSectors(intersectingSectors);
}

QRectF AbstractViewportLayer::getObjectArea(const CGObjectInstance * object) const
{
	auto pos = object->pos;
	int x = ((pos.x + 1) * tileSize) - (object->getWidth() * tileSize);	//Qt set 0,0 point on the top right corner, CGObjectInstance on the bottom left
	int y = ((pos.y + 1) * tileSize) - (object->getHeight() * tileSize);
	QRectF objectArea(x, y, object->getWidth() * tileSize, object->getHeight() * tileSize);
	return objectArea;
}

void AbstractViewportLayer::addSector(QGraphicsItem * sector)
{
	items->addToGroup(sector);
}

void AbstractViewportLayer::removeSector(QGraphicsItem * sector)
{
	items->removeFromGroup(sector);
	delete sector;
}

void AbstractViewportLayer::redrawSectors(std::set<QGraphicsItem *> & sectors)
{
	std::set<QGraphicsItem *> sectorsToRemove;

	for (QGraphicsItem * existingSectors : getAllSectors())
	{
		for (QGraphicsItem * sector : sectors)
		{
			if (existingSectors->sceneBoundingRect().contains(sector->sceneBoundingRect()))
				sectorsToRemove.insert(existingSectors);
		}
	}
	for (QGraphicsItem * sectorToRemove : sectorsToRemove)
	{
		addSector(draw(sectorToRemove->sceneBoundingRect()));
		removeSector(sectorToRemove);
	}
}

const QList<QGraphicsItem *> AbstractViewportLayer::getAllSectors() const	//returning const is necessary to avoid "range-loop might detach Qt container" problem
{
	QList<QGraphicsItem *> emptyList;
	return items ? items->childItems() : emptyList;
}

std::set<QGraphicsItem *> AbstractViewportLayer::getContainingSectors(const std::vector<int3> & tiles) const
{
	std::set<QGraphicsItem *> result;
	for (QGraphicsItem * existingSector : getAllSectors()) {
		for (const int3 tile : tiles)
		{
			if (existingSector->sceneBoundingRect().contains(QPointF(tile.x * tileSize, tile.y * tileSize)))
			{
				result.insert(existingSector);
				break;
			}
		}
	}
	return result;
}

std::set<QGraphicsItem *> AbstractViewportLayer::getIntersectingSectors(const std::vector<QRectF> & areas) const
{
	std::set<QGraphicsItem *> result;
	for (QGraphicsItem * existingSector : getAllSectors()) {
		for (QRectF area : areas)
		{
			if (existingSector->sceneBoundingRect().intersects(area))
			{
				result.insert(existingSector);
			}
		}
	}
	return result;
}

EmptyLayer::EmptyLayer(MapSceneBase * s): AbstractFixedLayer(s)
{
	isShown = true;
}

void EmptyLayer::update()
{
	if(!map)
		return;

	pixmap = std::make_unique<QPixmap>(map->width * 32, map->height * 32);
	redraw();
}

GridLayer::GridLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

QGraphicsItem * GridLayer::draw(const QRectF & section)
{
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);
	if (isShown)
	{
		QPainter painter(&pixmap);
		painter.setPen(QColor(0, 0, 0, 190));

		for(int j = 0; j <= pixmap.height(); j += tileSize)
		{
			painter.drawLine(0, j, pixmap.width(), j);
		}
		for(int i = 0; i <= pixmap.width(); i += tileSize)
		{
			painter.drawLine(i, 0, i, pixmap.height());
		}
	}

	QGraphicsItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

PassabilityLayer::PassabilityLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

QGraphicsItem * PassabilityLayer::draw(const QRectF & section)
{

	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	if(isShown)
	{
		QPainter painter(&pixmap);
		for(int j = 0; j <= pixmap.height(); j += tileSize)
		{
			for(int i = 0; i < pixmap.width(); i += tileSize)
			{
				auto tl = map->getTile(int3(toInt(section.x())/tileSize + i/tileSize, toInt(section.y())/tileSize + j/tileSize, scene->level));
				if(tl.blocked() || tl.visitable())
				{
					painter.fillRect(i, j, 31, 31, tl.visitable() ? QColor(200, 200, 0, 64) : QColor(255, 0, 0, 64));
				}
			}
		}
	}

	QGraphicsItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

ObjectPickerLayer::ObjectPickerLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

void ObjectPickerLayer::highlight(const std::function<bool(const CGObjectInstance *)> & predicate)
{
	if(!map)
		return;
	
	for(int j = 0; j < map->height; ++j)
	{
		for(int i = 0; i < map->width; ++i)
		{
			auto tl = map->getTile(int3(i, j, scene->level));
			ObjectInstanceID objID = tl.topVisitableObj();
			if(!objID.hasValue() && !tl.blockingObjects.empty())
				objID = tl.blockingObjects.front();

			if (objID.hasValue())
			{
				const CGObjectInstance * obj = map->getObject(objID);
			
				if(obj && predicate(obj))
					possibleObjects.insert(obj);
			}
		}
	}
	
	isActive = true;
}

bool ObjectPickerLayer::isVisible() const
{
	return isShown && isActive;
}

void ObjectPickerLayer::clear()
{
	possibleObjects.clear();
	isActive = false;
}

QGraphicsItem * ObjectPickerLayer::draw(const QRectF & section)
{

	int offsetX = toInt(section.x());
	int offsetY = toInt(section.y());
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	if(isVisible())
		pixmap.fill(QColor(255, 255, 255, 128));


	QPainter painter(&pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	for(const auto * obj : possibleObjects)
	{
		if(obj->pos.z != scene->level)
			continue;

		for(const auto & pos : obj->getBlockedPos())
			painter.fillRect(pos.x * tileSize - offsetX, pos.y * tileSize - offsetY, tileSize, tileSize, QColor(255, 211, 0, 64));
	}

	QGraphicsItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

void ObjectPickerLayer::select(const CGObjectInstance * obj)
{
	if(obj && possibleObjects.count(obj))
	{
		clear();
		Q_EMIT selectionMade(obj);
	}
}

void ObjectPickerLayer::discard()
{
	clear();
	Q_EMIT selectionMade(nullptr);
}

SelectionTerrainLayer::SelectionTerrainLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

QGraphicsItem * SelectionTerrainLayer::draw(const QRectF & section)
{
	int offsetX = toInt(section.x());
	int offsetY = toInt(section.y());
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	painter.setCompositionMode(QPainter::CompositionMode_Source);
	for(const auto & t : area)
	{
		if(section.contains(t.x * tileSize, t.y * tileSize))
			painter.fillRect(t.x * tileSize - offsetX, t.y * tileSize - offsetY, 31, 31, QColor(128, 128, 128, 96));
	}

	QGraphicsPixmapItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

void SelectionTerrainLayer::select(const std::vector<int3> & tiles)
{
	for (int3 tile : tiles)
	{
		if(!area.count(tile) && map->isInTheMap(tile))
			area.insert(tile);
	}
	redraw(tiles);
	onSelection();
}

void SelectionTerrainLayer::erase(const std::vector<int3> & tiles)
{
	for (int3 tile : tiles)
	{
		if(area.count(tile))
		{
			area.erase(tile);
		}
	}
	redraw(tiles);
	onSelection();
}

void SelectionTerrainLayer::clear()
{
	area.clear();
	onSelection();
	redraw();
}

const std::set<int3> & SelectionTerrainLayer::selection() const
{
	return area;
}

void SelectionTerrainLayer::onSelection()
{
	 Q_EMIT selectionMade(!area.empty());
}


TerrainLayer::TerrainLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

void TerrainLayer::redrawTerrain(const std::vector<int3> & tiles)
{
	redrawWithSurroundingTiles(tiles);
}

QGraphicsItem * TerrainLayer::draw(const QRectF & section)
{
	int left = toInt(section.left());
	int right = toInt(section.right());
	int top = toInt(section.top());
	int bottom = toInt(section.bottom());
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	QPointF offset = section.topLeft();

	for(int x = left/tileSize; x < right/tileSize; ++x)
	{
		for(int y = top/tileSize; y < bottom/tileSize; ++y)
		{
			handler->drawTerrainTile(painter, x, y, scene->level, offset);
			handler->drawRiver(painter, x, y, scene->level, offset);
			handler->drawRoad(painter, x, y, scene->level, offset);
		}
	}

	QGraphicsPixmapItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

ObjectsLayer::ObjectsLayer(MapSceneBase * s): AbstractViewportLayer(s)
{
}

QGraphicsItem * ObjectsLayer::draw(const QRectF & section)
{
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	if (isShown)
	{
		QPainter painter(&pixmap);
		handler->drawObjects(painter, section, scene->level, lockedObjects);
	}

	QGraphicsPixmapItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

void ObjectsLayer::redrawObjects(const std::set<CGObjectInstance *> & objects)
{
	redraw(objects);
}

void ObjectsLayer::setLockObject(const CGObjectInstance * object, bool lock)
{
	if(lock)
		lockedObjects.insert(object);
	else
		lockedObjects.erase(object);
	QRectF area = getObjectArea(object);
	redraw({area});
}

void ObjectsLayer::unlockAll()
{
	lockedObjects.clear();
	redraw();
}

SelectionObjectsLayer::SelectionObjectsLayer(MapSceneBase * s): AbstractViewportLayer(s), newObject(nullptr)
{
}


QGraphicsItem * SelectionObjectsLayer::draw(const QRectF & section)
{
	QPixmap pixmap(toInt(section.width()), toInt(section.height()));
	pixmap.fill(Qt::transparent);

	if (isShown)
	{
		QPainter painter(&pixmap);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.setPen(Qt::white);

		QPointF offset = section.topLeft();

		for(auto * obj : selectedObjects)
		{
			auto objectArea = getObjectArea(obj);
			if(obj != newObject.get() && section.intersects(objectArea))
			{
				auto pos = obj->anchorPos();
				QRectF bbox(pos.x, pos.y, 1, 1);
				for(const auto & t : obj->getBlockedPos())
				{
					QPointF topLeft(std::min(t.x * 1.0, bbox.topLeft().x()), std::min(t.y * 1.0, bbox.topLeft().y()));
					bbox.setTopLeft(topLeft);
					QPointF bottomRight(std::max(t.x * 1.0, bbox.bottomRight().x()), std::max(t.y * 1.0, bbox.bottomRight().y()));
					bbox.setBottomRight(bottomRight);
				}
				//selection box's size was decreased by 1 px to get rid of a persistent bug
				//with displaying a box on a border of two sectors. Bite me.

				painter.setOpacity(1.0);
				QRectF rect((bbox.x() * tileSize + 1) - offset.x(), (bbox.y() * tileSize + 1) - offset.y(), (bbox.width() * tileSize) - 2, (bbox.height() * tileSize) - 2);
				painter.drawRect(rect);
			}

			if(selectionMode == SelectionMode::MOVEMENT && (shift.x() || shift.y()))
			{
				objectArea.moveTo(objectArea.topLeft() + (shift * tileSize));
				if (section.intersects(objectArea))
				{
					painter.setOpacity(0.7);
					auto newPos = QPoint(obj->anchorPos().x, obj->anchorPos().y) + shift;
					handler->drawObjectAt(painter, obj, newPos.x(), newPos.y(), offset);
				}
			}
		}
	}

	QGraphicsPixmapItem * result = scene->addPixmap(pixmap);
	result->setPos(section.x(), section.y());

	return result;
}

CGObjectInstance * SelectionObjectsLayer::selectObjectAt(int x, int y, const CGObjectInstance * ignore) const
{
	if(!map || !map->isInTheMap(int3(x, y, scene->level)))
		return nullptr;
	
	auto & objects = handler->getObjects(x, y, scene->level);
	
	//visitable is most important
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore || lockedObjects.count(object.obj))
			continue;
		
		if(object.obj->visitableAt(int3(x, y, scene->level)))
		{
			return const_cast<CGObjectInstance*>(object.obj);
		}
	}
	
	//if not visitable tile - try to get blocked
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore || lockedObjects.count(object.obj))
			continue;
		
		if(object.obj->blockingAt(int3(x, y, scene->level)))
		{
			return const_cast<CGObjectInstance*>(object.obj);
		}
	}
	
	//finally, we can take any object
	for(auto & object : objects)
	{
		if(!object.obj || object.obj == ignore || lockedObjects.count(object.obj))
			continue;
		
		if(object.obj->coveringAt(int3(x, y, scene->level)))
		{
			return const_cast<CGObjectInstance*>(object.obj);
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

	std::set<CGObjectInstance *> selectedObjects;
	for(int j = y1; j < y2; ++j)
	{
		for(int i = x1; i < x2; ++i)
		{
			if(map->isInTheMap(int3(i, j, scene->level)))
			{
				for(auto & o : handler->getObjects(i, j, scene->level))
					if(!lockedObjects.count(o.obj))
					{
						selectedObjects.insert(const_cast<CGObjectInstance*>(o.obj));
					}
			}
		}
	}
	selectObjects(selectedObjects);
}

void SelectionObjectsLayer::selectObject(CGObjectInstance * obj)
{
	selectedObjects.insert(obj);
	onSelection();
	redraw({obj});
}

void SelectionObjectsLayer::selectObjects(const std::set<CGObjectInstance *> & objs)
{
	for (CGObjectInstance * obj : objs)
	{
		selectedObjects.insert(obj);
	}
	onSelection();
	redraw(objs);
}

void SelectionObjectsLayer::deselectObject(CGObjectInstance * obj)
{
	selectedObjects.erase(obj);
	redraw({obj});
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
	redraw();
}

void SelectionObjectsLayer::onSelection()
{
	 Q_EMIT selectionMade(!selectedObjects.empty());
}

void SelectionObjectsLayer::setShift(int x, int y)
{
	std::vector<QRectF>areas;

	if(shift.x() || shift.y())
	{
		for (auto * selectedObject : selectedObjects)
		{
			QRectF formerArea = getObjectArea(selectedObject);
			formerArea.moveTo(formerArea.topLeft() + (shift * tileSize));
			areas.emplace_back(formerArea);
		}
	}

	shift = QPoint(x, y);
	for (auto * selectedObject : selectedObjects)
	{
		QRectF area = getObjectArea(selectedObject);
		area.moveTo(area.topLeft() + (shift * tileSize));
		areas.emplace_back(area);
	}
	redraw(areas);
}

void SelectionObjectsLayer::setLockObject(CGObjectInstance * object, bool lock)
{
	if(lock)
		lockedObjects.insert(object);
	else
		lockedObjects.erase(object);
	redraw({object});
}

void SelectionObjectsLayer::unlockAll()
{
	lockedObjects.clear();
}

MinimapLayer::MinimapLayer(MapSceneBase * s): AbstractFixedLayer(s)
{

}

void MinimapLayer::update()
{
	if(!map)
		return;

	pixmap = std::make_unique<QPixmap>(map->width, map->height);

	QPainter painter(pixmap.get());
	//coordinate transformation
	for(int j = 0; j < map->height; ++j)
	{
		for(int i = 0; i < map->width; ++i)
		{
			handler->drawMinimapTile(painter, i, j, scene->level);
		}
	}
	
	redraw();
}

MinimapViewLayer::MinimapViewLayer(MapSceneBase * s): AbstractFixedLayer(s)
{
}

void MinimapViewLayer::update()
{
	if(!map)
		return;

	pixmap = std::make_unique<QPixmap>(map->width, map->height);

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
