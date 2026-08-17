/*
 * mainwindow_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "mainwindow_moc.h"
#include "ui_mainwindow_moc.h"

#include <QDir>

#include "../lib/CConfigHandler.h"
#include "../lib/CConsoleHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/texts/Languages.h"
#include "../lib/ExceptionsCommon.h"

#include "updatedialog_moc.h"
#include "main.h"
#include "helper.h"
#ifndef VCMI_MOBILE
#include "gamepadHandler.h"
#endif

void MainWindow::load()
{
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

#ifndef VCMI_MOBILE
	console = std::make_unique<CConsoleHandler>();
	CBasicLogConfigurator logConfigurator(VCMIDirs::get().userLogsPath() / "VCMI_Launcher_log.txt", console.get());
#else
	CBasicLogConfigurator logConfigurator(VCMIDirs::get().userLogsPath() / "VCMI_Launcher_log.txt", nullptr);
#endif

	logConfigurator.configureDefault();

	try
	{
		CResourceHandler::initialize();
		CResourceHandler::load("config/filesystem.json");
	}
	catch (const DataLoadingException & e)
	{
		QMessageBox::critical(this, tr("Error starting executable"), QString::fromStdString(e.what()));
	}

	Helper::loadSettings();
}

void MainWindow::computeSidePanelSizes()
{
	QVector<QToolButton*> widgets = {
		ui->modslistButton,
		ui->settingsButton,
		ui->aboutButton,
		ui->startGameButton
	};

	for(auto & widget : widgets)
	{
		QFontMetrics metrics(widget->font());
		QSize iconSize = widget->iconSize();

		// this is minimal space that is needed for our button to avoid text clipping
		int buttonHeight = iconSize.height() + metrics.height() + 4;

		widget->setMinimumHeight(buttonHeight);
		widget->setMaximumHeight(buttonHeight * 1.2);
	}
}

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	load(); // load FS before UI

	bool setupCompleted = settings["launcher"]["setupCompleted"].Bool();

	if (!setupCompleted)
		detectPreferredLanguage();

	updateTranslation(); // load translation

	ui->setupUi(this);

#ifndef VCMI_MOBILE
	connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *)
	{
		// Some platforms update the window's screen association after screenRemoved
		QTimer::singleShot(250, this, &MainWindow::handleScreenRemoved);
	});
	connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *)
	{
		// A newly connected screen only changes the available choices. Do not move
		// the launcher: its current screen and geometry are still valid.
		QTimer::singleShot(0, ui->settingsView, &CSettingsView::setDisplayList);
	});
	// The native window handle is created after the widget enters the event loop.
	QTimer::singleShot(0, this, [this]()
	{
		if(auto * handle = windowHandle())
		{
			connect(handle, &QWindow::screenChanged, this, &MainWindow::updateDisplayIndex);
			updateDisplayIndex(handle->screen());
		}
	});
#endif

	setAcceptDrops(true);

	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->modslistButton->setIcon(QIcon{":/icons/menu-mods.png"});
	ui->settingsButton->setIcon(QIcon{":/icons/menu-settings.png"});
	ui->aboutButton->setIcon(QIcon{":/icons/about-project.png"});
	ui->startGameButton->setIcon(QIcon{":/icons/menu-game.png"});

#ifndef VCMI_MOBILE
	restoreWindowSettings();
#endif

	computeSidePanelSizes();

	bool h3DataFound = CResourceHandler::get()->existsResource(ResourcePath("DATA/GENRLTXT.TXT"));

	if (h3DataFound && setupCompleted)
		ui->tabListWidget->setCurrentIndex(TabRows::START);
	else
		enterSetup();

	setGamepadStartAllowed(h3DataFound && setupCompleted);

	ui->settingsView->setDisplayList();

#ifndef VCMI_MOBILE
	auto * gamepadHandler = new GamepadHandler(this);
	connect(gamepadHandler, &GamepadHandler::startGameRequested, this, &MainWindow::startGameFromGamepad);
#endif

	if(settings["launcher"]["updateOnStartup"].Bool())
		UpdateDialog::showUpdateDialog(false);
}

void MainWindow::setGamepadStartAllowed(bool allowed)
{
	gamepadStartAllowed = allowed;
	Helper::allowGamepadStart(allowed);
}

void MainWindow::startGameFromGamepad()
{
	// setup and dialogs can't be operated with a gamepad, so don't take control away from the player
	if(!gamepadStartAllowed || QApplication::activeModalWidget())
		return;

	hide();
	startGame({});
}

void MainWindow::centerWindowOnScreen(QScreen * screen)
{
#ifndef VCMI_MOBILE
	if(screen == nullptr)
		return;

	// Use frame geometry so native window decorations stay inside the screen too.
	const QRect availableGeometry = screen->availableGeometry();
	const QSize currentWindowSize = frameGeometry().size();
	QSize windowSize = currentWindowSize.boundedTo(availableGeometry.size());
	if(windowSize != currentWindowSize)
		resize(windowSize);

	move(availableGeometry.center() - QPoint(windowSize.width() / 2, windowSize.height() / 2));
#endif
}

void MainWindow::ensureWindowVisibleOnExistingScreen()
{
#ifndef VCMI_MOBILE
	// The frame center identifies the monitor on which most of the window belongs.
	const QRect windowGeometry = frameGeometry();
	QScreen * screen = QGuiApplication::screenAt(windowGeometry.center());
	if(screen == nullptr)
	{
		centerWindowOnScreen(QGuiApplication::primaryScreen());
		return;
	}

	const QRect availableGeometry = screen->availableGeometry();
	QSize windowSize = windowGeometry.size().boundedTo(availableGeometry.size());
	if(windowSize != windowGeometry.size())
		resize(windowSize);

	QPoint windowPosition(
		qBound(availableGeometry.left(), windowGeometry.left(), availableGeometry.right() - windowSize.width() + 1),
		qBound(availableGeometry.top(), windowGeometry.top(), availableGeometry.bottom() - windowSize.height() + 1));
	move(windowPosition);
#endif
}

void MainWindow::handleScreenRemoved()
{
#ifndef VCMI_MOBILE
	ensureWindowVisibleOnExistingScreen();
	updateDisplayIndex(QGuiApplication::screenAt(frameGeometry().center()));
	saveWindowSettings();
	ui->settingsView->setDisplayList();
#endif
}

void MainWindow::restoreWindowSettings()
{
#ifndef VCMI_MOBILE
	const auto & windowGeometry = settings["launcher"]["mainWindow"]["geometry"];
	const QSize windowSize(windowGeometry["width"].Integer(), windowGeometry["height"].Integer());

	// Missing geometry receives zero-valued schema defaults. QSize::isValid considers 0x0
	// valid, but it does not represent a previously saved window size.
	if(!windowSize.isEmpty())
	{
		resize(windowSize);
		move(windowGeometry["x"].Integer(), windowGeometry["y"].Integer());

		// Keep the saved monitor when it still exists, but always start centered on it.
		QScreen * screen = QGuiApplication::screenAt(frameGeometry().center());
		centerWindowOnScreen(screen ? screen : QGuiApplication::primaryScreen());
		return;
	}

	// When no saved window geometry is available, ensure there is enough room for
	// all controls. On screens where the minimum window would not fit with window
	// decorations, use the entire screen instead.
	static const QSize minimumLauncherSize(800, 600);
	QScreen * screen = QGuiApplication::primaryScreen();
	if(screen == nullptr)
		return;

	const QSize availableSize = screen->availableSize();
	if(availableSize.width() <= minimumLauncherSize.width() || availableSize.height() <= minimumLauncherSize.height())
	{
		setWindowState(windowState() | Qt::WindowFullScreen);
		return;
	}

	// Use a common widescreen size on larger displays. On smaller displays, limit
	// the initial window size to 90% of the available screen area while respecting
	// the minimum launcher size, so the window keeps a visible margin and does not
	// look fullscreen.
	static const QSize widescreenLauncherSize(1366, 768);
	const QSize screenSize = screen->geometry().size();
	const bool isWidescreen = screenSize.width() * 3 > screenSize.height() * 4;
	const QSize preferredSize = isWidescreen ? widescreenLauncherSize : minimumLauncherSize;
	const QSize maximumInitialSize = availableSize * 0.9;
	resize(preferredSize.boundedTo(maximumInitialSize).expandedTo(minimumLauncherSize));
	centerWindowOnScreen(screen);
#endif
}

void MainWindow::updateDisplayIndex(QScreen * screen)
{
#ifndef VCMI_MOBILE
	// QGuiApplication screen order is the order exposed by the display-index setting.
	const int screenIndex = QGuiApplication::screens().indexOf(screen);
	if(screenIndex < 0 || screenIndex == settings["video"]["displayIndex"].Integer())
		return;

	Settings node = settings.write["video"]["displayIndex"];
	node->Integer() = screenIndex;
	ui->settingsView->setDisplayList();
#endif
}

void MainWindow::moveToScreen(int screenIndex)
{
#ifndef VCMI_MOBILE
	const auto screens = QGuiApplication::screens();
	if(screenIndex < 0 || screenIndex >= screens.size())
		return;

	centerWindowOnScreen(screens[screenIndex]);
	saveWindowSettings();
#endif
}

void MainWindow::saveWindowSettings()
{
#ifndef VCMI_MOBILE
	ensureWindowVisibleOnExistingScreen();

	Settings windowGeometry = settings.write["launcher"]["mainWindow"]["geometry"];
	windowGeometry["x"].Integer() = pos().x();
	windowGeometry["y"].Integer() = pos().y();
	windowGeometry["width"].Integer() = size().width();
	windowGeometry["height"].Integer() = size().height();
#endif
}

void MainWindow::detectPreferredLanguage()
{
	auto preferredLanguages = QLocale::system().uiLanguages();

	std::string selectedLanguage;

	for (auto const & userLang : preferredLanguages)
	{
		logGlobal->info("Preferred language: %s", userLang.toStdString());

		for (auto const & vcmiLang : Languages::getLanguageList())
			if (vcmiLang.selectable && (vcmiLang.tagIETF == userLang.toStdString() || vcmiLang.localeName == userLang.toStdString()))
				selectedLanguage = vcmiLang.identifier;

		if (!selectedLanguage.empty())
		{
			logGlobal->info("Selected language: %s", selectedLanguage);
			Settings node = settings.write["general"]["language"];
			node->String() = selectedLanguage;
			return;
		}
	}
}

void MainWindow::enterSetup()
{
	ui->sidePanel->setVisible(false);
	ui->tabListWidget->setCurrentIndex(TabRows::SETUP);
	ui->setupView->enterSetup();
}

void MainWindow::exitSetup(bool goToMods)
{
	Settings writer = settings.write["launcher"]["setupCompleted"];
	writer->Bool() = true;

	setGamepadStartAllowed(true);

	ui->sidePanel->setVisible(true);
	if (goToMods)
		switchToModsTab();
	else
		switchToStartTab();
}

void MainWindow::switchToStartTab()
{
	ui->startGameButton->setEnabled(true);
	ui->startGameButton->setChecked(true);
	ui->tabListWidget->setCurrentIndex(TabRows::START);

	auto* startGameTabWidget = qobject_cast<StartGameTab*>(ui->tabListWidget->widget(TabRows::START));
	if(startGameTabWidget)
		startGameTabWidget->refreshState();
}

void MainWindow::switchToModsTab()
{
	ui->startGameButton->setEnabled(true);
	ui->modslistButton->setChecked(true);
	ui->tabListWidget->setCurrentIndex(TabRows::MODS);
}

void MainWindow::changeEvent(QEvent * event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QMainWindow::changeEvent(event);
}

MainWindow::~MainWindow()
{
	saveWindowSettings();

	delete ui;
}

void MainWindow::on_startGameButton_clicked()
{
	switchToStartTab();
}

CModListView * MainWindow::getModView()
{
	return ui->modlistView;
}

void MainWindow::on_modslistButton_clicked()
{
	switchToModsTab();
}

void MainWindow::on_settingsButton_clicked()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::SETTINGS);
}

void MainWindow::on_aboutButton_clicked()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::ABOUT);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if(event->mimeData()->hasUrls())
		for(const auto & url : event->mimeData()->urls())
			for(const auto & ending : QStringList({".zip", ".vsgm1", ".h3m", ".h3c", ".vmap", ".vcmp", ".json", ".exe"}))
				if(url.fileName().endsWith(ending, Qt::CaseInsensitive))
				{
					event->acceptProposedAction();
					return;
				}
}

void MainWindow::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	if(mimeData->hasUrls())
	{
		const QList<QUrl> urlList = mimeData->urls();
		for (const auto & url : urlList)
			manualInstallFile(url.toLocalFile());
	}
}

void MainWindow::manualInstallFile(QString filePath)
{
	QString realFilePath = Helper::getRealPath(filePath);

	if(realFilePath.endsWith(".zip", Qt::CaseInsensitive) || realFilePath.endsWith(".exe", Qt::CaseInsensitive))
		switchToModsTab();

	QString fileName = QFileInfo{filePath}.fileName();
	if(realFilePath.endsWith(".zip", Qt::CaseInsensitive))
	{
		QString filenameClean = fileName.toLower()
			// mod name currently comes from zip file -> remove suffixes from github zip download
			.replace(QRegularExpression("-[0-9a-f]{40}"), "")
			.replace(QRegularExpression("-vcmi-.+\\.zip"), ".zip")
			.replace("-main.zip", ".zip");

		getModView()->downloadFile(filenameClean, QUrl::fromLocalFile(filePath), "mods");
	}
	else if(realFilePath.endsWith(".json", Qt::CaseInsensitive))
	{
		QDir configDir(QString::fromStdString(VCMIDirs::get().userConfigPath().string()));
		QStringList configFile = configDir.entryList({fileName}, QDir::Filter::Files); // case insensitive check
		if(!configFile.empty())
		{
			auto dialogResult = QMessageBox::warning(this, tr("Replace config file?"), tr("Do you want to replace %1?").arg(configFile[0]), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if(dialogResult == QMessageBox::Yes)
			{
				const auto configFilePath = configDir.filePath(configFile[0]);
				QFile::remove(configFilePath);
				Helper::performNativeCopy(filePath, configFilePath);

				Helper::reLoadSettings();
			}
		}
	}
	else
		getModView()->installFiles(QStringList{filePath});
}

ETranslationStatus MainWindow::getTranslationStatus()
{
	QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
	QString installedlanguage = QString::fromStdString(settings["session"]["language"].String());

	if (preferredlanguage == installedlanguage)
		return ETranslationStatus::ACTIVE;

	QString modName = getModView()->getTranslationModName(preferredlanguage);

	if (modName.isEmpty())
		return ETranslationStatus::NOT_AVAILABLE;

	if (!getModView()->isModInstalled(modName))
		return ETranslationStatus::NOT_INSTALLLED;

	if (!getModView()->isModEnabled(modName))
		return ETranslationStatus::DISABLED;

	return ETranslationStatus::ACTIVE;
}

void MainWindow::updateTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	const std::string translationFile = settings["general"]["language"].String()+ ".qm";
	QString translationFileResourcePath = QString{":/translation/%1"}.arg(translationFile.c_str());

	logGlobal->info("Loading translation %s", translationFile);

	qApp->removeTranslator(&translator);

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
