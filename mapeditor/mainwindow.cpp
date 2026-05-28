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
#include <QTranslator>
#include <QFileInfo>
#include <QDialog>
#include <QListWidget>
#include <QTimer>

#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CConsoleHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/GameConstants.h"
#include "../lib/campaign/CampaignHandler.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/mapping/MapFormatJson.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/RoadHandler.h"
#include "../lib/RiverHandler.h"
#include "../lib/TerrainHandler.h"
#include "../lib/MapLayerHandler.h"

#include "../vcmiqt/launcherdirs.h"

#include "maphandler.h"
#include "graphics.h"
#include "windownewmap.h"
#include "objectbrowser.h"
#include "inspector/inspector.h"
#include "mapsettings/mapsettings.h"
#include "mapsettings/translations.h"
#include "mapsettings/modsettings.h"
#include "PlayerSettingsDialog.h"
#include "validator.h"
#include "helper.h"
#include "campaigneditor/campaigneditor.h"
#ifdef ENABLE_TEMPLATE_EDITOR
#include "templateeditor/templateeditor.h"
#endif
#include "editorfiledialog.h"

#ifdef VCMI_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#include <QWindow>
#endif

#ifdef VCMI_ANDROID
// Qt 5 bug on Android with QT_SCALE_FACTOR:
// In androidjniinput.cpp, mouse/tablet handlers compute:
//   localPos = globalPos(device_pixels) - tlw->position()(logical_pixels)
// mixing coordinate spaces. After QHighDpiScaling the widget receives
// a localPos that is off by windowPos*(1-1/S) for any window not at (0,0).
// Touch events use normalized coordinates and are NOT affected.
//
// Fix: recompute localPos from the already-correct global/screen position.
// After QHighDpi scaling, screenPos() (mouse) and globalPosF() (tablet)
// are in logical coordinates.  window->position() is also logical.
// So: correctedLocal = globalLogical - window->position().
// We modify the event in-place (protected members l,w / mPos) to preserve
// the spontaneous flag and Qt's internal mouse-grab state.
// The fix is always active (harmless no-op when scale == 1.0).
class AndroidInputOffsetFix : public QObject
{
public:
	AndroidInputOffsetFix(QObject * parent) : QObject(parent) {}

	bool eventFilter(QObject * obj, QEvent * event) override
	{
		auto * window = qobject_cast<QWindow *>(obj);
		if(!window)
			return false;

		switch(event->type())
		{
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseMove:
			case QEvent::MouseButtonDblClick:
			{
				auto * me = static_cast<QMouseEvent *>(event);
				// screenPos() (= global logical pos) is always correct after
				// QHighDpi scaling.  Recompute the local position from it.
				QPointF correct = me->screenPos() - QPointF(window->position());
				struct Accessor : QMouseEvent
				{
					void set(const QPointF & p) { l = p; w = p; }
				};
				static_cast<Accessor *>(me)->set(correct);
				return false;
			}
			case QEvent::TabletPress:
			case QEvent::TabletRelease:
			case QEvent::TabletMove:
			{
				auto * te = static_cast<QTabletEvent *>(event);
				// globalPosF() is already in correct logical coordinates after
				// QHighDpi::fromNativePixels in handleTabletEvent().  Only the
				// local position (mPos) was computed incorrectly by the JNI
				// layer (native_px - logical_px mismatch).  Recompute it.
				QPointF correctedLocal = te->globalPosF() - QPointF(window->position());
				struct Accessor : QTabletEvent
				{
					void set(const QPointF & lp) { mPos = lp; }
				};
				static_cast<Accessor *>(te)->set(correctedLocal);
				return false;
			}
			default:
				return false;
		}
	}
};

static void androidFinishActivity()
{
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if(activity.isValid())
	{
		activity.callMethod<void>("finishAffinity");
	}
}
#endif

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

void EditorMainWindow::loadUserSettings()
{
	//load window settings
	QSettings s = CLauncherDirs::getSettings(Ui::appName);

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

void EditorMainWindow::saveUserSettings()
{
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	s.setValue(mainWindowSizeSetting, size());
	s.setValue(mainWindowPositionSetting, pos());
	s.setValue(lastDirectorySetting, lastSavingDir);
}

void EditorMainWindow::parseCommandLine(ExtractionOptions & extractionOptions)
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

void EditorMainWindow::loadTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	const std::string language = settings["general"]["language"].String();
	const std::string translationFile = language + ".qm";
	QString translationFileResourcePath = QString{":/editor/translation/%1"}.arg(translationFile.c_str());

	logGlobal->info("Loading editor translation: language='%s', file='%s', resource='%s'",
		language, translationFile, translationFileResourcePath.toStdString());

	qApp->removeTranslator(&translator);

	if(!QFile::exists(translationFileResourcePath))
	{
		logGlobal->warn("Translation file %s does not exist!", translationFileResourcePath.toStdString());
		return;
	}

	if(translationFile == "english.qm")
	{
		// No translator needed for English
		return;
	}

	if(!translator.load(translationFileResourcePath))
	{
		logGlobal->error("Failed to load translation file %s", translationFileResourcePath.toStdString());
		return;
	}

	if(!qApp->installTranslator(&translator))
	{
		logGlobal->error("Failed to install translator for translation file %s", translationFileResourcePath.toStdString());
	}
#endif
}

void EditorMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void EditorMainWindow::dropEvent(QDropEvent* event)
{
	if (!getAnswerAboutUnsavedChanges())
		return;

	for (const QUrl& url : event->mimeData()->urls())
	{
		QString path = url.toLocalFile();
		if (path.endsWith(".h3m", Qt::CaseInsensitive) || path.endsWith(".vmap", Qt::CaseInsensitive))
		{
			openMap(path);
			break;
		}
		else if (path.endsWith(".h3c", Qt::CaseInsensitive) || path.endsWith(".vcmp", Qt::CaseInsensitive))
		{
			hide();
			openCampaign(path);
			break;
		}
	}
}

