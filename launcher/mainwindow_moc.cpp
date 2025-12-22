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

#include "../vcmiqt/launcherdirs.h"

#include "updatedialog_moc.h"
#include "main.h"
#include "helper.h"

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

	setAcceptDrops(true);

	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->modslistButton->setIcon(QIcon{":/icons/menu-mods.png"});
	ui->settingsButton->setIcon(QIcon{":/icons/menu-settings.png"});
	ui->aboutButton->setIcon(QIcon{":/icons/about-project.png"});
	ui->startGameButton->setIcon(QIcon{":/icons/menu-game.png"});

#ifndef VCMI_MOBILE
	//load window settings
	QSettings s = CLauncherDirs::getSettings(Ui::appName);

	auto size = s.value("MainWindow/WindowSize").toSize();
	if(size.isValid())
	{
		resize(size);
	}
	auto position = s.value("MainWindow/WindowPosition").toPoint();
	if(!position.isNull())
	{
		move(position);
	}
#endif

	computeSidePanelSizes();

	bool h3DataFound = CResourceHandler::get()->existsResource(ResourcePath("DATA/GENRLTXT.TXT"));

	if (h3DataFound && setupCompleted)
		ui->tabListWidget->setCurrentIndex(TabRows::START);
	else
		enterSetup();

	ui->settingsView->setDisplayList();
	
	if(settings["launcher"]["updateOnStartup"].Bool())
		UpdateDialog::showUpdateDialog(false);
}

void MainWindow::detectPreferredLanguage()
{
	auto preferredLanguages = QLocale::system().uiLanguages();

	std::string selectedLanguage;

	for (auto const & userLang : preferredLanguages)
	{
		logGlobal->info("Preferred language: %s", userLang.toStdString());

		for (auto const & vcmiLang : Languages::getLanguageList())
			if (vcmiLang.tagIETF == userLang.toStdString() && vcmiLang.selectable)
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
}

void MainWindow::exitSetup(bool goToMods)
{
	Settings writer = settings.write["launcher"]["setupCompleted"];
	writer->Bool() = true;

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
#ifndef VCMI_MOBILE
	//save window settings
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	s.setValue("MainWindow/WindowSize", size());
	s.setValue("MainWindow/WindowPosition", pos());
#endif

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
			for(const auto & ending : QStringList({".zip", ".h3m", ".h3c", ".vmap", ".vcmp", ".json", ".exe"}))
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
