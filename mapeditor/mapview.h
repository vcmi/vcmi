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
#include <QRubberBand>
#include "scenelayer.h"
#include "../lib/int3.h"


VCMI_LIB_NAMESPACE_BEGIN
class CGObjectInstance;
VCMI_LIB_NAMESPACE_END

class MainWindow;
class MapController;

class MapSceneBase : public QGraphicsScene
{
	Q_OBJECT;
public:
	MapSceneBase(int lvl);
	
	const int level;
	virtual void createMap();
	virtual void updateMap();
	virtual void initialize(MapController &);
	virtual std::list<AbstractFixedLayer *> getStaticLayers() = 0;
	virtual std::list<AbstractViewportLayer *> getDynamicLayers() = 0;
};

class MinimapScene : public MapSceneBase
{
public:
	MinimapScene(int lvl);
	
	void createMap() override;
	
	MinimapLayer minimapView;
	MinimapViewLayer viewport;
	std::list<AbstractFixedLayer *> getStaticLayers() override;
	std::list<AbstractViewportLayer *> getDynamicLayers() override;
};

class MapScene : public MapSceneBase
{
	Q_OBJECT
public:
	MapScene(int lvl);
	
	void createMap() override;
	std::list<AbstractFixedLayer *> getStaticLayers() override;
	std::list<AbstractViewportLayer *> getDynamicLayers() override;
	
	EmptyLayer emptyLayer;
	GridLayer gridView;
	PassabilityLayer passabilityView;
	SelectionTerrainLayer selectionTerrainView;
	TerrainLayer terrainView;
	ObjectsLayer objectsView;
	SelectionObjectsLayer selectionObjectsView;
	ObjectPickerLayer objectPickerView;

signals:
	void selected(bool anything);

public slots:
	void terrainSelected(bool anything);
	void objectSelected(bool anything);

protected:
	bool isTerrainSelected;
	bool isObjectSelected;

};

class MapView : public QGraphicsView
{
	Q_OBJECT
public:
	enum class SelectionTool
	{
		None, Brush, Brush2, Brush4, Area, Lasso, Line, Fill
	};

public:
	MapView(QWidget * parent);
	void setController(MapController *);

	SelectionTool selectionTool;

public slots:
	void resizeEvent (QResizeEvent * event) override;
	void mouseMoveEvent(QMouseEvent * mouseEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent * event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent * event) override;
	void cameraChanged(const QPointF & pos);
	void setViewports();
	
signals:
	void openObjectProperties(CGObjectInstance *, bool switchTab);
	void currentCoordinates(int, int);
	//void viewportChanged(const QRectF & rect);

protected:
	
private:
	MapController * controller = nullptr;
	QRubberBand * rubberBand = nullptr;
	QPointF mouseStart;
	int3 tileStart;
	int3 tilePrev;
	bool pressedOnSelected;
	
	std::set<int3> temporaryTiles;
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
	void mousePressEvent(QMouseEvent * event) override;
	
signals:
	void cameraPositionChanged(const QPointF & newPosition);
	
private:
	MapController * controller = nullptr;
	
	int displayWidth = 192;
	int displayHeight = 192;
};
