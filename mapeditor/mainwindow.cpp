#include "StdInc.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

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
#include "objectbrowser.h"
#include "inspector.h"
#include "mapsettings.h"
#include "playersettings.h"
#include "validator.h"

static CBasicLogConfigurator * logConfig;

QJsonValue jsonFromPixmap(const QPixmap &p)
{
  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);
  p.save(&buffer, "PNG");
  auto const encoded = buffer.data().toBase64();
  return {QLatin1String(encoded)};
}

QPixmap pixmapFromJson(const QJsonValue &val)
{
  auto const encoded = val.toString().toLatin1();
  QPixmap p;
  p.loadFromData(QByteArray::fromBase64(encoded), "PNG");
  return p;
}

void init()
{
	loadDLLClasses();
	const_cast<CGameInfo*>(CGI)->setFromLib();
	logGlobal->info("Initializing VCMI_Lib");
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	controller(this)
{
	ui->setupUi(this);
	setTitle();
	
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

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
	//auto resPath = *CResourceHandler::get()->getResourceName(ResourceID("DATA/new-menu/Background.png"));
	
	ui->mapView->setScene(controller.scene(0));
	ui->mapView->setController(&controller);
	connect(ui->mapView, &MapView::openObjectProperties, this, &MainWindow::loadInspector);
	
	ui->minimapView->setScene(controller.miniScene(0));
	ui->minimapView->setController(&controller);
	connect(ui->minimapView, &MinimapView::cameraPositionChanged, ui->mapView, &MapView::cameraChanged);

	scenePreview = new QGraphicsScene(this);
	ui->objectPreview->setScene(scenePreview);

	//scenes[0]->addPixmap(QPixmap(QString::fromStdString(resPath.native())));

	//loading objects
	loadObjectsTree();

	show();
}

MainWindow::~MainWindow()
{
    delete ui;
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
	
	controller.sceneForceUpdate(level);

	//sceneMini->clear();
	//sceneMini->addPixmap(minimap);
}

void MainWindow::setTitle()
{
	QString title = QString("%1%2 - %3 (v%4)").arg(filename, unsaved ? "*" : "", VCMI_EDITOR_NAME, VCMI_EDITOR_VERSION);
	setWindowTitle(title);
}

void MainWindow::mapChanged()
{
	unsaved = true;
	setTitle();
}

void MainWindow::initializeMap(bool isNew)
{
	unsaved = isNew;
	if(isNew)
		filename.clear();
	setTitle();

	mapLevel = 0;
	ui->mapView->setScene(controller.scene(mapLevel));
	ui->minimapView->setScene(controller.miniScene(mapLevel));
	ui->minimapView->dimensions();
	
	setStatusMessage(QString("Scene objects: %1").arg(ui->mapView->scene()->items().size()));

	//enable settings
	ui->actionMapSettings->setEnabled(true);
	ui->actionPlayers_settings->setEnabled(true);
}

void MainWindow::on_actionOpen_triggered()
{
	auto filenameSelect = QFileDialog::getOpenFileName(this, tr("Open Image"), QString::fromStdString(VCMIDirs::get().userCachePath().make_preferred().string()), tr("Homm3 Files (*.vmap *.h3m)"));
	
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
		controller.setMap(mapService.loadMap(resId));
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to open map", e.what());
	}


	filename = filenameSelect;
	initializeMap(controller.map()->version != EMapFormat::VCMI);
}

void MainWindow::saveMap()
{
	if(!controller.map())
		return;

	if(!unsaved)
		return;
	
	//validate map
	auto issues = Validator::validate(controller.map());
	bool critical = false;
	for(auto & issue : issues)
		critical |= issue.critical;
	
	if(!issues.empty())
	{
		if(critical)
			QMessageBox::warning(this, "Map validation", "Map has critical problems and most probably will not be playable. Open Validator from the Map menu to see issues found");
		else
			QMessageBox::information(this, "Map validation", "Map has some errors. Open Validator from the Map menu to see issues found");
	}

	CMapService mapService;
	try
	{
		mapService.saveMap(controller.getMapUniquePtr(), filename.toStdString());
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to save map", e.what());
	}

	unsaved = false;
	setTitle();
}