EditorMainWindow::EditorMainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::EditorMainWindow),
	controller(this)
{
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

	setAcceptDrops(true);
	
	new QShortcut(QKeySequence("Backspace"), this, SLOT(on_actionErase_triggered()));

	ExtractionOptions extractionOptions;
	parseCommandLine(extractionOptions);

	//configure logging
	const boost::filesystem::path logPath = VCMIDirs::get().userLogsPath() / "VCMI_Editor_log.txt";
#ifndef VCMI_MOBILE
	console = std::make_unique<CConsoleHandler>();
	logConfig = std::make_unique<CBasicLogConfigurator>(logPath, console.get());
#else
	logConfig = std::make_unique<CBasicLogConfigurator>(logPath, nullptr);
#endif
	logConfig->configureDefault();
	logGlobal->info("Starting map editor of '%s'", GameConstants::VCMI_VERSION);
	logGlobal->info("The log file will be saved to %s", logPath);

	//init
	LIBRARY = new GameLibrary();
	LIBRARY->initializeFilesystem(extractionOptions.extractArchives);

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

#ifdef VCMI_MOBILE
	// On Android the native menu bar is hidden; force it into the window content area
	menuBar()->setNativeMenuBar(false);
#endif

#ifdef VCMI_ANDROID
	qApp->installEventFilter(new AndroidInputOffsetFix(this));
#endif

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
	ui->actionOpenRecent->setIcon(QIcon{":/icons/document-open-recent.png"});
	ui->menuOpenRecent->setIcon(QIcon{":/icons/document-open-recent.png"});
	ui->actionSave->setIcon(QIcon{":/icons/document-save.png"});
	ui->actionNew->setIcon(QIcon{":/icons/document-new.png"});
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
	ui->actionCampaignEditor->setIcon(QIcon{":/icons/mapeditor.64x64.png"});
	ui->actionTemplateEditor->setIcon(QIcon{":/icons/dice.png"});

	// Add combobox action
	for (QWidget* c : QList<QWidget*>{ ui->toolBar, ui->menuView })
	{
		QWidget* container = new QWidget;
		QHBoxLayout* layout = new QHBoxLayout(container);
		layout->setContentsMargins(6, 2, 4, 2);
		layout->setSpacing(2);

		if (c == ui->menuView)
		{
			// Add icon label only for QMenu
			QLabel* iconLabel = new QLabel;
			iconLabel->setPixmap(QIcon(":/icons/toggle-underground.png").pixmap(16, 16));
			iconLabel->setContentsMargins(0, 2, 0, 0);
			layout->addWidget(iconLabel);
		}

		// Add the combo box
		QComboBox* combo = new QComboBox;
		combo->setFixedHeight(ui->menuView->fontMetrics().height() + 6);
		combo->setMinimumWidth(160);
		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int index) {
			for(auto & box : levelComboBoxes)
				if (box->currentIndex() != index && combo != box)
        			box->setCurrentIndex(index);
			
			if(!controller.map())
				return;
			
			mapLevel = combo->currentIndex();
			ui->mapView->setScene(controller.scene(mapLevel));
			ui->mapView->resetInteractionState();
			ui->mapView->setViewports();
			ui->minimapView->setScene(controller.miniScene(mapLevel));
		});
		layout->addWidget(combo, c == ui->menuView ? 1 : 0);

		// Create the widget action
		QWidgetAction* comboAction = new QWidgetAction(this);
		comboAction->setDefaultWidget(container);

		int index = c->actions().indexOf(ui->actionLevel);
		if (index != -1)
		{
			c->removeAction(ui->actionLevel);
			c->insertAction(c->actions().value(index), comboAction);
		}

		levelComboBoxes.push_back(combo);
	}

#ifndef ENABLE_TEMPLATE_EDITOR
	ui->actionTemplateEditor->setVisible(false);
#endif

	loadUserSettings(); //For example window size
	setTitle();

	LIBRARY->initializeLibrary();

	Settings config = settings.write["session"]["editor"];
	config->Bool() = true;

	logGlobal->info("Initializing VCMI_Lib");

	graphics = new Graphics(); // should be before curh->init()
	graphics->load();//must be after Content loading but should be in main thread

	if (extractionOptions.extractArchives)
		ResourceConverter::convertExtractedResourceFiles(extractionOptions.conversionOptions);
	
	ui->mapView->setScene(controller.scene(0));
	ui->mapView->setController(&controller);
	ui->mapView->resetInteractionState();
	ui->mapView->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);
	connect(ui->mapView, &MapView::openObjectProperties, this, &EditorMainWindow::loadInspector);
	connect(ui->mapView, &MapView::currentCoordinates, this, &EditorMainWindow::currentCoordinatesChanged);
	
	ui->minimapView->setScene(controller.miniScene(0));
	ui->minimapView->setController(&controller);
	connect(ui->minimapView, &MinimapView::cameraPositionChanged, ui->mapView, &MapView::cameraChanged);

	scenePreview = new QGraphicsScene(this);
	ui->objectPreview->setScene(scenePreview);

	connect(ui->actionOpenRecentMore, &QAction::triggered, this, &EditorMainWindow::on_actionOpenRecent_triggered);

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

#ifdef VCMI_ANDROID
	QTimer::singleShot(0, this, [this]() {
		QMessageBox::information(this,
			tr("Mapeditor"),
			tr("Mapeditor on Android is experimental.\n\nFor the best experience, we recommend using the map editor on a tablet (or with a mouse/pen)."));
	});
#endif

	//Load map from command line
	if(!mapFilePath.isEmpty())
		openMap(mapFilePath);
}

EditorMainWindow::~EditorMainWindow()
{
	saveUserSettings(); //save window size etc.
	delete ui;
}

bool EditorMainWindow::getAnswerAboutUnsavedChanges()
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

void EditorMainWindow::closeEvent(QCloseEvent *event)
{
	if(getAnswerAboutUnsavedChanges())
	{
		QMainWindow::closeEvent(event);
#ifdef VCMI_ANDROID
		// On Android the Qt event loop may not quit automatically when the last
		// window closes. Quit the app and finish the Activity to avoid a black screen.
		QApplication::quit();
		androidFinishActivity();
#endif
	}
	else
		event->ignore();
}

void EditorMainWindow::setStatusMessage(const QString & status)
{
	statusBar()->showMessage(status);
}

