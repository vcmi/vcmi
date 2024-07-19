/*
 * mainwindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/RoadHandler.h"
#include "../lib/RiverHandler.h"
#include "../lib/TerrainHandler.h"
#include "../lib/filesystem/CFilesystemLoader.h"

#include "maphandler.h"
#include "graphics.h"
#include "windownewmap.h"
#include "objectbrowser.h"
#include "inspector/inspector.h"
#include "mapsettings/mapsettings.h"
#include "mapsettings/translations.h"
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

	Settings config = settings.write["session"]["editor"];
	config->Bool() = true;

	logGlobal->info("Initializing VCMI_Lib");
}

void MainWindow::loadUserSettings()
{
	//load window settings
	QSettings s(Ui::teamName, Ui::appName);

	auto size = s.value(mainWindowSizeSetting).toSize();
	if (size.isValid())
	{
		resize(size);
	}
	auto position = s.value(mainWindowPositionSetting).toPoint();
	if (!position.isNull())
	{
		move(position);
	}
	lastSavingDir = s.value(lastDirectorySetting).toString();
	if(lastSavingDir.isEmpty())
		lastSavingDir = QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string());
}

void MainWindow::saveUserSettings()
{
	QSettings s(Ui::teamName, Ui::appName);
	s.setValue(mainWindowSizeSetting, size());
	s.setValue(mainWindowPositionSetting, pos());
	s.setValue(lastDirectorySetting, lastSavingDir);
}

void MainWindow::parseCommandLine(ExtractionOptions & extractionOptions)
{
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addPositionalArgument("map", QCoreApplication::translate("main", "Filepath of the map to open."));

	parser.addOptions({
		{"e", QCoreApplication::translate("main", "Extract original H3 archives into a separate folder.")},
		{"s", QCoreApplication::translate("main", "From an extracted archive, it Splits TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44 into individual PNG's.")},
		{"c", QCoreApplication::translate("main", "From an extracted archive, Converts single Images (found in Images folder) from .pcx to png.")},
		{"d", QCoreApplication::translate("main", "Delete original files, for the ones split / converted.")},
		});

	parser.process(qApp->arguments());

	const QStringList positionalArgs = parser.positionalArguments();

	if(!positionalArgs.isEmpty())
		mapFilePath = positionalArgs.at(0);

	extractionOptions = {
		parser.isSet("e"), {
			parser.isSet("s"),
			parser.isSet("c"),
			parser.isSet("d")}};
}

void MainWindow::loadTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	const std::string translationFile = settings["general"]["language"].String()+ ".qm";
	QString translationFileResourcePath = QString{":/translation/%1"}.arg(translationFile.c_str());

	logGlobal->info("Loading translation %s", translationFile);

	if(!QFile::exists(translationFileResourcePath))
	{
		logGlobal->debug("Translation file %s does not exist", translationFileResourcePath.toStdString());
		return;
	}

	if (!translator.load(translationFileResourcePath))
	{
		logGlobal->error("Failed to load translation file %s", translationFileResourcePath.toStdString());
		return;
	}

	if(translationFile == "english.qm")
	{
		// translator doesn't need to be installed for English
		return;
	}

	if (!qApp->installTranslator(&translator))
	{
		logGlobal->error("Failed to install translator for translation file %s", translationFileResourcePath.toStdString());
	}
#endif
}

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	controller(this)
{
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

	new QShortcut(QKeySequence("Backspace"), this, SLOT(on_actionErase_triggered()));

	ExtractionOptions extractionOptions;
	parseCommandLine(extractionOptions);

	//configure logging
	const boost::filesystem::path logPath = VCMIDirs::get().userLogsPath() / "VCMI_Editor_log.txt";
	console = new CConsoleHandler();
	logConfig = new CBasicLogConfigurator(logPath, console);
	logConfig->configureDefault();
	logGlobal->info("The log file will be saved to %s", logPath);

	//init
	preinitDLL(::console, extractionOptions.extractArchives);

	// Initialize logging based on settings
	logConfig->configure();
	logGlobal->debug("settings = %s", settings.toJsonNode().toString());

	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message) -> bool
	{
		if (CResourceHandler::get()->existsResource(ResourcePath(filename)))
			return true;

		logGlobal->error("Error: %s was not found!", message);
		return false;
	};

	if (!testFile("DATA/HELP.TXT", "Heroes III data") ||
		!testFile("MODS/VCMI/MOD.JSON", "VCMI data"))
	{
		QApplication::quit();
	}

	loadTranslation();

	ui->setupUi(this);

	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->toolBrush->setIcon(QIcon{":/icons/brush-1.png"});
	ui->toolBrush2->setIcon(QIcon{":/icons/brush-2.png"});
	ui->toolBrush4->setIcon(QIcon{":/icons/brush-4.png"});
	ui->toolLasso->setIcon(QIcon{":/icons/tool-lasso.png"});
	ui->toolLine->setIcon(QIcon{":/icons/tool-line.png"});
	ui->toolArea->setIcon(QIcon{":/icons/tool-area.png"});
	ui->toolFill->setIcon(QIcon{":/icons/tool-fill.png"});
	ui->toolSelect->setIcon(QIcon{":/icons/tool-select.png"});
	ui->actionOpen->setIcon(QIcon{":/icons/document-open.png"});
	ui->actionSave->setIcon(QIcon{":/icons/document-save.png"});
	ui->actionNew->setIcon(QIcon{":/icons/document-new.png"});
	ui->actionLevel->setIcon(QIcon{":/icons/toggle-underground.png"});
	ui->actionPass->setIcon(QIcon{":/icons/toggle-pass.png"});
	ui->actionCut->setIcon(QIcon{":/icons/edit-cut.png"});
	ui->actionCopy->setIcon(QIcon{":/icons/edit-copy.png"});
	ui->actionPaste->setIcon(QIcon{":/icons/edit-paste.png"});
	ui->actionFill->setIcon(QIcon{":/icons/fill-obstacles.png"});
	ui->actionGrid->setIcon(QIcon{":/icons/toggle-grid.png"});
	ui->actionUndo->setIcon(QIcon{":/icons/edit-undo.png"});
	ui->actionRedo->setIcon(QIcon{":/icons/edit-redo.png"});
	ui->actionErase->setIcon(QIcon{":/icons/edit-clear.png"});
	ui->actionTranslations->setIcon(QIcon{":/icons/translations.png"});
	ui->actionLock->setIcon(QIcon{":/icons/lock-closed.png"});
	ui->actionUnlock->setIcon(QIcon{":/icons/lock-open.png"});
	ui->actionZoom_in->setIcon(QIcon{":/icons/zoom_plus.png"});
	ui->actionZoom_out->setIcon(QIcon{":/icons/zoom_minus.png"});
	ui->actionZoom_reset->setIcon(QIcon{":/icons/zoom_zero.png"});

	loadUserSettings(); //For example window size
	setTitle();

	init();

	graphics = new Graphics(); // should be before curh->init()
	graphics->load();//must be after Content loading but should be in main thread

	if (extractionOptions.extractArchives)
		ResourceConverter::convertExtractedResourceFiles(extractionOptions.conversionOptions);
	
	ui->mapView->setScene(controller.scene(0));
	ui->mapView->setController(&controller);
	ui->mapView->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);
	connect(ui->mapView, &MapView::openObjectProperties, this, &MainWindow::loadInspector);
	connect(ui->mapView, &MapView::currentCoordinates, this, &MainWindow::currentCoordinatesChanged);
	
	ui->minimapView->setScene(controller.miniScene(0));
	ui->minimapView->setController(&controller);
	connect(ui->minimapView, &MinimapView::cameraPositionChanged, ui->mapView, &MapView::cameraChanged);

	scenePreview = new QGraphicsScene(this);
	ui->objectPreview->setScene(scenePreview);

	//loading objects
	loadObjectsTree();
	
	ui->tabWidget->setCurrentIndex(0);
	
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT.getNum(); ++i)
	{
		connect(getActionPlayer(PlayerColor(i)), &QAction::toggled, this, [&, i](){switchDefaultPlayer(PlayerColor(i));});
	}
	connect(getActionPlayer(PlayerColor::NEUTRAL), &QAction::toggled, this, [&](){switchDefaultPlayer(PlayerColor::NEUTRAL);});
	onPlayersChanged();
	
	show();
	
	//Load map from command line
	if(!mapFilePath.isEmpty())
		openMap(mapFilePath);
}

MainWindow::~MainWindow()
{
	saveUserSettings(); //save window size etc.
	delete ui;
}

bool MainWindow::getAnswerAboutUnsavedChanges()
{
	if(unsaved)
	{
		auto sure = QMessageBox::question(this, tr("Confirmation"), tr("Unsaved changes will be lost, are you sure?"));
		if(sure == QMessageBox::No)
		{
			return false;
		}
	}
	return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if(getAnswerAboutUnsavedChanges())
		QMainWindow::closeEvent(event);
	else
		event->ignore();
}

void MainWindow::setStatusMessage(const QString & status)
{
	statusBar()->showMessage(status);
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
	if(initialScale.isValid())
		on_actionZoom_reset_triggered();
	initialScale = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	
	//enable settings
	ui->actionMapSettings->setEnabled(true);
	ui->actionPlayers_settings->setEnabled(true);
	ui->actionTranslations->setEnabled(true);
	ui->actionLevel->setEnabled(controller.map()->twoLevel);
	
	//set minimal players count
	if(isNew)
	{
		controller.map()->players[0].canComputerPlay = true;
		controller.map()->players[0].canHumanPlay = true;
	}
	
	onPlayersChanged();
}

std::unique_ptr<CMap> MainWindow::openMapInternal(const QString & filenameSelect)
{
	QFileInfo fi(filenameSelect);
	std::string fname = fi.fileName().toStdString();
	std::string fdir = fi.dir().path().toStdString();
	
	ResourcePath resId("MAPEDITOR/" + fname, EResType::MAP);
	
	//addFilesystem takes care about memory deallocation if case of failure, no memory leak here
	auto * mapEditorFilesystem = new CFilesystemLoader("MAPEDITOR/", fdir, 0);
	CResourceHandler::removeFilesystem("local", "mapEditor");
	CResourceHandler::addFilesystem("local", "mapEditor", mapEditorFilesystem);
	
	if(!CResourceHandler::get("mapEditor")->existsResource(resId))
		throw std::runtime_error("Cannot open map from this folder");
	
	CMapService mapService;
	if(auto header = mapService.loadMapHeader(resId))
	{
		auto missingMods = CMapService::verifyMapHeaderMods(*header);
		ModIncompatibility::ModList modList;
		for(const auto & m : missingMods)
			modList.push_back(m.second.name);
		
		if(!modList.empty())
			throw ModIncompatibility(modList);
		
		return mapService.loadMap(resId, nullptr);
	}
	else
		throw std::runtime_error("Corrupted map");
}

bool MainWindow::openMap(const QString & filenameSelect)
{
	try
	{
		controller.setMap(openMapInternal(filenameSelect));
	}
	catch(const ModIncompatibility & e)
	{
		assert(e.whatExcessive().empty());
		QMessageBox::warning(this, "Mods are required", QString::fromStdString(e.whatMissing()));
		return false;
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to open map", tr(e.what()));
		return false;
	}
	
	filename = filenameSelect;
	initializeMap(controller.map()->version != EMapFormat::VCMI);
	return true;
}

void MainWindow::on_actionOpen_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;
	
	auto filenameSelect = QFileDialog::getOpenFileName(this, tr("Open map"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()),
		tr("All supported maps (*.vmap *.h3m);;VCMI maps(*.vmap);;HoMM3 maps(*.h3m)"));
	if(filenameSelect.isEmpty())
		return;
	
	openMap(filenameSelect);
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
	
	Translations::cleanupRemovedItems(*controller.map());

	CMapService mapService;
	try
	{
		mapService.saveMap(controller.getMapUniquePtr(), filename.toStdString());
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, "Failed to save map", e.what());
		return;
	}
	
	unsaved = false;
	setTitle();
}

void MainWindow::on_actionSave_as_triggered()
{
	if(!controller.map())
		return;

	auto filenameSelect = QFileDialog::getSaveFileName(this, tr("Save map"), lastSavingDir, tr("VCMI maps (*.vmap)"));

	if(filenameSelect.isNull())
		return;

	QFileInfo fileInfo(filenameSelect);
	lastSavingDir = fileInfo.dir().path();

	if(fileInfo.suffix().toLower() != "vmap")
		filenameSelect += ".vmap";

	filename = filenameSelect;

	saveMap();
}


void MainWindow::on_actionNew_triggered()
{
	if(getAnswerAboutUnsavedChanges())
		new WindowNewMap(this);
}

void MainWindow::on_actionSave_triggered()
{
	if(!controller.map())
		return;

	if(filename.isNull())
		on_actionSave_as_triggered();
	else
		saveMap();
}

void MainWindow::currentCoordinatesChanged(int x, int y)
{
	setStatusMessage(QString("x: %1   y: %2").arg(x).arg(y));
}

void MainWindow::terrainButtonClicked(TerrainId terrain)
{
	controller.commitTerrainChange(mapLevel, terrain);
}

void MainWindow::roadOrRiverButtonClicked(ui8 type, bool isRoad)
{
	controller.commitRoadOrRiverChange(mapLevel, type, isRoad);
}

void MainWindow::addGroupIntoCatalog(const std::string & groupName, bool staticOnly)
{
	auto knownObjects = VLC->objtypeh->knownObjects();
	for(auto ID : knownObjects)
	{
		if(catalog.count(ID))
			continue;

		addGroupIntoCatalog(groupName, true, staticOnly, ID);
	}
}

void MainWindow::addGroupIntoCatalog(const std::string & groupName, bool useCustomName, bool staticOnly, int ID)
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

	if (VLC->objtypeh->knownObjects().count(ID) == 0)
		return;

	auto knownSubObjects = VLC->objtypeh->knownSubObjects(ID);
	for(auto secondaryID : knownSubObjects)
	{
		auto factory = VLC->objtypeh->getHandlerFor(ID, secondaryID);
		auto templates = factory->getTemplates();
		bool singleTemplate = templates.size() == 1;
		if(staticOnly && !factory->isStaticObject())
			continue;

		auto subGroupName = QString::fromStdString(VLC->objtypeh->getObjectName(ID, secondaryID));
		
		auto * itemType = new QStandardItem(subGroupName);
		for(int templateId = 0; templateId < templates.size(); ++templateId)
		{
			auto templ = templates[templateId];

			//selecting file
			const AnimationPath & afile = templ->editorAnimationFile.empty() ? templ->animationFile : templ->editorAnimationFile;

			//creating picture
			QPixmap preview(128, 128);
			preview.fill(QColor(255, 255, 255));
			QPainter painter(&preview);
			Animation animation(afile.getOriginalName());
			animation.preload();
			auto picture = animation.getImage(0);
			if(picture && picture->width() && picture->height())
			{
				qreal xscale = static_cast<qreal>(128) / static_cast<qreal>(picture->width());
				qreal yscale = static_cast<qreal>(128) / static_cast<qreal>(picture->height());
				qreal scale = std::min(xscale, yscale);
				painter.scale(scale, scale);
				painter.drawImage(QPoint(0, 0), *picture);
			}
			
			//create object to extract name
			std::unique_ptr<CGObjectInstance> temporaryObj(factory->create(nullptr, templ));
			QString translated = useCustomName ? QString::fromStdString(temporaryObj->getObjectName().c_str()) : subGroupName;
			itemType->setText(translated);
			
			//add parameters
			QJsonObject data{{"id", QJsonValue(ID)},
							 {"subid", QJsonValue(secondaryID)},
							 {"template", QJsonValue(templateId)},
							 {"animationEditor", QString::fromStdString(templ->editorAnimationFile.getOriginalName())},
							 {"animation", QString::fromStdString(templ->animationFile.getOriginalName())},
							 {"preview", jsonFromPixmap(preview)},
							 {"typeName", QString::fromStdString(factory->getJsonKey())}
			};

			//do not have extra level
			if(singleTemplate)
			{
				itemType->setIcon(QIcon(preview));
				itemType->setData(data);
			}
			else
			{
				auto * item = new QStandardItem(QIcon(preview), QString::fromStdString(templ->stringID));
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
	try
	{
	ui->terrainFilterCombo->addItem("");
	//adding terrains
	for(auto & terrain : VLC->terrainTypeHandler->objects)
	{
		auto *b = new QPushButton(QString::fromStdString(terrain->getNameTranslated()));
		ui->terrainLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, terrainID=terrain->getId()]{ terrainButtonClicked(terrainID); });

		//filter
		QString displayName = QString::fromStdString(terrain->getNameTranslated());
		QString uniqueName = QString::fromStdString(terrain->getJsonKey());

		ui->terrainFilterCombo->addItem(displayName, QVariant(uniqueName));
	}
	//add spacer to keep terrain button on the top
	ui->terrainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
	//adding roads
	for(auto & road : VLC->roadTypeHandler->objects)
	{
		auto *b = new QPushButton(QString::fromStdString(road->getNameTranslated()));
		ui->roadLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, roadID=road->getIndex()]{ roadOrRiverButtonClicked(roadID, true); });
	}
	//add spacer to keep terrain button on the top
	ui->roadLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
	//adding rivers
	for(auto & river : VLC->riverTypeHandler->objects)
	{
		auto *b = new QPushButton(QString::fromStdString(river->getNameTranslated()));
		ui->riverLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, riverID=river->getIndex()]{ roadOrRiverButtonClicked(riverID, false); });
	}
	//add spacer to keep terrain button on the top
	ui->riverLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	if(objectBrowser)
		throw std::runtime_error("object browser exists");

	//model
	objectsModel.setHorizontalHeaderLabels(QStringList() << tr("Type"));
	objectBrowser = new ObjectBrowserProxyModel(this);
	objectBrowser->setSourceModel(&objectsModel);
	objectBrowser->setDynamicSortFilter(false);
	objectBrowser->setRecursiveFilteringEnabled(true);
	ui->treeView->setModel(objectBrowser);
	ui->treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
	ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(treeViewSelected(const QModelIndex &, const QModelIndex &)));


	//adding objects
	addGroupIntoCatalog("TOWNS", false, false, Obj::TOWN);
	addGroupIntoCatalog("TOWNS", false, false, Obj::RANDOM_TOWN);
	addGroupIntoCatalog("TOWNS", true, false, Obj::SHIPYARD);
	addGroupIntoCatalog("TOWNS", true, false, Obj::GARRISON);
	addGroupIntoCatalog("TOWNS", true, false, Obj::GARRISON2);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::ARENA);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::BUOY);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::CARTOGRAPHER);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SWAN_POND);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::COVER_OF_DARKNESS);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::CORPSE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::FAERIE_RING);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::FOUNTAIN_OF_FORTUNE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::FOUNTAIN_OF_YOUTH);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::GARDEN_OF_REVELATION);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::HILL_FORT);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::IDOL_OF_FORTUNE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::LIBRARY_OF_ENLIGHTENMENT);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::LIGHTHOUSE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SCHOOL_OF_MAGIC);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MAGIC_SPRING);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MAGIC_WELL);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MERCENARY_CAMP);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MERMAID);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MYSTICAL_GARDEN);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::OASIS);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::LEAN_TO);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::OBELISK);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::REDWOOD_OBSERVATORY);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::PILLAR_OF_FIRE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::STAR_AXIS);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::RALLY_FLAG);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::WATERING_HOLE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SCHOLAR);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SHRINE_OF_MAGIC_INCANTATION);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SHRINE_OF_MAGIC_GESTURE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SHRINE_OF_MAGIC_THOUGHT);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SIRENS);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::STABLES);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::TAVERN);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::TEMPLE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::DEN_OF_THIEVES);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::LEARNING_STONE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::TREE_OF_KNOWLEDGE);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::WAGON);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SCHOOL_OF_WAR);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::WAR_MACHINE_FACTORY);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::WARRIORS_TOMB);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::WITCH_HUT);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::SANCTUARY);
	addGroupIntoCatalog("OBJECTS", true, false, Obj::MARLETTO_TOWER);
	addGroupIntoCatalog("HEROES", true, false, Obj::PRISON);
	addGroupIntoCatalog("HEROES", false, false, Obj::HERO);
	addGroupIntoCatalog("HEROES", false, false, Obj::RANDOM_HERO);
	addGroupIntoCatalog("HEROES", false, false, Obj::HERO_PLACEHOLDER);
	addGroupIntoCatalog("HEROES", false, false, Obj::BOAT);
	addGroupIntoCatalog("ARTIFACTS", true, false, Obj::ARTIFACT);
	addGroupIntoCatalog("ARTIFACTS", false, false, Obj::RANDOM_ART);
	addGroupIntoCatalog("ARTIFACTS", false, false, Obj::RANDOM_TREASURE_ART);
	addGroupIntoCatalog("ARTIFACTS", false, false, Obj::RANDOM_MINOR_ART);
	addGroupIntoCatalog("ARTIFACTS", false, false, Obj::RANDOM_MAJOR_ART);
	addGroupIntoCatalog("ARTIFACTS", false, false, Obj::RANDOM_RELIC_ART);
	addGroupIntoCatalog("ARTIFACTS", true, false, Obj::SPELL_SCROLL);
	addGroupIntoCatalog("ARTIFACTS", true, false, Obj::PANDORAS_BOX);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::RANDOM_RESOURCE);
	addGroupIntoCatalog("RESOURCES", false, false, Obj::RESOURCE);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::SEA_CHEST);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::TREASURE_CHEST);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::CAMPFIRE);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::SHIPWRECK_SURVIVOR);
	addGroupIntoCatalog("RESOURCES", true, false, Obj::FLOTSAM);
	addGroupIntoCatalog("BANKS", true, false, Obj::CREATURE_BANK);
	addGroupIntoCatalog("BANKS", true, false, Obj::DRAGON_UTOPIA);
	addGroupIntoCatalog("BANKS", true, false, Obj::CRYPT);
	addGroupIntoCatalog("BANKS", true, false, Obj::DERELICT_SHIP);
	addGroupIntoCatalog("BANKS", true, false, Obj::PYRAMID);
	addGroupIntoCatalog("BANKS", true, false, Obj::SHIPWRECK);
	addGroupIntoCatalog("DWELLINGS", true, false, Obj::CREATURE_GENERATOR1);
	addGroupIntoCatalog("DWELLINGS", true, false, Obj::CREATURE_GENERATOR2);
	addGroupIntoCatalog("DWELLINGS", true, false, Obj::CREATURE_GENERATOR3);
	addGroupIntoCatalog("DWELLINGS", true, false, Obj::CREATURE_GENERATOR4);
	addGroupIntoCatalog("DWELLINGS", true, false, Obj::REFUGEE_CAMP);
	addGroupIntoCatalog("DWELLINGS", false, false, Obj::RANDOM_DWELLING);
	addGroupIntoCatalog("DWELLINGS", false, false, Obj::RANDOM_DWELLING_LVL);
	addGroupIntoCatalog("DWELLINGS", false, false, Obj::RANDOM_DWELLING_FACTION);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::CURSED_GROUND1);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::MAGIC_PLAINS1);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::CLOVER_FIELD);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::CURSED_GROUND2);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::EVIL_FOG);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::FAVORABLE_WINDS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::FIERY_FIELDS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::HOLY_GROUNDS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::LUCID_POOLS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::MAGIC_CLOUDS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::MAGIC_PLAINS2);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::ROCKLANDS);
	addGroupIntoCatalog("GROUNDS", true, false, Obj::HOLE);
	addGroupIntoCatalog("TELEPORTS", true, false, Obj::MONOLITH_ONE_WAY_ENTRANCE);
	addGroupIntoCatalog("TELEPORTS", true, false, Obj::MONOLITH_ONE_WAY_EXIT);
	addGroupIntoCatalog("TELEPORTS", true, false, Obj::MONOLITH_TWO_WAY);
	addGroupIntoCatalog("TELEPORTS", true, false, Obj::SUBTERRANEAN_GATE);
	addGroupIntoCatalog("TELEPORTS", true, false, Obj::WHIRLPOOL);
	addGroupIntoCatalog("MINES", true, false, Obj::MINE);
	addGroupIntoCatalog("MINES", false, false, Obj::ABANDONED_MINE);
	addGroupIntoCatalog("MINES", true, false, Obj::WINDMILL);
	addGroupIntoCatalog("MINES", true, false, Obj::WATER_WHEEL);
	addGroupIntoCatalog("TRIGGERS", true, false, Obj::EVENT);
	addGroupIntoCatalog("TRIGGERS", true, false, Obj::GRAIL);
	addGroupIntoCatalog("TRIGGERS", true, false, Obj::SIGN);
	addGroupIntoCatalog("TRIGGERS", true, false, Obj::OCEAN_BOTTLE);
	addGroupIntoCatalog("MONSTERS", false, false, Obj::MONSTER);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L1);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L2);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L3);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L4);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L5);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L6);
	addGroupIntoCatalog("MONSTERS", true, false, Obj::RANDOM_MONSTER_L7);
	addGroupIntoCatalog("QUESTS", true, false, Obj::SEER_HUT);
	addGroupIntoCatalog("QUESTS", true, false, Obj::BORDER_GATE);
	addGroupIntoCatalog("QUESTS", true, false, Obj::QUEST_GUARD);
	addGroupIntoCatalog("QUESTS", true, false, Obj::HUT_OF_MAGI);
	addGroupIntoCatalog("QUESTS", true, false, Obj::EYE_OF_MAGI);
	addGroupIntoCatalog("QUESTS", true, false, Obj::BORDERGUARD);
	addGroupIntoCatalog("QUESTS", true, false, Obj::KEYMASTER);
	addGroupIntoCatalog("wog object", true, false, Obj::WOG_OBJECT);
	addGroupIntoCatalog("OBSTACLES", true);
	addGroupIntoCatalog("OTHER", false);
	}
	catch(const std::exception &)
	{
		QMessageBox::critical(this, "Mods loading problem", "Critical error during Mods loading. Disable invalid mods and restart.");
	}
}

void MainWindow::on_actionLevel_triggered()
{
	if(controller.map() && controller.map()->twoLevel)
	{
		mapLevel = mapLevel ? 0 : 1;
		ui->mapView->setScene(controller.scene(mapLevel));
		ui->minimapView->setScene(controller.miniScene(mapLevel));
		if (mapLevel == 0)
		{
			ui->actionLevel->setToolTip(tr("View underground"));
		}
		else
		{
			ui->actionLevel->setToolTip(tr("View surface"));
		}
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
	displayStatus(str);

	if (controller.map())
	{
		controller.redo();
	}
}

void MainWindow::on_actionPass_triggered(bool checked)
{
	QString str("Passability clicked");
	displayStatus(str);

	if(controller.map())
	{
		controller.scene(0)->passabilityView.show(checked);
		controller.scene(1)->passabilityView.show(checked);
	}
}


void MainWindow::on_actionGrid_triggered(bool checked)
{
	QString str("Grid clicked");
	displayStatus(str);

	if(controller.map())
	{
		controller.scene(0)->gridView.show(checked);
		controller.scene(1)->gridView.show(checked);
	}
}

void MainWindow::changeBrushState(int idx)
{

}

void MainWindow::on_actionErase_triggered()
{
	if(controller.map())
	{
		controller.commitObjectErase(mapLevel);
	}
}

void MainWindow::preparePreview(const QModelIndex &index)
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
		}
	}

	ui->objectPreview->fitInView(scenePreview->itemsBoundingRect(), Qt::KeepAspectRatio);
}


void MainWindow::treeViewSelected(const QModelIndex & index, const QModelIndex & deselected)
{
	ui->toolSelect->setChecked(true);
	ui->mapView->selectionTool = MapView::SelectionTool::None;
	
	preparePreview(index);
}

void MainWindow::on_terrainFilterCombo_currentIndexChanged(int index)
{
	if(!objectBrowser)
		return;

	QString uniqueName = ui->terrainFilterCombo->itemData(index).toString();

	objectBrowser->terrain = TerrainId(ETerrainId::ANY_TERRAIN);
	if (!uniqueName.isEmpty())
	{
		for (auto const & terrain : VLC->terrainTypeHandler->objects)
			if (terrain->getJsonKey() == uniqueName.toStdString())
				objectBrowser->terrain = terrain->getId();
	}
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
	QString str("Fill clicked");
	displayStatus(str);

	if(!controller.map())
		return;

	controller.commitObstacleFill(mapLevel);
}

void MainWindow::loadInspector(CGObjectInstance * obj, bool switchTab)
{
	if(switchTab)
		ui->tabWidget->setCurrentIndex(1);
	Inspector inspector(controller, obj, ui->inspectorWidget);
	inspector.updateProperties();
}

void MainWindow::on_inspectorWidget_itemChanged(QTableWidgetItem *item)
{
	if(!item->isSelected() && !(item->flags() & Qt::ItemIsUserCheckable))
		return;

	int r = item->row();
	int c = item->column();
	if(c < 1)
		return;

	auto * tableWidget = item->tableWidget();

	//get identifier
	auto identifier = tableWidget->item(0, 1)->text();
	CGObjectInstance * obj = data_cast<CGObjectInstance>(identifier.toLongLong());

	//get parameter name
	auto param = tableWidget->item(r, c - 1)->text();

	//set parameter
	Inspector inspector(controller, obj, tableWidget);
	inspector.setProperty(param, item);
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
	connect(settingsDialog, &QDialog::finished, this, &MainWindow::onPlayersChanged);
}

QAction * MainWindow::getActionPlayer(const PlayerColor & player)
{
	if(player.getNum() == 0) return ui->actionPlayer_1;
	if(player.getNum() == 1) return ui->actionPlayer_2;
	if(player.getNum() == 2) return ui->actionPlayer_3;
	if(player.getNum() == 3) return ui->actionPlayer_4;
	if(player.getNum() == 4) return ui->actionPlayer_5;
	if(player.getNum() == 5) return ui->actionPlayer_6;
	if(player.getNum() == 6) return ui->actionPlayer_7;
	if(player.getNum() == 7) return ui->actionPlayer_8;
	return ui->actionNeutral;
}

void MainWindow::switchDefaultPlayer(const PlayerColor & player)
{
	if(controller.defaultPlayer == player)
		return;
	
	ui->actionNeutral->blockSignals(true);
	ui->actionNeutral->setChecked(PlayerColor::NEUTRAL == player);
	ui->actionNeutral->blockSignals(false);
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT.getNum(); ++i)
	{
		getActionPlayer(PlayerColor(i))->blockSignals(true);
		getActionPlayer(PlayerColor(i))->setChecked(PlayerColor(i) == player);
		getActionPlayer(PlayerColor(i))->blockSignals(false);
	}
	controller.defaultPlayer = player;
}

void MainWindow::onPlayersChanged()
{
	if(controller.map())
	{
		getActionPlayer(PlayerColor::NEUTRAL)->setEnabled(true);
		for(int i = 0; i < controller.map()->players.size(); ++i)
			getActionPlayer(PlayerColor(i))->setEnabled(controller.map()->players.at(i).canAnyonePlay());
		if(!getActionPlayer(controller.defaultPlayer)->isEnabled() || controller.defaultPlayer == PlayerColor::NEUTRAL)
			switchDefaultPlayer(PlayerColor::NEUTRAL);
	}
	else
	{
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT.getNum(); ++i)
			getActionPlayer(PlayerColor(i))->setEnabled(false);
		getActionPlayer(PlayerColor::NEUTRAL)->setEnabled(false);
	}
	
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
		ui->actionErase->setEnabled(anythingSelected);
	}
}
void MainWindow::displayStatus(const QString& message, int timeout /* = 2000 */)
{
	statusBar()->showMessage(message, timeout);
}

