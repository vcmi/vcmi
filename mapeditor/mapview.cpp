/*
 * mapview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapview.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>
#include "mapcontroller.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"

MinimapView::MinimapView(QWidget * parent):
	QGraphicsView(parent)
{
	// Disable scrollbars
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MinimapView::dimensions()
{
	fitInView(0, 0, controller->map()->width, controller->map()->height, Qt::KeepAspectRatio);
}

void MinimapView::setController(MapController * ctrl)
{
	controller = ctrl;
}

void MinimapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();
	
	auto * sc = dynamic_cast<MinimapScene*>(scene());
	if(!sc)
		return;
	
	int w = sc->viewport.viewportWidth();
	int h = sc->viewport.viewportHeight();
	auto pos = mapToScene(mouseEvent->pos());
	pos.setX(pos.x() - w / 2);
	pos.setY(pos.y() - h / 2);
	
	QPointF point = pos * 32;
			
	emit cameraPositionChanged(point);
}

void MinimapView::mousePressEvent(QMouseEvent* event)
{
	mouseMoveEvent(event);
}

MapView::MapView(QWidget * parent):
	QGraphicsView(parent),
	selectionTool(MapView::SelectionTool::None)
{
}

void MapView::cameraChanged(const QPointF & pos)
{
	horizontalScrollBar()->setValue(pos.x());
	verticalScrollBar()->setValue(pos.y());
}

void MapView::setController(MapController * ctrl)
{
	controller = ctrl;
}

void MapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();

	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc || !controller->map())
		return;

	auto pos = mapToScene(mouseEvent->pos()); //TODO: do we need to check size?
	int3 tile(pos.x() / 32, pos.y() / 32, sc->level);
	
	//if scene will be scrolled without mouse movement, selection, object moving and rubber band will not be updated
	//to change this behavior, all this logic should be placed in viewportEvent
	if(rubberBand)
		rubberBand->setGeometry(QRect(mapFromScene(mouseStart), mouseEvent->pos()).normalized());
	
	if(tile == tilePrev) //do not redraw
		return;

	tilePrev = tile;

	//TODO: cast parent->parent to MainWindow in order to show coordinates or another way to do it?
	//main->setStatusMessage(QString("x: %1 y: %2").arg(tile.x, tile.y));

	switch(selectionTool)
	{
	case MapView::SelectionTool::Brush:
		if(mouseEvent->buttons() & Qt::RightButton)
			sc->selectionTerrainView.erase(tile);
		else if(mouseEvent->buttons() == Qt::LeftButton)
			sc->selectionTerrainView.select(tile);
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Brush2:
		{
			std::array<int3, 4> extra{ int3{0, 0, 0}, int3{1, 0, 0}, int3{0, 1, 0}, int3{1, 1, 0} };
			for(auto & e : extra)
			{
				if(mouseEvent->buttons() & Qt::RightButton)
					sc->selectionTerrainView.erase(tile + e);
				else if(mouseEvent->buttons() == Qt::LeftButton)
					sc->selectionTerrainView.select(tile + e);
			}
		}
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Brush4:
	{
		std::array<int3, 16> extra{
			int3{-1, -1, 0}, int3{0, -1, 0}, int3{1, -1, 0}, int3{2, -1, 0},
			int3{-1, 0, 0}, int3{0, 0, 0}, int3{1, 0, 0}, int3{2, 0, 0},
			int3{-1, 1, 0}, int3{0, 1, 0}, int3{1, 1, 0}, int3{2, 1, 0},
			int3{-1, 2, 0}, int3{0, 2, 0}, int3{1, 2, 0}, int3{2, 2, 0}
		};
		for(auto & e : extra)
		{
			if(mouseEvent->buttons() & Qt::RightButton)
				sc->selectionTerrainView.erase(tile + e);
			else if(mouseEvent->buttons() == Qt::LeftButton)
				sc->selectionTerrainView.select(tile + e);
		}
	}
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Area:
		if(mouseEvent->buttons() & Qt::RightButton || !(mouseEvent->buttons() & Qt::LeftButton))
			break;

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
			
	case MapView::SelectionTool::Lasso:
		if(mouseEvent->buttons() == Qt::LeftButton)
		{
			sc->selectionTerrainView.select(tile);
			sc->selectionTerrainView.draw();
		}
		break;

	case MapView::SelectionTool::None:
		if(mouseEvent->buttons() & Qt::RightButton)
			break;

		auto sh = tile - tileStart;
		sc->selectionObjectsView.shift = QPoint(sh.x, sh.y);

		if(sh.x || sh.y)
		{
			if(!sc->selectionObjectsView.newObject && (mouseEvent->buttons() & Qt::LeftButton))
			{
				if(sc->selectionObjectsView.selectionMode == SelectionObjectsLayer::SELECTION)
				{
					sc->selectionObjectsView.clear();
					sc->selectionObjectsView.selectObjects(tileStart.x, tileStart.y, tile.x, tile.y);
				}
			}
		}

		sc->selectionObjectsView.draw();
		break;
	}
}

void MapView::mousePressEvent(QMouseEvent *event)
{
	this->update();

	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc || !controller->map())
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
		sc->selectionObjectsView.clear();
		sc->selectionObjectsView.draw();

		if(event->button() == Qt::RightButton)
			sc->selectionTerrainView.erase(tileStart);
		else if(event->button() == Qt::LeftButton)
			sc->selectionTerrainView.select(tileStart);
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Brush2:
		sc->selectionObjectsView.clear();
		sc->selectionObjectsView.draw();
	{
		std::array<int3, 4> extra{ int3{0, 0, 0}, int3{1, 0, 0}, int3{0, 1, 0}, int3{1, 1, 0} };
		for(auto & e : extra)
		{
			if(event->button() == Qt::RightButton)
				sc->selectionTerrainView.erase(tileStart + e);
			else if(event->button() == Qt::LeftButton)
				sc->selectionTerrainView.select(tileStart + e);
		}
	}
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Brush4:
		sc->selectionObjectsView.clear();
		sc->selectionObjectsView.draw();
	{
		std::array<int3, 16> extra{
			int3{-1, -1, 0}, int3{0, -1, 0}, int3{1, -1, 0}, int3{2, -1, 0},
			int3{-1, 0, 0}, int3{0, 0, 0}, int3{1, 0, 0}, int3{2, 0, 0},
			int3{-1, 1, 0}, int3{0, 1, 0}, int3{1, 1, 0}, int3{2, 1, 0},
			int3{-1, 2, 0}, int3{0, 2, 0}, int3{1, 2, 0}, int3{2, 2, 0}
		};
		for(auto & e : extra)
		{
			if(event->button() == Qt::RightButton)
				sc->selectionTerrainView.erase(tileStart + e);
			else if(event->button() == Qt::LeftButton)
				sc->selectionTerrainView.select(tileStart + e);
		}
	}
		sc->selectionTerrainView.draw();
		break;

	case MapView::SelectionTool::Area:
	case MapView::SelectionTool::Lasso:
		if(event->button() == Qt::RightButton)
			break;

		sc->selectionTerrainView.clear();
		sc->selectionTerrainView.draw();
		sc->selectionObjectsView.clear();
		sc->selectionObjectsView.draw();
		break;

	case MapView::SelectionTool::None:
		sc->selectionTerrainView.clear();
		sc->selectionTerrainView.draw();

		if(sc->selectionObjectsView.newObject && sc->selectionObjectsView.isSelected(sc->selectionObjectsView.newObject))
		{
			if(event->button() == Qt::RightButton)
				controller->discardObject(sc->level);
		}
		else
		{
			if(event->button() == Qt::LeftButton)
			{
				//when paste, new object could be beyond initial object so we need to test two objects in order to select new one
				//if object is pasted at place where is multiple objects then proper selection is not guaranteed
				auto * firstSelectedObject = sc->selectionObjectsView.selectObjectAt(tileStart.x, tileStart.y);
				auto * secondSelectedObject = sc->selectionObjectsView.selectObjectAt(tileStart.x, tileStart.y, firstSelectedObject);
				if(firstSelectedObject)
				{
					if(sc->selectionObjectsView.isSelected(firstSelectedObject))
					{
						if(qApp->keyboardModifiers() & Qt::ControlModifier)
						{
							sc->selectionObjectsView.deselectObject(firstSelectedObject);
							sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::SELECTION;
						}
						else
							sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
					}
					else
					{
						if(!secondSelectedObject || !sc->selectionObjectsView.isSelected(secondSelectedObject))
						{
							if(!(qApp->keyboardModifiers() & Qt::ControlModifier))
								sc->selectionObjectsView.clear();
							sc->selectionObjectsView.selectObject(firstSelectedObject);
						}
						sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
					}
				}
				else
				{
					sc->selectionObjectsView.clear();
					sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::SELECTION;
					
					if(!rubberBand)
						rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
					rubberBand->setGeometry(QRect(mapFromScene(mouseStart), QSize()));
					rubberBand->show();
				}
			}
			sc->selectionObjectsView.shift = QPoint(0, 0);
			sc->selectionObjectsView.draw();
		}
		break;
	}

	//main->setStatusMessage(QString("x: %1 y: %2").arg(QString::number(event->pos().x()), QString::number(event->pos().y())));
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
	this->update();

	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc || !controller->map())
		return;
	
	if(rubberBand)
		rubberBand->hide();

	switch(selectionTool)
	{
	case MapView::SelectionTool::Lasso: {
		if(event->button() == Qt::RightButton)
			break;
				
		//key: y position of tile
		//value.first: x position of left tile
		//value.second: x postiion of right tile
		std::map<int, std::pair<int, int>> selectionRangeMapX, selectionRangeMapY;
		for(auto & t : sc->selectionTerrainView.selection())
		{
			auto pairIter = selectionRangeMapX.find(t.y);
			if(pairIter == selectionRangeMapX.end())
				selectionRangeMapX[t.y] = std::make_pair(t.x, t.x);
			else
			{
				pairIter->second.first = std::min(pairIter->second.first, t.x);
				pairIter->second.second = std::max(pairIter->second.second, t.x);
			}
			
			pairIter = selectionRangeMapY.find(t.x);
			if(pairIter == selectionRangeMapY.end())
				selectionRangeMapY[t.x] = std::make_pair(t.y, t.y);
			else
			{
				pairIter->second.first = std::min(pairIter->second.first, t.y);
				pairIter->second.second = std::max(pairIter->second.second, t.y);
			}
		}
		
		std::set<int3> selectionByX, selectionByY;
		std::vector<int3> finalSelection;
		for(auto & selectionRange : selectionRangeMapX)
		{
			for(int i = selectionRange.second.first; i < selectionRange.second.second; ++i)
				selectionByX.insert(int3(i, selectionRange.first, sc->level));
		}
		for(auto & selectionRange : selectionRangeMapY)
		{
			for(int i = selectionRange.second.first; i < selectionRange.second.second; ++i)
				selectionByY.insert(int3(selectionRange.first, i, sc->level));
		}
		std::set_intersection(selectionByX.begin(), selectionByX.end(), selectionByY.begin(), selectionByY.end(), std::back_inserter(finalSelection));
		for(auto & lassoTile : finalSelection)
			sc->selectionTerrainView.select(lassoTile);
		
		sc->selectionTerrainView.draw();
		break;
	}
			
	case MapView::SelectionTool::None:
		if(event->button() == Qt::RightButton)
			break;
		//switch position
		bool tab = false;
		if(sc->selectionObjectsView.selectionMode == SelectionObjectsLayer::MOVEMENT)
		{
			controller->commitObjectShift(sc->level);
		}
		else
		{
			sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
			sc->selectionObjectsView.shift = QPoint(0, 0);
			sc->selectionObjectsView.draw();
			tab = true;
			//check if we have only one object
		}
		auto selection = sc->selectionObjectsView.getSelection();
		if(selection.size() == 1)
		{
			emit openObjectProperties(*selection.begin(), tab);
		}
		break;
	}
}

void MapView::dragEnterEvent(QDragEnterEvent * event)
{
	if(!controller || !controller->map())
		return;
	
	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	if(event->mimeData()->hasImage())
	{
		logGlobal->info("Drag'n'drop: dispatching object");
		QVariant vdata = event->mimeData()->imageData();
		auto data = vdata.toJsonObject();
		if(!data.empty())
		{
			auto preview = data["preview"];
			if(preview != QJsonValue::Undefined)
			{
				auto objId = data["id"].toInt();
				auto objSubId = data["subid"].toInt();
				auto templateId = data["template"].toInt();
				auto factory = VLC->objtypeh->getHandlerFor(objId, objSubId);
				auto templ = factory->getTemplates()[templateId];
				controller->discardObject(sc->level);
				controller->createObject(sc->level, factory->create(templ));
			}
		}
		
		event->acceptProposedAction();
	}
}

void MapView::dropEvent(QDropEvent * event)
{
	if(!controller || !controller->map())
		return;
	
	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	if(sc->selectionObjectsView.newObject)
	{
		QString errorMsg;
		if(controller->canPlaceObject(sc->level, sc->selectionObjectsView.newObject, errorMsg))
		{
			controller->commitObjectCreate(sc->level);
		}
		else
		{
			controller->discardObject(sc->level);
			QMessageBox::information(this, "Can't place object", errorMsg);
		}
	}

	event->acceptProposedAction();
}

void MapView::dragMoveEvent(QDragMoveEvent * event)
{
	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	auto rect = event->answerRect();
	auto pos = mapToScene(rect.bottomRight()); //TODO: do we need to check size?
	int3 tile(pos.x() / 32 + 1, pos.y() / 32 + 1, sc->level);
	
	if(sc->selectionObjectsView.newObject)
	{
		sc->selectionObjectsView.shift = QPoint(tile.x, tile.y);
		sc->selectionObjectsView.selectObject(sc->selectionObjectsView.newObject);
		sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
		sc->selectionObjectsView.draw();
	}
	
	event->acceptProposedAction();
}

void MapView::dragLeaveEvent(QDragLeaveEvent * event)
{
	if(!controller || !controller->map())
		return;
	
	auto * sc = dynamic_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	controller->discardObject(sc->level);
}


bool MapView::viewportEvent(QEvent *event)
{
	if(auto * sc = dynamic_cast<MapScene*>(scene()))
	{
		auto rect = mapToScene(viewport()->geometry()).boundingRect();
		controller->miniScene(sc->level)->viewport.setViewport(rect.x() / 32, rect.y() / 32, rect.width() / 32, rect.height() / 32);
	}
	return QGraphicsView::viewportEvent(event);
}

MapSceneBase::MapSceneBase(int lvl):
	QGraphicsScene(nullptr),
	level(lvl)
{
}

void MapSceneBase::initialize(MapController & controller)
{
	for(auto * layer : getAbstractLayers())
		layer->initialize(controller);
}

void MapSceneBase::updateViews()
{
	for(auto * layer : getAbstractLayers())
		layer->update();
}

MapScene::MapScene(int lvl):
	MapSceneBase(lvl),
	gridView(this),
	passabilityView(this),
	selectionTerrainView(this),
	terrainView(this),
	objectsView(this),
	selectionObjectsView(this),
	isTerrainSelected(false),
	isObjectSelected(false)
{
	connect(&selectionTerrainView, &SelectionTerrainLayer::selectionMade, this, &MapScene::terrainSelected);
	connect(&selectionObjectsView, &SelectionObjectsLayer::selectionMade, this, &MapScene::objectSelected);
}

std::list<AbstractLayer *> MapScene::getAbstractLayers()
{
	//sequence is important because it defines rendering order
	return {
		&terrainView,
		&objectsView,
		&gridView,
		&passabilityView,
		&selectionTerrainView,
		&selectionObjectsView
	};
}

void MapScene::updateViews()
{
	MapSceneBase::updateViews();

	terrainView.show(true);
	objectsView.show(true);
	selectionTerrainView.show(true);
	selectionObjectsView.show(true);
}

void MapScene::terrainSelected(bool anythingSelected)
{
	isTerrainSelected = anythingSelected;
	emit selected(isTerrainSelected || isObjectSelected);
}

void MapScene::objectSelected(bool anythingSelected)
{
	isObjectSelected = anythingSelected;
	emit selected(isTerrainSelected || isObjectSelected);
}

MinimapScene::MinimapScene(int lvl):
	MapSceneBase(lvl),
	minimapView(this),
	viewport(this)
{
}

std::list<AbstractLayer *> MinimapScene::getAbstractLayers()
{
	//sequence is important because it defines rendering order
	return {
		&minimapView,
		&viewport
	};
}

void MinimapScene::updateViews()
{
	MapSceneBase::updateViews();
	
	minimapView.show(true);
	viewport.show(true);
}
