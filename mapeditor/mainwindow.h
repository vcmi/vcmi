#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include "../lib/Terrain.h"

#include "maphandler.h"
#include "mapview.h"

class CMap;
class ObjectBrowser;

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
	void resetMapHandler();

	void loadObjectsTree();

	void setStatusMessage(const QString & status);

	MapView * getMapView();

	int getMapLevel() const {return mapLevel;}

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

	void on_toolErase_clicked();

	void on_treeView_activated(const QModelIndex &index);

	void on_terrainFilterCombo_currentTextChanged(const QString &arg1);

	void on_filter_textChanged(const QString &arg1);

public slots:

	void treeViewSelected(const QModelIndex &selected, const QModelIndex &deselected);

private:
	void preparePreview(const QModelIndex &index, bool createNew);
	void addGroupIntoCatalog(const std::string & groupName, bool staticOnly);
	void addGroupIntoCatalog(const std::string & groupName, bool staticOnly, int ID);

private:
    Ui::MainWindow *ui;
	ObjectBrowser * objectBrowser = nullptr;
	std::unique_ptr<MapHandler> mapHandler;
	std::array<MapScene *, 2> scenes;
	QGraphicsScene * sceneMini;
	QGraphicsScene * scenePreview;
	QPixmap minimap;
	QPixmap objPreview;
	
	std::unique_ptr<CMap> map;
	QString filename;
	bool unsaved = false;

	QStandardItemModel objectsModel;

	int mapLevel = 0;

	std::set<int> catalog;
};

#endif // MAINWINDOW_H