void MainWindow::on_actionSave_as_triggered()
{
	if(!controller.map())
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
	new WindowNewMap(this);
}

void MainWindow::on_actionSave_triggered()
{
	if(!controller.map())
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
	controller.commitTerrainChange(mapLevel, terrain);
}

void MainWindow::addGroupIntoCatalog(const std::string & groupName, bool staticOnly)
{
	auto knownObjects = VLC->objtypeh->knownObjects();
	for(auto ID : knownObjects)
	{
		if(catalog.count(ID))
			continue;

		addGroupIntoCatalog(groupName, staticOnly, ID);
	}
}

void MainWindow::addGroupIntoCatalog(const std::string & groupName, bool staticOnly, int ID)
{
	QStandardItem * itemGroup = nullptr;
	auto itms = objectsModel.findItems(QString::fromStdString(groupName));
	if(itms.empty())
	{
		itemGroup = new QStandardItem(QString::fromStdString(groupName));
		objectsModel.appendRow(itemGroup);
	}
	else
	{
		itemGroup = itms.front();
	}

	auto knownSubObjects = VLC->objtypeh->knownSubObjects(ID);
	bool singleSubObject = knownSubObjects.size() == 1;
	for(auto secondaryID : knownSubObjects)
	{
		auto factory = VLC->objtypeh->getHandlerFor(ID, secondaryID);
		auto templates = factory->getTemplates();
		bool singleTemplate = templates.size() == 1;
		if(staticOnly && !factory->isStaticObject())
			continue;

		auto * itemType = new QStandardItem(QString::fromStdString(factory->subTypeName));
		for(int templateId = 0; templateId < templates.size(); ++templateId)
		{
			auto templ = templates[templateId];

			//selecting file
			const std::string & afile = templ.editorAnimationFile.empty() ? templ.animationFile : templ.editorAnimationFile;

			//creating picture
			QPixmap preview(128, 128);
			preview.fill(QColor(255, 255, 255));
			QPainter painter(&preview);
			Animation animation(afile);
			animation.preload();
			auto picture = animation.getImage(0);
			if(picture && picture->width() && picture->height())
			{
				qreal xscale = qreal(128) / qreal(picture->width()), yscale = qreal(128) / qreal(picture->height());
				qreal scale = std::min(xscale, yscale);
				painter.scale(scale, scale);
				painter.drawImage(QPoint(0, 0), *picture);
			}

			//add parameters
			QJsonObject data{{"id", QJsonValue(ID)},
							 {"subid", QJsonValue(secondaryID)},
							 {"template", QJsonValue(templateId)},
							 {"animationEditor", QString::fromStdString(templ.editorAnimationFile)},
							 {"animation", QString::fromStdString(templ.animationFile)},
							 {"preview", jsonFromPixmap(preview)}};

			//do not have extra level
			if(singleTemplate)
			{
				itemType->setIcon(QIcon(preview));
				itemType->setData(data);
			}
			else
			{
				auto * item = new QStandardItem(QIcon(preview), QString::fromStdString(templ.stringID));
				item->setData(data);
				itemType->appendRow(item);
			}
		}
		itemGroup->appendRow(itemType);
		catalog.insert(ID);
	}
}

