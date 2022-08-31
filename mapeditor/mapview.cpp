#include "mapview.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>

MapView::MapView(QWidget *parent):
	QGraphicsView(parent)
{
}

void MapView::setMain(MainWindow * m)
{
	main = m;
}

void MapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();

	auto pos = mouseEvent->pos();

	main->setStatusMessage(QString("x: %1 y: %2").arg(QString::number(pos.x()), QString::number(pos.y())));
}

void MapView::mousePressEvent(QMouseEvent *event)
{
	this->update();
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
	this->update();
}

MapScene::MapScene(MainWindow *parent, int l):
	QGraphicsScene(parent),
	gridView(parent, this),
	passabilityView(parent, this),
	main(parent),
	level(l)
{

}

void MapScene::updateViews(const QPixmap & surface)
{
	if(item)
		removeItem(item.get());
	item.reset(addPixmap(surface));

	gridView.update();
	passabilityView.update();
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

GridView::GridView(MainWindow * m, MapScene * s): BasicView(m, s)
{
}

void GridView::update()
{
	show(false);
	auto map = main->getMap();

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
	show(false);
	auto map = main->getMap();

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
