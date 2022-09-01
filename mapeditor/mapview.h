#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include "../lib/int3.h"

class int3;
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

class MapScene : public QGraphicsScene
{
public:
	MapScene(MainWindow *parent, int l);

	void updateViews();

	GridView gridView;
	PassabilityView passabilityView;
	SelectionTerrainView selectionTerrainView;
	TerrainView terrainView;

	const int level;

private:
	MainWindow * main;
};

#endif // MAPVIEW_H