void MainWindow::loadObjectsTree()
{
	ui->terrainFilterCombo->addItem("");
	//adding terrains
	for(auto & terrain : Terrain::Manager::terrains())
	{
		QPushButton *b = new QPushButton(QString::fromStdString(terrain));
		ui->terrainLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, terrain]{ terrainButtonClicked(terrain); });

		//filter
		ui->terrainFilterCombo->addItem(QString::fromStdString(terrain));
	}

	//add spacer to keep terrain button on the top
	ui->terrainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	if(objectBrowser)
		throw std::runtime_error("object browser exists");

	//model
	objectsModel.setHorizontalHeaderLabels(QStringList() << QStringLiteral("Type"));
	objectBrowser = new ObjectBrowser(this);
	objectBrowser->setSourceModel(&objectsModel);
	objectBrowser->setDynamicSortFilter(false);
	objectBrowser->setRecursiveFilteringEnabled(true);
	ui->treeView->setModel(objectBrowser);
	ui->treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
	ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(treeViewSelected(const QModelIndex &, const QModelIndex &)));


	//adding objects
	addGroupIntoCatalog("TOWNS", false, Obj::TOWN);
	addGroupIntoCatalog("TOWNS", false, Obj::RANDOM_TOWN);
	addGroupIntoCatalog("TOWNS", false, Obj::SHIPYARD);
	addGroupIntoCatalog("TOWNS", false, Obj::GARRISON);
	addGroupIntoCatalog("TOWNS", false, Obj::GARRISON2);
	addGroupIntoCatalog("MISC", false, Obj::ALTAR_OF_SACRIFICE);
	addGroupIntoCatalog("MISC", false, Obj::ARENA);
	addGroupIntoCatalog("MISC", false, Obj::BLACK_MARKET);
	addGroupIntoCatalog("MISC", false, Obj::BORDERGUARD);
	addGroupIntoCatalog("MISC", false, Obj::KEYMASTER);
	addGroupIntoCatalog("MISC", false, Obj::BUOY);
	addGroupIntoCatalog("MISC", false, Obj::CAMPFIRE);
	addGroupIntoCatalog("MISC", false, Obj::CARTOGRAPHER);
	addGroupIntoCatalog("MISC", false, Obj::SWAN_POND);
	addGroupIntoCatalog("MISC", false, Obj::COVER_OF_DARKNESS);
	addGroupIntoCatalog("MISC", false, Obj::CORPSE);
	addGroupIntoCatalog("MISC", false, Obj::MARLETTO_TOWER);
	addGroupIntoCatalog("MISC", false, Obj::DERELICT_SHIP);
	addGroupIntoCatalog("MISC", false, Obj::FAERIE_RING);
	addGroupIntoCatalog("MISC", false, Obj::FLOTSAM);
	addGroupIntoCatalog("MISC", false, Obj::FOUNTAIN_OF_FORTUNE);
	addGroupIntoCatalog("MISC", false, Obj::FOUNTAIN_OF_YOUTH);
	addGroupIntoCatalog("MISC", false, Obj::GARDEN_OF_REVELATION);
	addGroupIntoCatalog("MISC", false, Obj::HILL_FORT);
	addGroupIntoCatalog("MISC", false, Obj::IDOL_OF_FORTUNE);
	addGroupIntoCatalog("MISC", false, Obj::LIBRARY_OF_ENLIGHTENMENT);
	addGroupIntoCatalog("MISC", false, Obj::LIGHTHOUSE);
	addGroupIntoCatalog("MISC", false, Obj::SCHOOL_OF_MAGIC);
	addGroupIntoCatalog("MISC", false, Obj::MAGIC_SPRING);
	addGroupIntoCatalog("MISC", false, Obj::MAGIC_WELL);
	addGroupIntoCatalog("MISC", false, Obj::MERCENARY_CAMP);
	addGroupIntoCatalog("MISC", false, Obj::MERMAID);
	addGroupIntoCatalog("MISC", false, Obj::MYSTICAL_GARDEN);
	addGroupIntoCatalog("MISC", false, Obj::OASIS);
	addGroupIntoCatalog("MISC", false, Obj::OBELISK);
	addGroupIntoCatalog("MISC", false, Obj::REDWOOD_OBSERVATORY);
	addGroupIntoCatalog("MISC", false, Obj::OCEAN_BOTTLE);
	addGroupIntoCatalog("MISC", false, Obj::PILLAR_OF_FIRE);
	addGroupIntoCatalog("MISC", false, Obj::STAR_AXIS);
	addGroupIntoCatalog("MISC", false, Obj::RALLY_FLAG);
	addGroupIntoCatalog("MISC", false, Obj::LEAN_TO);
	addGroupIntoCatalog("MISC", false, Obj::WATERING_HOLE);
	addGroupIntoCatalog("PRISON", false, Obj::PRISON);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::ARTIFACT);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::RANDOM_ART);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::RANDOM_TREASURE_ART);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::RANDOM_MINOR_ART);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::RANDOM_MAJOR_ART);
	addGroupIntoCatalog("ARTIFACTS", false, Obj::RANDOM_RELIC_ART);
	addGroupIntoCatalog("RESOURCES", false, Obj::PANDORAS_BOX);
	addGroupIntoCatalog("RESOURCES", false, Obj::RANDOM_RESOURCE);
	addGroupIntoCatalog("RESOURCES", false, Obj::RESOURCE);
	addGroupIntoCatalog("RESOURCES", false, Obj::SEA_CHEST);
	addGroupIntoCatalog("RESOURCES", false, Obj::TREASURE_CHEST);
	addGroupIntoCatalog("RESOURCES", false, Obj::SPELL_SCROLL);
	addGroupIntoCatalog("BANKS", false, Obj::CREATURE_BANK);
	addGroupIntoCatalog("BANKS", false, Obj::DRAGON_UTOPIA);
	addGroupIntoCatalog("DWELLINGS", false, Obj::CREATURE_GENERATOR1);
	addGroupIntoCatalog("DWELLINGS", false, Obj::CREATURE_GENERATOR2);
	addGroupIntoCatalog("DWELLINGS", false, Obj::CREATURE_GENERATOR3);
	addGroupIntoCatalog("DWELLINGS", false, Obj::CREATURE_GENERATOR4);
	addGroupIntoCatalog("DWELLINGS", false, Obj::RANDOM_DWELLING);
	addGroupIntoCatalog("DWELLINGS", false, Obj::RANDOM_DWELLING_LVL);
	addGroupIntoCatalog("DWELLINGS", false, Obj::RANDOM_DWELLING_FACTION);
	addGroupIntoCatalog("GROUNDS", false, Obj::CURSED_GROUND1);
	addGroupIntoCatalog("GROUNDS", false, Obj::MAGIC_PLAINS1);
	addGroupIntoCatalog("GROUNDS", false, Obj::CLOVER_FIELD);
	addGroupIntoCatalog("GROUNDS", false, Obj::CURSED_GROUND2);
	addGroupIntoCatalog("GROUNDS", false, Obj::EVIL_FOG);
	addGroupIntoCatalog("GROUNDS", false, Obj::FAVORABLE_WINDS);
	addGroupIntoCatalog("GROUNDS", false, Obj::FIERY_FIELDS);
	addGroupIntoCatalog("GROUNDS", false, Obj::HOLY_GROUNDS);
	addGroupIntoCatalog("GROUNDS", false, Obj::LUCID_POOLS);
	addGroupIntoCatalog("GROUNDS", false, Obj::MAGIC_CLOUDS);
	addGroupIntoCatalog("GROUNDS", false, Obj::MAGIC_PLAINS2);
	addGroupIntoCatalog("GROUNDS", false, Obj::ROCKLANDS);
	addGroupIntoCatalog("TELEPORTS", false, Obj::MONOLITH_ONE_WAY_ENTRANCE);
	addGroupIntoCatalog("TELEPORTS", false, Obj::MONOLITH_ONE_WAY_EXIT);
	addGroupIntoCatalog("TELEPORTS", false, Obj::MONOLITH_TWO_WAY);
	addGroupIntoCatalog("TELEPORTS", false, Obj::SUBTERRANEAN_GATE);
	addGroupIntoCatalog("TELEPORTS", false, Obj::WHIRLPOOL);
	addGroupIntoCatalog("MINES", false, Obj::MINE);
	addGroupIntoCatalog("MINES", false, Obj::ABANDONED_MINE);
	addGroupIntoCatalog("MINES", false, Obj::WINDMILL);
	addGroupIntoCatalog("MINES", false, Obj::WATER_WHEEL);
	addGroupIntoCatalog("TRIGGERS", false, Obj::EVENT);
	addGroupIntoCatalog("TRIGGERS", false, Obj::GRAIL);
	addGroupIntoCatalog("TRIGGERS", false, Obj::SIGN);
	addGroupIntoCatalog("MONSTERS", false, Obj::MONSTER);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L1);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L2);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L3);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L4);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L5);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L6);
	addGroupIntoCatalog("MONSTERS", false, Obj::RANDOM_MONSTER_L7);
	addGroupIntoCatalog("QUESTS", false, Obj::SEER_HUT);
	addGroupIntoCatalog("QUESTS", false, Obj::BORDER_GATE);
	addGroupIntoCatalog("QUESTS", false, Obj::QUEST_GUARD);
	addGroupIntoCatalog("QUESTS", false, Obj::HUT_OF_MAGI);
	addGroupIntoCatalog("QUESTS", false, Obj::EYE_OF_MAGI);
	addGroupIntoCatalog("OBSTACLES", true);
	addGroupIntoCatalog("OTHER", false);


	/*
		HERO = 34,
		LEAN_TO = 39,
		PYRAMID = 63,//subtype 0
		WOG_OBJECT = 63,//subtype > 0
		RANDOM_HERO = 70,
		REFUGEE_CAMP = 78,
		SANCTUARY = 80,
		SCHOLAR = 81,
		CRYPT = 84,
		SHIPWRECK = 85,
		SHIPWRECK_SURVIVOR = 86,
		SHRINE_OF_MAGIC_INCANTATION = 88,
		SHRINE_OF_MAGIC_GESTURE = 89,
		SHRINE_OF_MAGIC_THOUGHT = 90,
		SIRENS = 92,
		STABLES = 94,
		TAVERN = 95,
		TEMPLE = 96,
		DEN_OF_THIEVES = 97,
		TRADING_POST = 99,
		LEARNING_STONE = 100,
		TREE_OF_KNOWLEDGE = 102,
		UNIVERSITY = 104,
		WAGON = 105,
		WAR_MACHINE_FACTORY = 106,
		SCHOOL_OF_WAR = 107,
		WARRIORS_TOMB = 108,
		WITCH_HUT = 113,
		HOLE = 124,
		FREELANCERS_GUILD = 213,
		HERO_PLACEHOLDER = 214,
		TRADING_POST_SNOW = 221,
*/
}

