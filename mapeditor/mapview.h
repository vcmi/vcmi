#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>

class MainWindow;
class MapScene;

class BasicView
{
public:
	BasicView(MainWindow * m, MapScene * s);

	virtual void update() = 0;

	void show(bool show);

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
	MapView(QWidget *parent);

	void setMain(MainWindow * m);

public slots:
	void mouseMoveEvent(QMouseEvent * mouseEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private:
	MainWindow * main;
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

class MapScene : public QGraphicsScene
{
public:
	MapScene(MainWindow *parent, int l);

	void updateViews(const QPixmap & surface);

	GridView gridView;
	PassabilityView passabilityView;

	const int level;

private:
	std::unique_ptr<QGraphicsPixmapItem> item;
	MainWindow * main;
};

#endif // MAPVIEW_H