void EditorMainWindow::setTitle()
{
	QString title = QString("%1%2 - %3 (%4)").arg(filename, unsaved ? "*" : "", tr("VCMI Map Editor"), GameConstants::VCMI_VERSION.c_str());
	setWindowTitle(title);
}

void EditorMainWindow::mapChanged()
{
	unsaved = true;
	setTitle();
}

void EditorMainWindow::initializeMap(bool isNew)
{
	unsaved = isNew;
	if(isNew)
		filename.clear();
	setTitle();

	mapLevel = 0;
	ui->mapView->setScene(controller.scene(mapLevel));
	ui->mapView->resetInteractionState();
	ui->minimapView->setScene(controller.miniScene(mapLevel));
	ui->minimapView->dimensions();
	if(initialScale.isValid())
		on_actionZoom_reset_triggered();
	initialScale = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	
	//enable settings
	mapSettings = new MapSettings(controller, this);
	connect(&controller, &MapController::requestModsUpdate,
		mapSettings->getModSettings(), &ModSettings::updateModWidgetBasedOnMods);

	ui->actionMapSettings->setEnabled(true);
	ui->actionPlayers_settings->setEnabled(true);
	ui->actionTranslations->setEnabled(true);
	for(auto & box : levelComboBoxes)
	{
		box->clear();
		for(int i = 0; i < controller.map()->levels(); i++)
		{
			box->addItems({ tr("Level %1: %2").arg(i + 1).arg(QString::fromStdString(controller.map()->mapLayers.at(i).toEntity(LIBRARY)->getNameTranslated())) });
		}
	}
	ui->actionLevel->setEnabled(true);
	ui->actionMapLayer->setEnabled(true);
	
	//set minimal players count
	if(isNew)
	{
		controller.map()->players[0].canComputerPlay = true;
		controller.map()->players[0].canHumanPlay = true;
	}
	ui->inspectorWidget->setRowCount(0);
	onPlayersChanged();
}

bool EditorMainWindow::openMap(const QString & filenameSelect)
{
	try
	{
		controller.setMap(Helper::openMapInternal(filenameSelect, controller.getCallback()));
	}
	catch(const ModIncompatibility & e)
	{
		assert(e.whatExcessive().empty());
		auto qstrError = QString::fromStdString(e.getFullErrorMsg()).remove('{').remove('}');
		QMessageBox::warning(this, tr("Mods are required"), qstrError);
		return false;
	}
	catch(const IdentifierResolutionException & e)
	{
		MetaString errorMsg;
		errorMsg.appendTextID("vcmi.server.errors.campOrMapFile.unknownEntity");
		errorMsg.replaceRawString(e.identifierName);
		QMessageBox::critical(this, tr("Failed to open map"), QString::fromStdString(errorMsg.toString()));
		return false;
	}

	catch(const std::exception & e)
	{
		QMessageBox::critical(this, tr("Failed to open map"), tr(e.what()));
		return false;
	}
	
	filename = filenameSelect;
	initializeMap(controller.map()->version != EMapFormat::VCMI);

	updateRecentMenu(filenameSelect);

	return true;
}

void EditorMainWindow::openCampaign(const QString & filenameSelect)
{
	CampaignEditor::showCampaignEditor(this, filenameSelect, controller.getCallback());
}

void EditorMainWindow::updateRecentMenu(const QString & filenameSelect) {
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	QStringList recentFiles = s.value(recentlyOpenedFilesSetting).toStringList();
	recentFiles.removeAll(filenameSelect);
	recentFiles.prepend(filenameSelect);
	constexpr int maxRecentFiles = 10;
	s.setValue(recentlyOpenedFilesSetting, QStringList(recentFiles.mid(0, maxRecentFiles)));
}

void EditorMainWindow::on_actionOpen_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;
	
	auto title = tr("Open map");
	auto dir = QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string());
	auto filter = tr("All supported maps (*.vmap *.h3m);;VCMI maps(*.vmap);;HoMM3 maps(*.h3m)");

	auto filenameSelect = EditorFileDialog::getOpenFileName(this, title, dir, filter);
	if(filenameSelect.isEmpty())
		return;
	
	openMap(filenameSelect);
}

void EditorMainWindow::on_actionOpenRecent_triggered()
{
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	QStringList recentFiles = s.value(recentlyOpenedFilesSetting).toStringList();

	class RecentFileDialog : public QDialog
	{

	public:
		RecentFileDialog(const QStringList& recentFiles, QWidget *parent)
			: QDialog(parent), layout(new QVBoxLayout(this)), listWidget(new QListWidget(this))
		{
			setMinimumWidth(600);
			connect(listWidget, &QListWidget::itemActivated, this, [this](QListWidgetItem *item)
			{
				accept();
			});

			for (const QString &file : recentFiles)
			{
				QListWidgetItem *item = new QListWidgetItem(file);
				listWidget->addItem(item);
			}

			// Select most recent items by default.
			// This enables a "CTRL+R => Enter"-workflow instead of "CTRL+R => 'mouse click on first item'"
			if(listWidget->count() > 0)
			{
				listWidget->item(0)->setSelected(true);
			}

			layout->setSizeConstraint(QLayout::SetMaximumSize);
			layout->addWidget(listWidget);
		}

		QString getSelectedFilePath() const
		{
			return listWidget->currentItem()->text();
		}

	private:
		QVBoxLayout * layout;
		QListWidget * listWidget;
	};

	RecentFileDialog d(recentFiles, this);
	d.setWindowTitle(tr("Recently Opened Files"));
	if(d.exec() == QDialog::Accepted && getAnswerAboutUnsavedChanges())
	{
		openMap(d.getSelectedFilePath());
	}
}

void EditorMainWindow::on_menuOpenRecent_aboutToShow()
{
	// Clear all actions except "More...", lest the list will grow with each
	// showing of the list
	for (QAction* action : ui->menuOpenRecent->actions()) {
		if (action != ui->actionOpenRecentMore) {
			ui->menuOpenRecent->removeAction(action);
		}
	}

	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	QStringList recentFiles = s.value(recentlyOpenedFilesSetting).toStringList();

	// Dynamically populate menuOpenRecent with one action per file.
	for (const QString & file : recentFiles) {
		QAction *action = new QAction(file, this);
		ui->menuOpenRecent->insertAction(ui->actionOpenRecentMore, action);
		connect(action, &QAction::triggered, this, [this, file]() {
			if(!getAnswerAboutUnsavedChanges())
				return;
			openMap(file);
		});
	}

	// Finally add a separator between recent entries and "More..."
	if(recentFiles.size() > 0) {
		ui->menuOpenRecent->insertSeparator(ui->actionOpenRecentMore);
	}
}

