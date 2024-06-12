#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include "mapcontroller.h"
#include "resourceExtractor/ResourceConverter.h"

class ObjectBrowser;
class ObjectBrowserProxyModel;

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
class CGObjectInstance;
VCMI_LIB_NAMESPACE_END

namespace Ui
{
	class MainWindow;
	const QString teamName = "vcmi";
	const QString appName = "mapeditor";
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

	const QString mainWindowSizeSetting = "MainWindow/Size";
	const QString mainWindowPositionSetting = "MainWindow/Position";
	const QString lastDirectorySetting = "MainWindow/Directory";

#ifdef ENABLE_QT_TRANSLATIONS
	QTranslator translator;
#endif
	
	std::unique_ptr<CMap> openMapInternal(const QString &);

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void initializeMap(bool isNew);

	void saveMap();
	bool openMap(const QString &);
	
	//MapView * mapView();

	void loadObjectsTree();

	void setStatusMessage(const QString & status);

	int getMapLevel() const {return mapLevel;}
	
	MapController controller;

	void loadTranslation();

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

	void terrainButtonClicked(TerrainId terrain);
	void roadOrRiverButtonClicked(ui8 type, bool isRoad);
	void currentCoordinatesChanged(int x, int y);

	void on_terrainFilterCombo_currentIndexChanged(int index);

	void on_filter_textChanged(const QString &arg1);

	void on_actionFill_triggered();

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

	void on_actionExport_triggered();

	void on_actionTranslations_triggered();
	
	void on_actionh3m_converter_triggered();

	void on_actionLock_triggered();

	void on_actionUnlock_triggered();

	void on_actionZoom_in_triggered();

	void on_actionZoom_out_triggered();

	void on_actionZoom_reset_triggered();

	void on_toolLine_toggled(bool checked);

	void on_toolBrush2_toggled(bool checked);

	void on_toolBrush_toggled(bool checked);

	void on_toolBrush4_toggled(bool checked);

	void on_toolLasso_toggled(bool checked);

	void on_toolArea_toggled(bool checked);

	void on_toolFill_toggled(bool checked);

	void on_toolSelect_toggled(bool checked);

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
	QString lastSavingDir;
	bool unsaved = false;

	QStandardItemModel objectsModel;

	int mapLevel = 0;
	QRectF initialScale;

	std::set<int> catalog;

	// command line options
	QString mapFilePath;			// FilePath to the H3 or VCMI map to open
};