void MainWindow::on_actionValidate_triggered()
{
	new Validator(controller.map(), this);
}


void MainWindow::on_actionUpdate_appearance_triggered()
{
	if(!controller.map())
		return;
	
	if(controller.scene(mapLevel)->selectionObjectsView.getSelection().empty())
	{
		QMessageBox::information(this, tr("Update appearance"), tr("No objects selected"));
		return;
	}
	
	if(QMessageBox::Yes != QMessageBox::question(this, tr("Update appearance"), tr("This operation is irreversible. Do you want to continue?")))
		return;
	
	controller.scene(mapLevel)->selectionTerrainView.clear();
	
	int errors = 0;
	std::set<CGObjectInstance*> staticObjects;
	for(auto * obj : controller.scene(mapLevel)->selectionObjectsView.getSelection())
	{
		auto handler = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
		if(!controller.map()->isInTheMap(obj->visitablePos()))
		{
			++errors;
			continue;
		}
		
		auto * terrain = controller.map()->getTile(obj->visitablePos()).terType;
		
		if(handler->isStaticObject())
		{
			staticObjects.insert(obj);
			if(obj->appearance->canBePlacedAt(terrain->getId()))
			{
				controller.scene(mapLevel)->selectionObjectsView.deselectObject(obj);
				continue;
			}
			
			for(auto & offset : obj->appearance->getBlockedOffsets())
				controller.scene(mapLevel)->selectionTerrainView.select(obj->pos + offset);
		}
		else
		{
			auto app = handler->getOverride(terrain->getId(), obj);
			if(!app)
			{
				if(obj->appearance->canBePlacedAt(terrain->getId()))
					continue;
				
				auto templates = handler->getTemplates(terrain->getId());
				if(templates.empty())
				{
					++errors;
					continue;
				}
				app = templates.front();
			}
			obj->appearance = app;
			controller.mapHandler()->invalidate(obj);
			controller.scene(mapLevel)->selectionObjectsView.deselectObject(obj);
		}
	}
	controller.commitObjectChange(mapLevel);
	controller.commitObjectErase(mapLevel);
	controller.commitObstacleFill(mapLevel);
	
	
	if(errors)
		QMessageBox::warning(this, tr("Update appearance"), QString(tr("Errors occurred. %1 objects were not updated")).arg(errors));
}