void EditorMainWindow::saveMap(bool force)
{
	if(!controller.map())
		return;

	if(!force && !unsaved)
		return;
	
	//validate map
	auto issues = Validator::validate(controller.map());
	bool critical = false;
	for(auto & issue : issues)
		critical |= issue.critical;
	
	if(!issues.empty())
	{
		auto mapValidationTitle = tr("Map validation");
		if(critical)
			QMessageBox::warning(this, mapValidationTitle, tr("Map has critical problems and most probably will not be playable. Open Validator from the Map menu to see issues found"));
		else
			QMessageBox::information(this, mapValidationTitle, tr("Map has some errors. Open Validator from the Map menu to see issues found"));
	}
	
	Translations::cleanupRemovedItems(*controller.map());

	for(auto obj : controller.map()->objects)
	{
		if(!obj)
			continue;

		if(obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto hero = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(hero->heroType.has_value())
			{
				controller.map()->reservedCampaignHeroes.insert(hero->heroType.value());
			}
		}
	}

	CMapService mapService;
	try
	{
		mapService.saveMap(controller.getMapUniquePtr(), filename.toStdString());
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, tr("Failed to save map"), e.what());
		return;
	}
	
	unsaved = false;
	setTitle();
}

void EditorMainWindow::on_actionSave_as_triggered()
{
	if(!controller.map())
		return;

	auto title = tr("Save map");
	auto filter = tr("VCMI maps (*.vmap)");

	QString contentUri;
	auto filenameSelect = EditorFileDialog::getSaveFileName(this, title, lastSavingDir, filter, contentUri);

	if(filenameSelect.isNull())
		return;

	QFileInfo fileInfo(filenameSelect);
	lastSavingDir = fileInfo.dir().path();

	if(fileInfo.suffix().toLower() != "vmap")
		filenameSelect += ".vmap";

	filename = filenameSelect;

	saveMap(true);

	EditorFileDialog::writeFileToUri(filename, contentUri);
}

void EditorMainWindow::on_actionCampaignEditor_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;

	hide();
	CampaignEditor::showCampaignEditor(this, controller.getCallback());
}

void EditorMainWindow::on_actionTemplateEditor_triggered()
{
#ifdef ENABLE_TEMPLATE_EDITOR
	if(!getAnswerAboutUnsavedChanges())
		return;

	hide();
	TemplateEditor::showTemplateEditor(this);
#endif
}

void EditorMainWindow::on_actionNew_triggered()
{
	if(getAnswerAboutUnsavedChanges())
		new WindowNewMap(this);
}

void EditorMainWindow::on_actionSave_triggered()
{
	if(!controller.map())
		return;

	if(filename.isNull())
		on_actionSave_as_triggered();
	else
		saveMap();
}

void EditorMainWindow::currentCoordinatesChanged(int x, int y)
{
	setStatusMessage(QString("x: %1   y: %2").arg(x).arg(y));
}

void EditorMainWindow::terrainButtonClicked(TerrainId terrain)
{
	controller.commitTerrainChange(mapLevel, terrain);
}

void EditorMainWindow::roadOrRiverButtonClicked(ui8 type, bool isRoad)
{
	controller.commitRoadOrRiverChange(mapLevel, type, isRoad);
}

void EditorMainWindow::addGroupIntoCatalog(const QString & groupName, bool staticOnly)
{
	auto knownObjects = LIBRARY->objtypeh->knownObjects();
	for(auto ID : knownObjects)
	{
		if(catalog.count(ID))
			continue;

		addGroupIntoCatalog(groupName, true, staticOnly, ID);
	}
}