void MainWindow::on_actionLevel_triggered()
{
	if(controller.map() && controller.map()->twoLevel)
	{
		mapLevel = mapLevel ? 0 : 1;
		ui->mapView->setScene(controller.scene(mapLevel));
		ui->minimapView->setScene(controller.miniScene(mapLevel));
	}
}

void MainWindow::on_actionUndo_triggered()
{
	QString str("Undo clicked");
	statusBar()->showMessage(str, 1000);

	if (controller.map())
	{
		controller.undo();
	}
}

void MainWindow::on_actionRedo_triggered()
{
	QString str("Redo clicked");
	statusBar()->showMessage(str, 1000);

	if (controller.map())
	{
		controller.redo();
	}
}

void MainWindow::on_actionPass_triggered(bool checked)
{
	if(controller.map())
	{
		controller.scene(0)->passabilityView.show(checked);
		controller.scene(1)->passabilityView.show(checked);
	}
}


void MainWindow::on_actionGrid_triggered(bool checked)
{
	if(controller.map())
	{
		controller.scene(0)->gridView.show(checked);
		controller.scene(0)->gridView.show(checked);
	}
}

void MainWindow::changeBrushState(int idx)
{

}

void MainWindow::on_toolBrush_clicked(bool checked)
{
	//ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
		ui->mapView->selectionTool = MapView::SelectionTool::Brush;
	else
		ui->mapView->selectionTool = MapView::SelectionTool::None;
}

