#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include "../lib/Terrain.h"

#include "maphandler.h"
#include "mapview.h"

class CMap;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void setMapRaw(std::unique_ptr<CMap> cmap);
	void setMap(bool isNew);
	void reloadMap(int level = 0);
	void saveMap();

	CMap * getMap();
	MapHandler * getMapHandler();

	void loadObjectsTree();

	void setStatusMessage(const QString & status);

	MapView * getMapView();

private slots:
    void on_actionOpen_triggered();

	void on_actionSave_as_triggered();

	void on_actionNew_triggered();

	void on_actionLevel_triggered();

	void on_actionSave_triggered();

	void on_actionPass_triggered(bool checked);

	void on_actionGrid_triggered(bool checked);

	void on_toolBrush_clicked(bool checked);

	void on_toolArea_clicked(bool checked);

	void terrainButtonClicked(Terrain terrain);

private:
    Ui::MainWindow *ui;
	
	std::unique_ptr<MapHandler> mapHandler;
	std::array<MapScene *, 2> scenes;
	QGraphicsScene * sceneMini;
	QPixmap minimap;
	
	std::unique_ptr<CMap> map;
	QString filename;
	bool unsaved = false;

	QStandardItemModel objectsModel;

	int mapLevel = 0;
};

#endif // MAINWINDOW_H
