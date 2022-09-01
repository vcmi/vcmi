#include "StdInc.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication.h>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>

#include "../lib/VCMIDirs.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CConfigHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/GameConstants.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/Terrain.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"


#include "CGameInfo.h"
#include "maphandler.h"
#include "graphics.h"
#include "windownewmap.h"

static CBasicLogConfigurator * logConfig;

void init()
{
	loadDLLClasses();
	const_cast<CGameInfo*>(CGI)->setFromLib();
	logGlobal->info("Initializing VCMI_Lib");
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	ui->mapView->setMain(this);

	//configure logging
	const boost::filesystem::path logPath = VCMIDirs::get().userCachePath() / "VCMI_Editor_log.txt";
	console = new CConsoleHandler();
	logConfig = new CBasicLogConfigurator(logPath, console);
	logConfig->configureDefault();
	logGlobal->info("The log file will be saved to %s", logPath);
	
	//init
	preinitDLL(::console);
	settings.init();
	
	// Initialize logging based on settings
	logConfig->configure();
	logGlobal->debug("settings = %s", settings.toJsonNode().toJson());
	
	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message) -> bool
	{
		if (CResourceHandler::get()->existsResource(ResourceID(filename)))
			return true;
		
		logGlobal->error("Error: %s was not found!", message);
		return false;
	};
	
	if(!testFile("DATA/HELP.TXT", "Heroes III data") ||
	   !testFile("MODS/VCMI/MOD.JSON", "VCMI data"))
	{
		QApplication::quit();
	}
	
	conf.init();
	logGlobal->info("Loading settings");
	
	CGI = new CGameInfo(); //contains all global informations about game (texts, lodHandlers, map handler etc.)
	init();
	
	graphics = new Graphics(); // should be before curh->init()
	graphics->load();//must be after Content loading but should be in main thread
	
	
	if(!testFile("DATA/new-menu/Background.png", "Cannot find file"))
	{
		QApplication::quit();
	}
	
	//now let's try to draw
	auto resPath = *CResourceHandler::get()->getResourceName(ResourceID("DATA/new-menu/Background.png"));
	
	scenes[0] = new MapScene(this, 0);
	scenes[1] = new MapScene(this, 1);
	ui->mapView->setScene(scenes[0]);
	
	sceneMini = new QGraphicsScene(this);
	ui->minimapView->setScene(sceneMini);

	scenes[0]->addPixmap(QPixmap(QString::fromStdString(resPath.native())));

	//loading objects
	loadObjectsTree();

	show();

	setStatusMessage("privet");
}

MainWindow::~MainWindow()
{
    delete ui;
}

MapView * MainWindow::getMapView()
{
	return ui->mapView;
}

void MainWindow::setStatusMessage(const QString & status)
{
	statusBar()->showMessage(status);
}

void MainWindow::reloadMap(int level)
{
	//auto mapSizePx = mapHandler->surface.rect();
	//float ratio = std::fmin(mapSizePx.width() / 192., mapSizePx.height() / 192.);*/
	//minimap = mapHandler->surface;
	//minimap.setDevicePixelRatio(ratio);

	scenes[level]->updateViews();

	//sceneMini->clear();
	//sceneMini->addPixmap(minimap);
}

CMap * MainWindow::getMap()
{
	return map.get();
}

MapHandler * MainWindow::getMapHandler()
{
	return mapHandler.get();
}

void MainWindow::setMapRaw(std::unique_ptr<CMap> cmap)
{
	map = std::move(cmap);
}

void MainWindow::setMap(bool isNew)
{
	unsaved = isNew;
	if(isNew)
		filename.clear();

	setWindowTitle(filename + "* - VCMI Map Editor");

	mapHandler.reset(new MapHandler(map.get()));

	reloadMap();
	if(map->twoLevel)
		reloadMap(1);

	mapLevel = 0;
	ui->mapView->setScene(scenes[mapLevel]);
}

void MainWindow::on_actionOpen_triggered()
{
	auto filenameSelect = QFileDialog::getOpenFileName(this, tr("Open Image"), QString::fromStdString(VCMIDirs::get().userCachePath().native()), tr("Homm3 Files (*.vmap *.h3m)"));
	
	if(filenameSelect.isNull())
		return;
	
	QFileInfo fi(filenameSelect);
	std::string fname = fi.fileName().toStdString();
	std::string fdir = fi.dir().path().toStdString();
	ResourceID resId("MAPS/" + fname, EResType::MAP);
	//ResourceID resId("MAPS/SomeMap.vmap", EResType::MAP);
	
	if(!CResourceHandler::get()->existsResource(resId))
	{
		QMessageBox::information(this, "Failed to open map", "Only map folder is supported");
		return;
	}
	
	CMapService mapService;
	try
	{
		map = mapService.loadMap(resId);
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to open map", e.what());
	}

	setMap(false);
}