void EditorMainWindow::addGroupIntoCatalog(const QString & groupName, bool useCustomName, bool staticOnly, int ID)
{
	QStandardItem * itemGroup = nullptr;
	auto itms = objectsModel.findItems(groupName);
	if(itms.empty())
	{
		itemGroup = new QStandardItem(groupName);
		objectsModel.appendRow(itemGroup);
	}
	else
	{
		itemGroup = itms.front();
	}

	if (LIBRARY->objtypeh->knownObjects().count(ID) == 0)
		return;

	auto knownSubObjects = LIBRARY->objtypeh->knownSubObjects(ID);
	for(auto secondaryID : knownSubObjects)
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(ID, secondaryID);
		auto templates = factory->getTemplates();
		bool singleTemplate = templates.size() == 1;
		if(staticOnly && !factory->isStaticObject())
			continue;

		auto subGroupName = QString::fromStdString(LIBRARY->objtypeh->getObjectName(ID, secondaryID));
		
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
			auto temporaryObj(factory->create(controller.getCallback(), templ));
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

void EditorMainWindow::loadObjectsTree()
{
	try
	{
	ui->terrainFilterCombo->addItem("");
	//adding terrains
	for(auto & terrain : LIBRARY->terrainTypeHandler->objects)
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
	for(auto & road : LIBRARY->roadTypeHandler->objects)
	{
		auto *b = new QPushButton(QString::fromStdString(road->getNameTranslated()));
		ui->roadLayout->addWidget(b);
		connect(b, &QPushButton::clicked, this, [this, roadID=road->getIndex()]{ roadOrRiverButtonClicked(roadID, true); });
	}
	//add spacer to keep terrain button on the top
	ui->roadLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
	//adding rivers
	for(auto & river : LIBRARY->riverTypeHandler->objects)
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

	//groups
	enum GroupCat { TOWNS, OBJECTS, HEROES, ARTIFACTS, RESOURCES, BANKS, DWELLINGS, GROUNDS, TELEPORTS, MINES, TRIGGERS, MONSTERS, QUESTS, WOG_OBJECTS, OBSTACLES, OTHER };
	QMap<GroupCat, QString> groups = {
		{ TOWNS,       tr("Towns")       },
		{ OBJECTS,     tr("Objects")     },
		{ HEROES,      tr("Heroes")      },
		{ ARTIFACTS,   tr("Artifacts")   },
		{ RESOURCES,   tr("Resources")   },
		{ BANKS,       tr("Banks")       },
		{ DWELLINGS,   tr("Dwellings")   },
		{ GROUNDS,     tr("Grounds")     },
		{ TELEPORTS,   tr("Teleports")   },
		{ MINES,       tr("Mines")       },
		{ TRIGGERS,    tr("Triggers")    },
		{ MONSTERS,    tr("Monsters")    },
		{ QUESTS,      tr("Quests")      },
		{ WOG_OBJECTS, tr("Wog Objects") },
		{ OBSTACLES,   tr("Obstacles")   },
		{ OTHER,       tr("Other")       },
	};

	//adding objects
	addGroupIntoCatalog(groups[TOWNS], false, false, Obj::TOWN);
	addGroupIntoCatalog(groups[TOWNS], false, false, Obj::RANDOM_TOWN);
	addGroupIntoCatalog(groups[TOWNS], true, false, Obj::SHIPYARD);
	addGroupIntoCatalog(groups[TOWNS], true, false, Obj::GARRISON);
	addGroupIntoCatalog(groups[TOWNS], true, false, Obj::GARRISON2);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::ARENA);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::BUOY);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::CARTOGRAPHER);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SWAN_POND);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::COVER_OF_DARKNESS);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::CORPSE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::FAERIE_RING);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::FOUNTAIN_OF_FORTUNE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::FOUNTAIN_OF_YOUTH);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::GARDEN_OF_REVELATION);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::HILL_FORT);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::IDOL_OF_FORTUNE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::LIBRARY_OF_ENLIGHTENMENT);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::LIGHTHOUSE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SCHOOL_OF_MAGIC);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MAGIC_SPRING);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MAGIC_WELL);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MERCENARY_CAMP);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MERMAID);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MYSTICAL_GARDEN);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::OASIS);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::LEAN_TO);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::OBELISK);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::REDWOOD_OBSERVATORY);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::PILLAR_OF_FIRE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::STAR_AXIS);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::RALLY_FLAG);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::WATERING_HOLE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SCHOLAR);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SHRINE_OF_MAGIC_INCANTATION);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SHRINE_OF_MAGIC_GESTURE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SHRINE_OF_MAGIC_THOUGHT);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SIRENS);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::STABLES);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::TAVERN);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::TEMPLE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::DEN_OF_THIEVES);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::LEARNING_STONE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::TREE_OF_KNOWLEDGE);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::WAGON);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SCHOOL_OF_WAR);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::WAR_MACHINE_FACTORY);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::WARRIORS_TOMB);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::WITCH_HUT);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::SANCTUARY);
	addGroupIntoCatalog(groups[OBJECTS], true, false, Obj::MARLETTO_TOWER);
	addGroupIntoCatalog(groups[HEROES], true, false, Obj::PRISON);
	addGroupIntoCatalog(groups[HEROES], false, false, Obj::HERO);
	addGroupIntoCatalog(groups[HEROES], false, false, Obj::RANDOM_HERO);
	addGroupIntoCatalog(groups[HEROES], false, false, Obj::HERO_PLACEHOLDER);
	addGroupIntoCatalog(groups[HEROES], false, false, Obj::BOAT);
	addGroupIntoCatalog(groups[ARTIFACTS], true, false, Obj::ARTIFACT);
	addGroupIntoCatalog(groups[ARTIFACTS], false, false, Obj::RANDOM_ART);
	addGroupIntoCatalog(groups[ARTIFACTS], false, false, Obj::RANDOM_TREASURE_ART);
	addGroupIntoCatalog(groups[ARTIFACTS], false, false, Obj::RANDOM_MINOR_ART);
	addGroupIntoCatalog(groups[ARTIFACTS], false, false, Obj::RANDOM_MAJOR_ART);
	addGroupIntoCatalog(groups[ARTIFACTS], false, false, Obj::RANDOM_RELIC_ART);
	addGroupIntoCatalog(groups[ARTIFACTS], true, false, Obj::SPELL_SCROLL);
	addGroupIntoCatalog(groups[ARTIFACTS], true, false, Obj::PANDORAS_BOX);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::RANDOM_RESOURCE);
	addGroupIntoCatalog(groups[RESOURCES], false, false, Obj::RESOURCE);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::SEA_CHEST);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::TREASURE_CHEST);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::CAMPFIRE);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::SHIPWRECK_SURVIVOR);
	addGroupIntoCatalog(groups[RESOURCES], true, false, Obj::FLOTSAM);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::CREATURE_BANK);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::DRAGON_UTOPIA);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::CRYPT);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::DERELICT_SHIP);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::PYRAMID);
	addGroupIntoCatalog(groups[BANKS], true, false, Obj::SHIPWRECK);
	addGroupIntoCatalog(groups[DWELLINGS], true, false, Obj::CREATURE_GENERATOR1);
	addGroupIntoCatalog(groups[DWELLINGS], true, false, Obj::CREATURE_GENERATOR2);
	addGroupIntoCatalog(groups[DWELLINGS], true, false, Obj::CREATURE_GENERATOR3);
	addGroupIntoCatalog(groups[DWELLINGS], true, false, Obj::CREATURE_GENERATOR4);
	addGroupIntoCatalog(groups[DWELLINGS], true, false, Obj::REFUGEE_CAMP);
	addGroupIntoCatalog(groups[DWELLINGS], false, false, Obj::RANDOM_DWELLING);
	addGroupIntoCatalog(groups[DWELLINGS], false, false, Obj::RANDOM_DWELLING_LVL);
	addGroupIntoCatalog(groups[DWELLINGS], false, false, Obj::RANDOM_DWELLING_FACTION);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::CURSED_GROUND1);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::MAGIC_PLAINS1);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::CLOVER_FIELD);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::CURSED_GROUND2);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::EVIL_FOG);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::FAVORABLE_WINDS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::FIERY_FIELDS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::HOLY_GROUNDS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::LUCID_POOLS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::MAGIC_CLOUDS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::MAGIC_PLAINS2);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::ROCKLANDS);
	addGroupIntoCatalog(groups[GROUNDS], true, false, Obj::HOLE);
	addGroupIntoCatalog(groups[TELEPORTS], true, false, Obj::MONOLITH_ONE_WAY_ENTRANCE);
	addGroupIntoCatalog(groups[TELEPORTS], true, false, Obj::MONOLITH_ONE_WAY_EXIT);
	addGroupIntoCatalog(groups[TELEPORTS], true, false, Obj::MONOLITH_TWO_WAY);
	addGroupIntoCatalog(groups[TELEPORTS], true, false, Obj::SUBTERRANEAN_GATE);
	addGroupIntoCatalog(groups[TELEPORTS], true, false, Obj::WHIRLPOOL);
	addGroupIntoCatalog(groups[MINES], true, false, Obj::MINE);
	addGroupIntoCatalog(groups[MINES], false, false, Obj::ABANDONED_MINE);
	addGroupIntoCatalog(groups[MINES], true, false, Obj::WINDMILL);
	addGroupIntoCatalog(groups[MINES], true, false, Obj::WATER_WHEEL);
	addGroupIntoCatalog(groups[TRIGGERS], true, false, Obj::EVENT);
	addGroupIntoCatalog(groups[TRIGGERS], true, false, Obj::GRAIL);
	addGroupIntoCatalog(groups[TRIGGERS], true, false, Obj::SIGN);
	addGroupIntoCatalog(groups[TRIGGERS], true, false, Obj::OCEAN_BOTTLE);
	addGroupIntoCatalog(groups[MONSTERS], false, false, Obj::MONSTER);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L1);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L2);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L3);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L4);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L5);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L6);
	addGroupIntoCatalog(groups[MONSTERS], true, false, Obj::RANDOM_MONSTER_L7);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::SEER_HUT);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::BORDER_GATE);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::QUEST_GUARD);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::HUT_OF_MAGI);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::EYE_OF_MAGI);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::BORDERGUARD);
	addGroupIntoCatalog(groups[QUESTS], true, false, Obj::KEYMASTER);
	addGroupIntoCatalog(groups[WOG_OBJECTS], true, false, Obj::WOG_OBJECT);
	addGroupIntoCatalog(groups[OBSTACLES], true);
	addGroupIntoCatalog(groups[OTHER], false);
	}
	catch(const std::exception &)
	{
		QMessageBox::critical(this, tr("Mods loading problem"), tr("Critical error during Mods loading. Disable invalid mods and restart."));
	}
}

