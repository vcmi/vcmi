/*
 * mapview.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include "scenelayer.h"
#include "../lib/int3.h"


class CGObjectInstance;
class MainWindow;
class MapController;

class MapSceneBase : public QGraphicsScene
{
	Q_OBJECT;
public:
	MapSceneBase(int lvl);
	
	const int level;
	
	virtual void updateViews();
	virtual void initialize(MapController &);
	
protected:
	virtual std::list<AbstractLayer *> getAbstractLayers() = 0;
};

class MinimapScene : public MapSceneBase
{
public:
	MinimapScene(int lvl);
	
	void updateViews() override;
	
	MinimapLayer minimapView;
	MinimapViewLayer viewport;
	
protected:
	std::list<AbstractLayer *> getAbstractLayers() override;
};

class MapScene : public MapSceneBase
{
	Q_OBJECT
public:
	MapScene(int lvl);
	
	void updateViews() override;
	
	GridLayer gridView;
	PassabilityLayer passabilityView;
	SelectionTerrainLayer selectionTerrainView;
	TerrainLayer terrainView;
	ObjectsLayer objectsView;
	SelectionObjectsLayer selectionObjectsView;

signals:
	void selected(bool anything);

public slots:
	void terrainSelected(bool anything);
	void objectSelected(bool anything);
	
protected:
	std::list<AbstractLayer *> getAbstractLayers() override;

	bool isTerrainSelected;
	bool isObjectSelected;

};

class MapView : public QGraphicsView
{
	Q_OBJECT
public:
	enum class SelectionTool
	{
		None, Brush, Brush2, Brush4, Area, Lasso
	};

public:
	MapView(QWidget * parent);
	void setController(MapController *);

	SelectionTool selectionTool;

public slots:
	void mouseMoveEvent(QMouseEvent * mouseEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	
	void cameraChanged(const QPointF & pos);
	
signals:
	void openObjectProperties(CGObjectInstance *, bool switchTab);
	//void viewportChanged(const QRectF & rect);

protected:
	bool viewportEvent(QEvent *event) override;
	
private:
	MapController * controller = nullptr;
	QPointF mouseStart;
	int3 tileStart;
	int3 tilePrev;
	bool pressedOnSelected;
};

class MinimapView : public QGraphicsView
{
	Q_OBJECT
public:
	MinimapView(QWidget * parent);
	void setController(MapController *);
	
	void dimensions();
	
public slots:
	void mouseMoveEvent(QMouseEvent * mouseEvent) override;
	void mousePressEvent(QMouseEvent* event) override;
	
signals:
	void cameraPositionChanged(const QPointF & newPosition);
	
private:
	MapController * controller = nullptr;
	
	int displayWidth = 192;
	int displayHeight = 192;
};