void MainWindow::on_toolBrush2_clicked(bool checked)
{
	ui->toolBrush->setChecked(false);
	//ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
		ui->mapView->selectionTool = MapView::SelectionTool::Brush2;
	else
		ui->mapView->selectionTool = MapView::SelectionTool::None;
}


void MainWindow::on_toolBrush4_clicked(bool checked)
{
	ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	//ui->toolBrush4->setChecked(false);
	ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
		ui->mapView->selectionTool = MapView::SelectionTool::Brush4;
	else
		ui->mapView->selectionTool = MapView::SelectionTool::None;
}

void MainWindow::on_toolArea_clicked(bool checked)
{
	ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	//ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);

	if(checked)
		ui->mapView->selectionTool = MapView::SelectionTool::Area;
	else
		ui->mapView->selectionTool = MapView::SelectionTool::None;
}

void MainWindow::on_actionErase_triggered()
{
	on_toolErase_clicked();
}

void MainWindow::on_toolErase_clicked()
{
	if(controller.map())
	{
		controller.commitObjectErase(mapLevel);
	}
}

void MainWindow::preparePreview(const QModelIndex &index, bool createNew)
{
	scenePreview->clear();

	auto data = objectsModel.itemFromIndex(objectBrowser->mapToSource(index))->data().toJsonObject();

	if(!data.empty())
	{
		auto preview = data["preview"];
		if(preview != QJsonValue::Undefined)
		{
			QPixmap objPreview = pixmapFromJson(preview);
			scenePreview->addPixmap(objPreview);

			auto objId = data["id"].toInt();
			auto objSubId = data["subid"].toInt();
			auto templateId = data["template"].toInt();

			if(controller.discardObject(mapLevel) || createNew)
			{
				auto factory = VLC->objtypeh->getHandlerFor(objId, objSubId);
				auto templ = factory->getTemplates()[templateId];
				controller.createObject(mapLevel, factory->create(templ));
			}
		}
	}
}