void EditorMainWindow::on_actionUndo_triggered()
{
	QString str(tr("Undo clicked"));
	statusBar()->showMessage(str, 1000);

	if (controller.map())
	{
		controller.undo();
	}
}

void EditorMainWindow::on_actionRedo_triggered()
{
	QString str(tr("Redo clicked"));
	displayStatus(str);

	if (controller.map())
	{
		controller.redo();
	}
}

void EditorMainWindow::on_actionPass_triggered(bool checked)
{
	QString str(tr("Passability clicked"));
	displayStatus(str);

	if(controller.map())
	{
		for(int level = 0; level < controller.map()->levels(); ++level)
			controller.scene(level)->passabilityView.show(checked);
	}
}


void EditorMainWindow::on_actionGrid_triggered(bool checked)
{
	QString str(tr("Grid clicked"));
	displayStatus(str);

	if(controller.map())
	{
		for(int level = 0; level < controller.map()->levels(); ++level)
			controller.scene(level)->gridView.show(checked);
	}
}

void EditorMainWindow::changeBrushState(int idx)
{

}

void EditorMainWindow::on_actionErase_triggered()
{
	if(controller.map())
	{
		controller.commitObjectErase(mapLevel);
	}
}

void EditorMainWindow::preparePreview(const QModelIndex &index)
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


void EditorMainWindow::treeViewSelected(const QModelIndex & index, const QModelIndex & deselected)
{
	ui->toolSelect->setChecked(true);
	ui->mapView->selectionTool = MapView::SelectionTool::None;
	
	preparePreview(index);
}

void EditorMainWindow::on_terrainFilterCombo_currentIndexChanged(int index)
{
	if(!objectBrowser)
		return;

	QString uniqueName = ui->terrainFilterCombo->itemData(index).toString();

	objectBrowser->terrain = TerrainId(ETerrainId::ANY_TERRAIN);
	if (!uniqueName.isEmpty())
	{
		for (auto const & terrain : LIBRARY->terrainTypeHandler->objects)
			if (terrain->getJsonKey() == uniqueName.toStdString())
				objectBrowser->terrain = terrain->getId();
	}
	objectBrowser->invalidate();
	objectBrowser->sort(0);
}

void EditorMainWindow::on_filter_textChanged(const QString &arg1)
{
	if(!objectBrowser)
		return;

	objectBrowser->filter = arg1;
	objectBrowser->invalidate();
	objectBrowser->sort(0);
}


void EditorMainWindow::on_actionFill_triggered()
{
	QString str(tr("Fill clicked"));
	displayStatus(str);

	if(!controller.map())
		return;

	controller.commitObstacleFill(mapLevel);
}

void EditorMainWindow::loadInspector(CGObjectInstance * obj, bool switchTab)
{
	if(switchTab)
		ui->tabWidget->setCurrentIndex(1);
	Inspector inspector(controller, obj, ui->inspectorWidget);
	inspector.updateProperties();
}

void EditorMainWindow::on_inspectorWidget_itemChanged(QTableWidgetItem *item)
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

void EditorMainWindow::on_actionMapSettings_triggered()
{
	if(mapSettings->isVisible())
	{
		mapSettings->raise();
		mapSettings->activateWindow();
	}
	else
	{
		mapSettings->show();
	}
}


void EditorMainWindow::on_actionPlayers_settings_triggered()
{
	auto settingsDialog = new PlayerSettingsDialog(controller, this);
	settingsDialog->setWindowModality(Qt::WindowModal);
	settingsDialog->setModal(true);
	connect(settingsDialog, &QDialog::finished, this, &EditorMainWindow::onPlayersChanged);
}

