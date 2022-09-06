#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include "mapcontroller.h"
#include "../lib/Terrain.h"


class CMap;
class ObjectBrowser;
class CGObjectInstance;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void initializeMap(bool isNew);
	void reloadMap(int level = 0);
	void saveMap();
	
	MapView * mapView();

	void loadObjectsTree();

	void setStatusMessage(const QString & status);

	int getMapLevel() const {return mapLevel;}
	
	MapController controller;

private slots:
    void on_actionOpen_triggered();

	void on_actionSave_as_triggered();

	void on_actionNew_triggered();

	void on_actionLevel_triggered();

	void on_actionSave_triggered();
	
	void on_actionUndo_triggered();

	void on_actionRedo_triggered();

	void on_actionPass_triggered(bool checked);

	void on_actionGrid_triggered(bool checked);

	void on_toolBrush_clicked(bool checked);

	void on_toolArea_clicked(bool checked);

	void terrainButtonClicked(Terrain terrain);

	void on_toolErase_clicked();

	void on_treeView_activated(const QModelIndex &index);

	void on_terrainFilterCombo_currentTextChanged(const QString &arg1);

	void on_filter_textChanged(const QString &arg1);

	void on_actionFill_triggered();

	void on_toolBrush2_clicked(bool checked);

	void on_toolBrush4_clicked(bool checked);

	void on_inspectorWidget_itemChanged(QTableWidgetItem *item);

	void on_actionMapSettings_triggered();

	void on_actionPlayers_settings_triggered();

	void on_actionValidate_triggered();

public slots:

	void treeViewSelected(const QModelIndex &selected, const QModelIndex &deselected);
	void loadInspector(CGObjectInstance * obj);
	void mapChanged();
	void enableUndo(bool enable);
	void enableRedo(bool enable);

private:
	void preparePreview(const QModelIndex &index, bool createNew);
	void addGroupIntoCatalog(const std::string & groupName, bool staticOnly);
	void addGroupIntoCatalog(const std::string & groupName, bool staticOnly, int ID);

	void changeBrushState(int idx);
	void setTitle();

private:
    Ui::MainWindow *ui;
	ObjectBrowser * objectBrowser = nullptr;
	QGraphicsScene * scenePreview;
	
	QString filename;
	bool unsaved = false;

	QStandardItemModel objectsModel;

	int mapLevel = 0;

	std::set<int> catalog;
};

#endif // MAINWINDOW_H