void MainWindow::on_actionRecreate_obstacles_triggered()
{

}


void MainWindow::on_actionCut_triggered()
{
	if(controller.map())
	{
		controller.copyToClipboard(mapLevel);
		controller.commitObjectErase(mapLevel);
	}
}


void MainWindow::on_actionCopy_triggered()
{
	if(controller.map())
	{
		controller.copyToClipboard(mapLevel);
	}
}


void MainWindow::on_actionPaste_triggered()
{
	if(controller.map())
	{
		controller.pasteFromClipboard(mapLevel);
	}
}


void MainWindow::on_actionExport_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save to image"), lastSavingDir, "BMP (*.bmp);;JPEG (*.jpeg);;PNG (*.png)");
	if(!fileName.isNull())
	{
		QImage image(ui->mapView->scene()->sceneRect().size().toSize(), QImage::Format_RGB888);
		QPainter painter(&image);
		ui->mapView->scene()->render(&painter);
		image.save(fileName);
	}
}


void MainWindow::on_actionTranslations_triggered()
{
	auto translationsDialog = new Translations(*controller.map(), this);
	translationsDialog->show();
}

void MainWindow::on_actionh3m_converter_triggered()
{
	auto mapFiles = QFileDialog::getOpenFileNames(this, tr("Select maps to convert"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()),
		tr("HoMM3 maps(*.h3m)"));
	if(mapFiles.empty())
		return;
	
	auto saveDirectory = QFileDialog::getExistingDirectory(this, tr("Choose directory to save converted maps"), QCoreApplication::applicationDirPath());
	if(saveDirectory.isEmpty())
		return;
	
	try
	{
		for(auto & m : mapFiles)
		{
			CMapService mapService;
			auto map = openMapInternal(m);
			controller.repairMap(map.get());
			mapService.saveMap(map, (saveDirectory + '/' + QFileInfo(m).completeBaseName() + ".vmap").toStdString());
		}
		QMessageBox::information(this, tr("Operation completed"), tr("Successfully converted %1 maps").arg(mapFiles.size()));
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, tr("Failed to convert the map. Abort operation"), tr(e.what()));
	}
}