QAction * EditorMainWindow::getActionPlayer(const PlayerColor & player)
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

void EditorMainWindow::switchDefaultPlayer(const PlayerColor & player)
{
	if(controller.defaultPlayer == player)
		return;

	QSignalBlocker blockerNeutral(ui->actionNeutral);
	ui->actionNeutral->setChecked(PlayerColor::NEUTRAL == player);

	for(int i = 0; i < PlayerColor::PLAYER_LIMIT.getNum(); ++i)
	{
		QAction * playerEntry = getActionPlayer(PlayerColor(i));
		QSignalBlocker blocker(playerEntry); 
		playerEntry->setChecked(PlayerColor(i) == player);
	}
	controller.defaultPlayer = player;
}

void EditorMainWindow::onPlayersChanged()
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



void EditorMainWindow::enableUndo(bool enable)
{
	ui->actionUndo->setEnabled(enable);
}

void EditorMainWindow::enableRedo(bool enable)
{
	ui->actionRedo->setEnabled(enable);
}

void EditorMainWindow::onSelectionMade(int level, bool anythingSelected)
{
	if (level == mapLevel)
	{
		ui->actionErase->setEnabled(anythingSelected);
	}
}
void EditorMainWindow::displayStatus(const QString& message, int timeout /* = 2000 */)
{
	statusBar()->showMessage(message, timeout);
}

void EditorMainWindow::on_actionValidate_triggered()
{
	new Validator(controller.map(), this);
}


void EditorMainWindow::on_actionUpdate_appearance_triggered()
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
		auto handler = LIBRARY->objtypeh->getHandlerFor(obj->ID, obj->subID);
		if(!controller.map()->isInTheMap(obj->visitablePos()))
		{
			++errors;
			continue;
		}
		
		auto * terrain = controller.map()->getTile(obj->visitablePos()).getTerrain();
		
		if(handler->isStaticObject())
		{
			staticObjects.insert(obj);
			if(obj->appearance->canBePlacedAt(terrain->getId()))
			{
				controller.scene(mapLevel)->selectionObjectsView.deselectObject(obj);
				continue;
			}
			std::vector<int3> selectedTiles;
			for(auto & offset : obj->appearance->getBlockedOffsets())
				selectedTiles.push_back(obj->pos + offset);
			controller.scene(mapLevel)->selectionTerrainView.select(selectedTiles);
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


void EditorMainWindow::on_actionRecreate_obstacles_triggered()
{

}


void EditorMainWindow::on_actionMapLayer_triggered()
{
	auto & currentType = controller.map()->mapLayers[mapLevel];
	int currentPos = 0;

	QList<QPair<QString, MapLayerId>> layers;
	for(const auto & layer : LIBRARY->mapLayerHandler->objects)
	{
		if(currentType == layer->getId())
			currentPos = layers.size();
		layers.append(qMakePair(QString::fromStdString(layer->getNameTranslated()), layer->getId()));
	}
	QStringList layerNames;
	for (const auto &p : layers)
		layerNames << p.first;

    bool ok = false;
    QString selected = QInputDialog::getItem(
        nullptr,
        tr("Select map layer type"),
        tr("Type:"),
        layerNames,
        currentPos,
        false,
        &ok
    );

	if(ok)
	{    
		for (const auto & p : layers)
		{
			if (p.first == selected)
			{
				currentType = p.second;
				
				for(auto &box : levelComboBoxes)
					box->setItemText(mapLevel, tr("Level %1: %2")
										.arg(mapLevel + 1)
										.arg(QString::fromStdString(currentType.toEntity(LIBRARY)->getNameTranslated())));
			}
		}
	}
}


void EditorMainWindow::on_actionCut_triggered()
{
	if(controller.map())
	{
		controller.copyToClipboard(mapLevel);
		controller.commitObjectErase(mapLevel);
	}
}


void EditorMainWindow::on_actionCopy_triggered()
{
	if(controller.map())
	{
		controller.copyToClipboard(mapLevel);
	}
}


void EditorMainWindow::on_actionPaste_triggered()
{
	if(controller.map())
	{
		controller.pasteFromClipboard(mapLevel);
	}
}


void EditorMainWindow::on_actionExport_triggered()
{
	QString selectedFilter;
	QString contentUri;
#ifdef VCMI_ANDROID
	// On Android, ask for image format before opening the SAF file picker so
	// we can pass the correct MIME type and file extension.
	QMessageBox fmtBox(this);
	fmtBox.setWindowTitle(tr("Image format"));
	fmtBox.setText(tr("Select image format:"));
	auto * btnPng  = fmtBox.addButton("PNG",  QMessageBox::AcceptRole);
	auto * btnJpeg = fmtBox.addButton("JPEG", QMessageBox::AcceptRole);
	auto * btnBmp  = fmtBox.addButton("BMP",  QMessageBox::AcceptRole);
	fmtBox.addButton(QMessageBox::Cancel);
	fmtBox.exec();

	QString imgFilter;
	QString imgFormat;
	if     (fmtBox.clickedButton() == btnPng)  { imgFilter = "PNG (*.png)";   imgFormat = "PNG";  }
	else if(fmtBox.clickedButton() == btnJpeg) { imgFilter = "JPEG (*.jpeg)"; imgFormat = "JPEG"; }
	else if(fmtBox.clickedButton() == btnBmp)  { imgFilter = "BMP (*.bmp)";   imgFormat = "BMP";  }
	else return;

	QString fileName = EditorFileDialog::getSaveFileName(this, tr("Save to image"), lastSavingDir,
		imgFilter, contentUri, /*externalOnly=*/true);
#else
	QString imgFormat;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save to image"), lastSavingDir, "BMP (*.bmp);;JPEG (*.jpeg);;PNG (*.png)", &selectedFilter);
#endif
 
	if(!fileName.isNull())
	{
		QFileInfo fileInfo(fileName);
		if(fileInfo.suffix().isEmpty())
		{
			QString extension;
			if(selectedFilter.contains("*.bmp", Qt::CaseInsensitive))
				extension = "bmp";
			else if(selectedFilter.contains("*.jpeg", Qt::CaseInsensitive))
				extension = "jpeg";
			else if(selectedFilter.contains("*.png", Qt::CaseInsensitive))
				extension = "png";

			if(!extension.isEmpty())
				fileName += "." + extension;
		}
		lastSavingDir = QFileInfo(fileName).dir().path();

		auto * sc = static_cast<MapScene*>(ui->mapView->scene());
		if(!sc)
			return;

		QRectF sceneRect = sc->sceneRect();

		// Temporarily set viewport to full map for export
		for (auto * layer : sc->getDynamicLayers())
			layer->setViewport(sceneRect);

		QImage image(sceneRect.size().toSize(), QImage::Format_RGB888);
		QPainter painter(&image);
		sc->render(&painter, QRectF(), sceneRect);
		QByteArray imgFormatBytes = imgFormat.isEmpty() ? QByteArray{} : imgFormat.toLatin1();
		const bool saved = image.save(fileName, imgFormatBytes.isEmpty() ? nullptr : imgFormatBytes.constData());

		// Restore viewport to visible area
		ui->mapView->setViewports();

		if(!saved)
		{
			QMessageBox::critical(this, tr("Failed to save image"), tr("Cannot save image to %1.").arg(fileName));
			return;
		}
		
		// Restore viewport to visible area
		ui->mapView->setViewports();

		EditorFileDialog::writeFileToUri(fileName, contentUri);
	}
}


void EditorMainWindow::on_actionTranslations_triggered()
{
	auto translationsDialog = new Translations(*controller.map(), this);
	translationsDialog->show();
}

void EditorMainWindow::on_actionh3m_converter_triggered()
{
	auto mapFiles = QFileDialog::getOpenFileNames(this, tr("Select maps to convert"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()),
		tr("HoMM3 maps(*.h3m)"));
	if(mapFiles.empty())
		return;

#ifdef VCMI_ANDROID
	QString contentUri;
	QString saveDirectory = EditorFileDialog::getSaveFileName(this, tr("Choose directory to save converted maps"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()) + "/Maps",
		tr("Directory"), contentUri);
	if(saveDirectory.isEmpty())
		return;
	saveDirectory = QFileInfo(saveDirectory).absolutePath();
#else
	auto saveDirectory = QFileDialog::getExistingDirectory(this, tr("Choose directory to save converted maps"), QCoreApplication::applicationDirPath());
	if(saveDirectory.isEmpty())
		return;
#endif
	
	try
	{
		for(auto & m : mapFiles)
		{
			CMapService mapService;
			auto map = Helper::openMapInternal(m, controller.getCallback());
			controller.getCallback()->setMap(map.get());
			controller.repairMap(map.get());
			QString outVmap = saveDirectory + '/' + QFileInfo(m).completeBaseName() + ".vmap";
			mapService.saveMap(map, outVmap.toStdString());
		}
		QMessageBox::information(this, tr("Operation completed"), tr("Successfully converted %1 maps").arg(mapFiles.size()));
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, tr("Failed to convert the map. Abort operation"), tr(e.what()));
	}
}

