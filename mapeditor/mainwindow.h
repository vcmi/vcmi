#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include "mapcontroller.h"
#include "../lib/Terrain.h"
#include "resourceExtractor/ResourceConverter.h"

class CMap;
class ObjectBrowserProxyModel;
class CGObjectInstance;

namespace Ui
{
	class MainWindow;
	const QString teamName = "VCMI Team";
	const QString appName = "VCMI Map Editor";
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

	const QString mainWindowSizeSetting = "MainWindow/Size";
	const QString mainWindowPositionSetting = "MainWindow/Position";

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void initializeMap(bool isNew);

	void saveMap();
	bool openMap(const QString &);
	
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

	void on_actionErase_triggered();
	
	void on_actionUndo_triggered();

	void on_actionRedo_triggered();

	void on_actionPass_triggered(bool checked);

	void on_actionGrid_triggered(bool checked);

	void on_toolBrush_clicked(bool checked);

	void on_toolArea_clicked(bool checked);

	void terrainButtonClicked(TerrainId terrain);
	void roadOrRiverButtonClicked(ui8 type, bool isRoad);

	void on_toolErase_clicked();

	void on_terrainFilterCombo_currentTextChanged(const QString &arg1);

	void on_filter_textChanged(const QString &arg1);

	void on_actionFill_triggered();

	void on_toolBrush2_clicked(bool checked);

	void on_toolBrush4_clicked(bool checked);

	void on_inspectorWidget_itemChanged(QTableWidgetItem *item);

	void on_actionMapSettings_triggered();

	void on_actionPlayers_settings_triggered();

	void on_actionValidate_triggered();

	void on_actionUpdate_appearance_triggered();

	void on_actionRecreate_obstacles_triggered();
	
	void switchDefaultPlayer(const PlayerColor &);

	void on_actionCut_triggered();

	void on_actionCopy_triggered();

	void on_actionPaste_triggered();

public slots:

	void treeViewSelected(const QModelIndex &selected, const QModelIndex &deselected);
	void loadInspector(CGObjectInstance * obj, bool switchTab);
	void mapChanged();
	void enableUndo(bool enable);
	void enableRedo(bool enable);
	void onSelectionMade(int level, bool anythingSelected);
	void onPlayersChanged();

	void displayStatus(const QString& message, int timeout = 2000);

private:
	void preparePreview(const QModelIndex & index);
	void addGroupIntoCatalog(const std::string & groupName, bool staticOnly);
	void addGroupIntoCatalog(const std::string & groupName, bool useCustomName, bool staticOnly, int ID);
	
	QAction * getActionPlayer(const PlayerColor &);

	void changeBrushState(int idx);
	void setTitle();
	
	void closeEvent(QCloseEvent *event) override;
	
	bool getAnswerAboutUnsavedChanges();

	void loadUserSettings();
	void saveUserSettings();

	void parseCommandLine(ExtractionOptions & extractionOptions);

private:
    Ui::MainWindow * ui;
	ObjectBrowserProxyModel * objectBrowser = nullptr;
	QGraphicsScene * scenePreview;
	
	QString filename;
	bool unsaved = false;

	QStandardItemModel objectsModel;

	int mapLevel = 0;

	std::set<int> catalog;

	// command line options
	QString mapFilePath;			// FilePath to the H3 or VCMI map to open
};
