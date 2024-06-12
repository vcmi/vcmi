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
#include "../lib/VCMIDirs.h"
#include "../lib/Languages.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"

#include "updatedialog_moc.h"
#include "main.h"
#include "helper.h"

void MainWindow::load()
{
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

#ifndef VCMI_MOBILE
	console = new CConsoleHandler();
#endif
	CBasicLogConfigurator logConfig(VCMIDirs::get().userLogsPath() / "VCMI_Launcher_log.txt", console);
	logConfig.configureDefault();

	CResourceHandler::initialize();
	CResourceHandler::load("config/filesystem.json");

	Helper::loadSettings();
}

void MainWindow::computeSidePanelSizes()
{
	QVector<QToolButton*> widgets = {
		ui->modslistButton,
		ui->settingsButton,
		ui->aboutButton,
		ui->startEditorButton,
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

	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->modslistButton->setIcon(QIcon{":/icons/menu-mods.png"});
	ui->settingsButton->setIcon(QIcon{":/icons/menu-settings.png"});
	ui->aboutButton->setIcon(QIcon{":/icons/about-project.png"});
	ui->startEditorButton->setIcon(QIcon{":/icons/menu-editor.png"});
	ui->startGameButton->setIcon(QIcon{":/icons/menu-game.png"});

#ifndef VCMI_MOBILE
	//load window settings
	QSettings s(Ui::teamName, Ui::appName);

	auto size = s.value("MainWindow/Size").toSize();
	if(size.isValid())
	{
		resize(size);
	}
	auto position = s.value("MainWindow/Position").toPoint();
	if(!position.isNull())
	{
		move(position);
	}
#endif

#ifndef ENABLE_EDITOR
	ui->startEditorButton->hide();
#endif

	computeSidePanelSizes();

	bool h3DataFound = CResourceHandler::get()->existsResource(ResourcePath("DATA/GENRLTXT.TXT"));

	if (h3DataFound && setupCompleted)
		ui->tabListWidget->setCurrentIndex(TabRows::MODS);
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
			if (vcmiLang.tagIETF == userLang.toStdString())
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
	ui->startGameButton->setEnabled(false);
	ui->startEditorButton->setEnabled(false);
	ui->settingsButton->setEnabled(false);
	ui->aboutButton->setEnabled(false);
	ui->modslistButton->setEnabled(false);
	ui->tabListWidget->setCurrentIndex(TabRows::SETUP);
}

void MainWindow::exitSetup()
{
	Settings writer = settings.write["launcher"]["setupCompleted"];
	writer->Bool() = true;

	ui->startGameButton->setEnabled(true);
	ui->startEditorButton->setEnabled(true);
	ui->settingsButton->setEnabled(true);
	ui->aboutButton->setEnabled(true);
	ui->modslistButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::MODS);
}

void MainWindow::switchToModsTab()
{
	ui->startGameButton->setEnabled(true);
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
	QSettings s(Ui::teamName, Ui::appName);
	s.setValue("MainWindow/Size", size());
	s.setValue("MainWindow/Position", pos());
#endif

	delete ui;
}

void MainWindow::on_startGameButton_clicked()
{
	startGame({});
}

void MainWindow::on_startEditorButton_clicked()
{
	startEditor({});
}

const CModList & MainWindow::getModList() const
{
	return ui->modlistView->getModList();
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

void MainWindow::updateTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	const std::string translationFile = settings["general"]["language"].String() + ".qm";
	logGlobal->info("Loading translation '%s'", translationFile);

	if (!translator.load(QString{":/translation/%1"}.arg(translationFile.c_str())))
	{
		logGlobal->error("Failed to load translation");
		return;
	}

	if (!qApp->installTranslator(&translator))
		logGlobal->error("Failed to install translator");
#endif
}