void EditorMainWindow::on_actionh3c_converter_triggered()
{
	auto dir = QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string());

	auto openTitle = tr("Select campaign to convert");
	auto openFilter = tr("HoMM3 campaigns (*.h3c)");

	auto campaignFile = EditorFileDialog::getOpenFileName(this, openTitle, dir, openFilter);
	if(campaignFile.isEmpty())
		return;

	auto saveTitle = tr("Select destination file");
	auto saveFilter = tr("VCMI campaigns (*.vcmp)");

	QString contentUri;
	auto campaignFileDest = EditorFileDialog::getSaveFileName(this, saveTitle, dir, saveFilter, contentUri);
	if(campaignFileDest.isEmpty())
		return;

	QFileInfo fileInfo(campaignFileDest);
	if(fileInfo.suffix().toLower() != "vcmp")
		campaignFileDest += ".vcmp";
	auto campaign = Helper::openCampaignInternal(campaignFile);
	Helper::saveCampaign(campaign, campaignFileDest);

	EditorFileDialog::writeFileToUri(campaignFileDest, contentUri);
}

void EditorMainWindow::on_actionLock_triggered()
{
	if(controller.map())
	{
		if(controller.scene(mapLevel)->selectionObjectsView.getSelection().empty())
		{
			for(auto obj : controller.map()->objects)
			{
				controller.scene(mapLevel)->selectionObjectsView.setLockObject(obj.get(), true);
				controller.scene(mapLevel)->objectsView.setLockObject(obj.get(), true);
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
	}
}


void EditorMainWindow::on_actionUnlock_triggered()
{
	if(controller.map())
	{
		controller.scene(mapLevel)->selectionObjectsView.unlockAll();
		controller.scene(mapLevel)->objectsView.unlockAll();
	}
}


void EditorMainWindow::on_actionZoom_in_triggered()
{
	auto rect = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	rect -= QMargins{32 + 1, 32 + 1, 32 + 2, 32 + 2}; //compensate bounding box
	ui->mapView->fitInView(rect, Qt::KeepAspectRatioByExpanding);
}


void EditorMainWindow::on_actionZoom_out_triggered()
{
	auto rect = ui->mapView->mapToScene(ui->mapView->viewport()->geometry()).boundingRect();
	rect += QMargins{32 - 1, 32 - 1, 32 - 2, 32 - 2}; //compensate bounding box
	ui->mapView->fitInView(rect, Qt::KeepAspectRatioByExpanding);
}


void EditorMainWindow::on_actionZoom_reset_triggered()
{
	auto center = ui->mapView->mapToScene(ui->mapView->viewport()->geometry().center());
	ui->mapView->fitInView(initialScale, Qt::KeepAspectRatioByExpanding);
	ui->mapView->centerOn(center);
}


void EditorMainWindow::on_toolLine_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Line;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolBrush2_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush2;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolBrush_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolBrush4_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Brush4;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolLasso_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Lasso;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolArea_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Area;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolFill_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::Fill;
		ui->tabWidget->setCurrentIndex(0);
	}
}


void EditorMainWindow::on_toolSelect_toggled(bool checked)
{
	if(checked)
	{
		ui->mapView->selectionTool = MapView::SelectionTool::None;
		ui->tabWidget->setCurrentIndex(0);
	}
}
