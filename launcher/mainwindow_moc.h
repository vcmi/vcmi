/*
 * mainwindow_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QMainWindow>
#include <QStringList>
#include <QTranslator>

class CConsoleHandler;

namespace Ui
{
class MainWindow;
const QString appName = "launcher";
}

class QScreen;
class QTableWidgetItem;
class CModList;
class CModListView;

enum class ETranslationStatus : int8_t
{
	NOT_AVAILABLE, // translation for this language was not found in mod list. Could also happen if player is offline or disabled repository checkout
	NOT_INSTALLLED, // translation mod found, but it is not installed
	DISABLED, // translation mod found, and installed, but toggled off
	ACTIVE // translation mod active OR game is already in specified language (e.g. English H3 for players with English language)
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

#ifdef ENABLE_QT_TRANSLATIONS
	QTranslator translator;
#endif
	Ui::MainWindow * ui;

#ifndef VCMI_MOBILE
	std::unique_ptr<CConsoleHandler> console;
#endif

	bool gamepadStartAllowed = false;

	void load();
	void setGamepadStartAllowed(bool allowed);
	void startGameFromGamepad();
	void centerWindowOnScreen(QScreen * screen);
	void ensureWindowVisibleOnExistingScreen();
	void handleScreenRemoved();
	void restoreWindowSettings();
	void saveWindowSettings();
	void updateDisplayIndex(QScreen * screen);

	enum TabRows
	{
		MODS = 0,
		SETTINGS = 1,
		SETUP = 2,
		ABOUT = 3,
		START = 4,
	};

public:
	explicit MainWindow(QWidget * parent = nullptr);
	~MainWindow() override;

	CModListView * getModView();

	void updateTranslation();
	void computeSidePanelSizes();

	void detectPreferredLanguage();
	void enterSetup();
	void exitSetup(bool goToMods);
	void switchToModsTab();
	void switchToStartTab();
	void moveToScreen(int screenIndex);

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent *event) override;

	void manualInstallFile(QString filePath);
	ETranslationStatus getTranslationStatus();

protected:
	void changeEvent(QEvent * event) override;

public slots:
	void on_startGameButton_clicked();

private slots:
	void on_modslistButton_clicked();
	void on_settingsButton_clicked();
	void on_aboutButton_clicked();
};
