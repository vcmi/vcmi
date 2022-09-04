#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include "../lib/int3.h"

class int3;
class CGObjectInstance;
class MainWindow;
class MapScene;

class BasicView
{
public:
	BasicView(MainWindow * m, MapScene * s);

	virtual void update() = 0;

	void show(bool show);
	void redraw();

protected:
	MainWindow * main;
	MapScene * scene;
	bool isShown = false;

	std::unique_ptr<QPixmap> pixmap;
	QPixmap emptyPixmap;

private:
	std::unique_ptr<QGraphicsPixmapItem> item;
};

class MapView : public QGraphicsView
{
public:
	enum class SelectionTool
	{
		None, Brush, Brush2, Brush4, Area, Lasso
	};

public:
	MapView(QWidget *parent);

	void setMain(MainWindow * m);

	SelectionTool selectionTool;

public slots:
	void mouseMoveEvent(QMouseEvent * mouseEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private:
	MainWindow * main;

	QPointF mouseStart;
	int3 tileStart;
	int3 tilePrev;
	bool pressedOnSelected;
};

class GridView: public BasicView
{
public:
	GridView(MainWindow * m, MapScene * s);

	void update() override;
};

class PassabilityView: public BasicView
{
public:
	PassabilityView(MainWindow * m, MapScene * s);

	void update() override;
};

class SelectionTerrainView: public BasicView
{
public:
	SelectionTerrainView(MainWindow * m, MapScene * s);

	void update() override;

	void draw();
	void select(const int3 & tile);
	void erase(const int3 & tile);
	void clear();

	const std::set<int3> & selection() const;

private:
	std::set<int3> area, areaAdd, areaErase;
};

class TerrainView: public BasicView
{
public:
	TerrainView(MainWindow * m, MapScene * s);

	void update() override;

	void draw(bool onlyDirty = true);
	void setDirty(const int3 & tile);

private:
	std::set<int3> dirty;
};

class ObjectsView: public BasicView
{
public:
	ObjectsView(MainWindow * m, MapScene * s);

	void update() override;

	void draw(bool onlyDirty = true); //TODO: implement dirty

	void setDirty(int x, int y);
	void setDirty(const CGObjectInstance * object);

private:
	std::set<const CGObjectInstance *> dirty;
};

class SelectionObjectsView: public BasicView
{
public:
	SelectionObjectsView(MainWindow * m, MapScene * s);

	void update() override;

	void draw();

	CGObjectInstance * selectObjectAt(int x, int y) const;
	void selectObjects(int x1, int y1, int x2, int y2);
	void selectObject(CGObjectInstance *);
	bool isSelected(const CGObjectInstance *) const;
	std::set<CGObjectInstance*> getSelection() const;
	void moveSelection(int x, int y);
	void clear();

	bool applyShift();
	void deleteSelection();

	QPoint shift;
	CGObjectInstance * newObject;
	int selectionMode = 0; //0 - nothing, 1 - selection, 2 - movement

private:
	std::set<CGObjectInstance *> selectedObjects;
};

class MapScene : public QGraphicsScene
{
public:
	MapScene(MainWindow *parent, int l);

	void updateViews();

	GridView gridView;
	PassabilityView passabilityView;
	SelectionTerrainView selectionTerrainView;
	TerrainView terrainView;
	ObjectsView objectsView;
	SelectionObjectsView selectionObjectsView;

	const int level;

private:
	MainWindow * main;
};

#endif // MAPVIEW_H