void MainWindow::treeViewSelected(const QModelIndex & index, const QModelIndex & deselected)
{
	preparePreview(index, false);
}


void MainWindow::on_treeView_activated(const QModelIndex &index)
{
	ui->toolBrush->setChecked(false);
	ui->toolBrush2->setChecked(false);
	ui->toolBrush4->setChecked(false);
	ui->toolArea->setChecked(false);
	ui->toolLasso->setChecked(false);
	ui->mapView->selectionTool = MapView::SelectionTool::None;

	preparePreview(index, true);
}


void MainWindow::on_terrainFilterCombo_currentTextChanged(const QString &arg1)
{
	if(!objectBrowser)
		return;

	objectBrowser->terrain = arg1.isEmpty() ? Terrain::ANY : Terrain(arg1.toStdString());
	objectBrowser->invalidate();
	objectBrowser->sort(0);
}


void MainWindow::on_filter_textChanged(const QString &arg1)
{
	if(!objectBrowser)
		return;

	objectBrowser->filter = arg1;
	objectBrowser->invalidate();
	objectBrowser->sort(0);
}


void MainWindow::on_actionFill_triggered()
{
	if(!controller.map())
		return;

	controller.commitObstacleFill(mapLevel);
}

void MainWindow::loadInspector(CGObjectInstance * obj)
{
	Inspector inspector(obj, ui->inspectorWidget);
	inspector.updateProperties();
}

void MainWindow::on_inspectorWidget_itemChanged(QTableWidgetItem *item)
{
	if(!item->isSelected())
		return;

	int r = item->row();
	int c = item->column();
	if(c < 1)
		return;

	auto * tableWidget = item->tableWidget();

	//get identifier
	auto identifier = tableWidget->item(0, 1)->text();
	static_assert(sizeof(CGObjectInstance *) == sizeof(decltype(identifier.toLongLong())),
			"Compilied for 64 bit arcitecture. Use .toInt() method");

	CGObjectInstance * obj = reinterpret_cast<CGObjectInstance *>(identifier.toLongLong());

	//get parameter name
	auto param = tableWidget->item(r, c - 1)->text();

	//set parameter
	Inspector inspector(obj, tableWidget);
	inspector.setProperty(param, item->text());
	controller.commitObjectChange(mapLevel);
}

void MainWindow::on_actionMapSettings_triggered()
{
	auto settingsDialog = new MapSettings(controller, this);
	settingsDialog->setWindowModality(Qt::WindowModal);
	settingsDialog->setModal(true);
}


void MainWindow::on_actionPlayers_settings_triggered()
{
	auto settingsDialog = new PlayerSettings(controller, this);
	settingsDialog->setWindowModality(Qt::WindowModal);
	settingsDialog->setModal(true);
}

void MainWindow::enableUndo(bool enable)
{
	ui->actionUndo->setEnabled(enable);
}

void MainWindow::enableRedo(bool enable)
{
	ui->actionRedo->setEnabled(enable);
}

void MainWindow::onSelectionMade(int level, bool anythingSelected)
{
	if (level == mapLevel)
	{
		auto info = QString::asprintf("Selection on layer %s: %s", level, anythingSelected ? "true" : "false");
		setStatusMessage(info);

		ui->actionErase->setEnabled(anythingSelected);
		ui->toolErase->setEnabled(anythingSelected);
	}
}

void MainWindow::on_actionValidate_triggered()
{
	new Validator(controller.map(), this);
}

