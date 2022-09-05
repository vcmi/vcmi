#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include "scenelayer.h"
#include "../lib/int3.h"


class CGObjectInstance;
class MainWindow;
class MapController;

class MapScene : public QGraphicsScene
{
public:
	MapScene(int lev);
	
	void updateViews();
	void initialize(MapController &);
	
	GridLayer gridView;
	PassabilityLayer passabilityView;
	SelectionTerrainLayer selectionTerrainView;
	TerrainLayer terrainView;
	ObjectsLayer objectsView;
	SelectionObjectsLayer selectionObjectsView;
	
	const int level;
	
private:
	std::list<AbstractLayer *> getAbstractLayers();
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
	
signals:
	void openObjectProperties(CGObjectInstance *);

private:
	MapController * controller = nullptr;
	QPointF mouseStart;
	int3 tileStart;
	int3 tilePrev;
	bool pressedOnSelected;
};

#endif // MAPVIEW_H