void MainWindow::saveMap()
{
	if(!map)
		return;

	if(!unsaved)
		return;

	CMapService mapService;
	try
	{
		mapService.saveMap(map, filename.toStdString());
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to save map", e.what());
	}

	unsaved = false;
	setWindowTitle(filename + " - VCMI Map Editor");
}

void MainWindow::on_actionSave_as_triggered()
{
	if(!map)
		return;

	auto filenameSelect = QFileDialog::getSaveFileName(this, tr("Save map"), "", tr("VCMI maps (*.vmap)"));

	if(filenameSelect.isNull())
		return;

	if(filenameSelect == filename)
		return;

	filename = filenameSelect;

	saveMap();
}


void MainWindow::on_actionNew_triggered()
{
	auto newMapDialog = new WindowNewMap(this);
}

void MainWindow::on_actionSave_triggered()
{
	if(!map)
		return;

	if(filename.isNull())
	{
		auto filenameSelect = QFileDialog::getSaveFileName(this, tr("Save map"), "", tr("VCMI maps (*.vmap)"));

		if(filenameSelect.isNull())
			return;

		filename = filenameSelect;
	}

	saveMap();
}

void MainWindow::terrainButtonClicked(Terrain terrain)
{
	std::vector<int3> v(scenes[mapLevel]->selectionTerrainView.selection().begin(), scenes[mapLevel]->selectionTerrainView.selection().end());
	if(v.empty())
		return;

	scenes[mapLevel]->selectionTerrainView.clear();
	scenes[mapLevel]->selectionTerrainView.draw();

	map->getEditManager()->getTerrainSelection().setSelection(v);
	map->getEditManager()->drawTerrain(terrain, &CRandomGenerator::getDefault());

	for(auto & t : v)
		scenes[mapLevel]->terrainView.setDirty(t);
	scenes[mapLevel]->terrainView.draw();
}

void MainWindow::loadObjectsTree()
{
	for(auto & terrain : Terrain::Manager::terrains())
	{
		QPushButton *b = new QPushButton(QString::fromStdString(terrain));
		ui->terrainLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, terrain]{ terrainButtonClicked(terrain); });
	}
	ui->terrainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
	/*
	createHandler(bth, "Bonus type", pomtime);
	createHandler(generaltexth, "General text", pomtime);
	createHandler(heroh, "Hero", pomtime);
	createHandler(arth, "Artifact", pomtime);
	createHandler(creh, "Creature", pomtime);
	createHandler(townh, "Town", pomtime);
	createHandler(objh, "Object", pomtime);
	createHandler(objtypeh, "Object types information", pomtime);
	createHandler(spellh, "Spell", pomtime);
	createHandler(skillh, "Skill", pomtime);
	createHandler(terviewh, "Terrain view pattern", pomtime);
	createHandler(tplh, "Template", pomtime); //templates need already resolved identifiers (refactor?)
	createHandler(scriptHandler, "Script", pomtime);
	createHandler(battlefieldsHandler, "Battlefields", pomtime);
	createHandler(obstacleHandler, "Obstacles", pomtime);*/

	std::map<std::string, std::vector<std::string>> identifiers;

	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		//QList<QStandardItem*> objTypes;

		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			//QList<QStandardItem*> objSubTypes;
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);

			/*if(handler->isStaticObject())
			{
				for(auto temp : handler->getTemplates())
				{

				}
			}*/

			identifiers[handler->typeName].push_back(handler->subTypeName);
		}
	}

	objectsModel.setHorizontalHeaderLabels(QStringList() << QStringLiteral("Type"));
	QList<QStandardItem*> objTypes;
	for(auto & el1 : identifiers)
	{
		auto * objTypei = new QStandardItem(QString::fromStdString(el1.first));
		for(auto & el2 : el1.second)
		{
			objTypei->appendRow(new QStandardItem(QString::fromStdString(el2)));
		}
		objectsModel.appendRow(objTypei);
	}
	ui->treeView->setModel(&objectsModel);
}

void MainWindow::on_actionLevel_triggered()
{
	if(map && map->twoLevel)
	{
		mapLevel = mapLevel ? 0 : 1;
		ui->mapView->setScene(scenes[mapLevel]);
	}
}

void MainWindow::on_actionPass_triggered(bool checked)
{
	if(map)
	{
		scenes[0]->passabilityView.show(checked);
		scenes[1]->passabilityView.show(checked);
	}
}


void MainWindow::on_actionGrid_triggered(bool checked)
{
	if(map)
	{
		scenes[0]->gridView.show(checked);
		scenes[1]->gridView.show(checked);
	}
}


void MainWindow::on_toolBrush_clicked(bool checked)
{
	//ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush;
	}
	else
	{
		ui->mapView->selectionTool = MapView::SelectionTool::None;
	}
}


void MainWindow::on_toolArea_clicked(bool checked)
{
	ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	//ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Area;
	}
	else
	{
		ui->mapView->selectionTool = MapView::SelectionTool::None;
	}
}