void MainWindow::on_actionLock_triggered()
{
	if(controller.map())
	{
		if(controller.scene(mapLevel)->selectionObjectsView.getSelection().empty())
		{
			for(auto obj : controller.map()->objects)
			{
				controller.scene(mapLevel)->selectionObjectsView.setLockObject(obj, true);
				controller.scene(mapLevel)->objectsView.setLockObject(obj, true);
			}
		}
		else
		{
			for(auto * obj : controller.scene(mapLevel)->selectionObjectsView.getSelection())
			{
				controller.scene(mapLevel)->selectionObjectsView.setLockObject(obj, true);
				controller.scene(mapLevel)->objectsView.setLockObject(obj, true);
			}
			controller.scene(mapLevel)->selectionObjectsView.clear();
		}
		controller.scene(mapLevel)->objectsView.update();
		controller.scene(mapLevel)->selectionObjectsView.update();
	}
}


void MainWindow::on_actionUnlock_triggered()
{
	if(controller.map())
	{
		controller.scene(mapLevel)->selectionObjectsView.unlockAll();
		controller.scene(mapLevel)->objectsView.unlockAll();
	}
	controller.scene(mapLevel)->objectsView.update();
}


void MainWindow::on_actionZoom_in_triggered()
{
	auto rect = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	rect -= QMargins{32 + 1, 32 + 1, 32 + 2, 32 + 2}; //compensate bounding box
	ui->mapView->fitInView(rect, Qt::KeepAspectRatioByExpanding);
}


void MainWindow::on_actionZoom_out_triggered()
{
	auto rect = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	rect += QMargins{32 - 1, 32 - 1, 32 - 2, 32 - 2}; //compensate bounding box
	ui->mapView->fitInView(rect, Qt::KeepAspectRatioByExpanding);
}


void MainWindow::on_actionZoom_reset_triggered()
{
	auto center = ui->mapView->mapToScene(ui->mapView->viewport()->geometry().center());
	ui->mapView->fitInView(initialScale, Qt::KeepAspectRatioByExpanding);
	ui->mapView->centerOn(center);
}


void MainWindow::on_toolLine_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Line;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolBrush2_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush2;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolBrush_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolBrush4_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush4;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolLasso_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Lasso;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolArea_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Area;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolFill_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Fill;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void MainWindow::on_toolSelect_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::None;
		ui->tabWidget->setCurrentIndex(0);
	}
}

