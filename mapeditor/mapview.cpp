#include "StdInc.h"
#include "mapview.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>

MapView::MapView(QWidget *parent):
	QGraphicsView(parent),
	selectionTool(MapView::SelectionTool::Brush)
{
}

void MapView::setMain(MainWindow * m)
{
	main = m;
}

void MapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc)
		return;

	auto pos = mapToScene(mouseEvent->pos()); //TODO: do we need to check size?
	int3 tile(pos.x() / 32, pos.y() / 32, sc->level);

	if(tile == tilePrev) //do not redraw
		return;

	tilePrev = tile;

	//main->setStatusMessage(QString("x: %1 y: %2").arg(QString::number(pos.x()), QString::number(pos.y())));

	switch(selectionTool)
	{
	case MapView::SelectionTool::Brush:
		if(pressedOnSelected)
			sc->selectionTerrainView.erase(tile);
		else
			sc->selectionTerrainView.select(tile);
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Area:
		sc->selectionTerrainView.clear();
		for(int j = std::min(tile.y, tileStart.y); j < std::max(tile.y, tileStart.y); ++j)
		{
			for(int i = std::min(tile.x, tileStart.x); i < std::max(tile.x, tileStart.x); ++i)
			{
				sc->selectionTerrainView.select(int3(i, j, sc->level));
			}
		}
		sc->selectionTerrainView.draw();
		break;
	}
}

void MapView::mousePressEvent(QMouseEvent *event)
{
	this->update();

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc)
		return;

	mouseStart = mapToScene(event->pos());
	tileStart = tilePrev = int3(mouseStart.x() / 32, mouseStart.y() / 32, sc->level);

	if(sc->selectionTerrainView.selection().count(tileStart))
		pressedOnSelected = true;
	else
		pressedOnSelected = false;

	switch(selectionTool)
	{
	case MapView::SelectionTool::Brush:
		if(pressedOnSelected)
			sc->selectionTerrainView.erase(tileStart);
		else
			sc->selectionTerrainView.select(tileStart);
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Area:
		sc->selectionTerrainView.clear();
		sc->selectionTerrainView.draw();
		break;
	}

	//main->setStatusMessage(QString("x: %1 y: %2").arg(QString::number(event->pos().x()), QString::number(event->pos().y())));
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
	this->update();
}

MapScene::MapScene(MainWindow *parent, int l):
	QGraphicsScene(parent),
	gridView(parent, this),
	passabilityView(parent, this),
	selectionTerrainView(parent, this),
	terrainView(parent, this),
	main(parent),
	level(l)
{

}

void MapScene::updateViews()
{
	terrainView.update();
	gridView.update();
	passabilityView.update();
	selectionTerrainView.update();
}

BasicView::BasicView(MainWindow * m, MapScene * s): main(m), scene(s)
{
	if(main->getMap())
		update();
}

void BasicView::show(bool show)
{
	if(isShown == show)
		return;

	if(show)
	{
		if(item)
			scene->addItem(item.get());
		else
			item.reset(scene->addPixmap(*pixmap));
	}
	else
	{
		scene->removeItem(item.get());
	}

	isShown = show;
}

void BasicView::redraw()
{
	if(item)
		scene->removeItem(item.get());

	if(isShown)
	{
		item.reset(scene->addPixmap(*pixmap));
	}
}

GridView::GridView(MainWindow * m, MapScene * s): BasicView(m, s)
{
}

void GridView::update()
{
	auto map = main->getMap();
	if(!map)
		return;

	show(false);

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

	show(isShown);
}

PassabilityView::PassabilityView(MainWindow * m, MapScene * s): BasicView(m, s)
{
}

void PassabilityView::update()
{
	auto map = main->getMap();
	if(!map)
		return;

	show(false);

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

	show(isShown);
}

SelectionTerrainView::SelectionTerrainView(MainWindow * m, MapScene * s): BasicView(m, s)
{
}

void SelectionTerrainView::update()
{
	auto map = main->getMap();
	if(!map)
		return;

	show(false);

	area.clear();
	areaAdd.clear();
	areaErase.clear();

	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	pixmap->fill(QColor(0, 0, 0, 0));

	show(true); //always visible
}

void SelectionTerrainView::draw()
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

void SelectionTerrainView::select(const int3 & tile)
{
	if(!area.count(tile))
	{
		area.insert(tile);
		areaAdd.insert(tile);
		areaErase.erase(tile);
	}
}

void SelectionTerrainView::erase(const int3 & tile)
{
	if(area.count(tile))
	{
		area.erase(tile);
		areaErase.insert(tile);
		areaAdd.erase(tile);
	}
}

void SelectionTerrainView::clear()
{
	areaErase = area;
	areaAdd.clear();
	area.clear();
}

const std::set<int3> & SelectionTerrainView::selection() const
{
	return area;
}

TerrainView::TerrainView(MainWindow * m, MapScene * s): BasicView(m, s)
{
}

void TerrainView::update()
{
	auto map = main->getMap();
	if(!map)
		return;

	show(false);

	pixmap.reset(new QPixmap(map->width * 32, map->height * 32));
	draw(false);

	show(true); //always visible
}

void TerrainView::setDirty(const int3 & tile)
{
	dirty.insert(tile);
}

void TerrainView::draw(bool onlyDirty)
{
	if(!pixmap)
		return;

	auto map = main->getMap();

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
			main->getMapHandler()->drawTerrainTile(painter, t.x, t.y, scene->level);
		}
	}
	else
	{
		for(int j = 0; j < map->height; ++j)
			for(int i = 0; i < map->width; ++i)
				main->getMapHandler()->drawTerrainTile(painter, i, j, scene->level);
	}

	redraw();
}
